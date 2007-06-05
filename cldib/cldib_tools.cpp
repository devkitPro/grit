//
//! \file cldib_tools.cpp
//!   cldib_tools implementation file
//! \date 20050823 - 20051228
//! \author cearn
//
// === NOTES ===

#include "cldib_core.h"
#include "cldib_tools.h"

// === GLOBALS ========================================================

// --------------------------------------------------------------------
// FUNCTIONS
// --------------------------------------------------------------------

//! Find the distance bwtween two RGBQUADS
DWORD rgb_dist(const RGBQUAD *a, const RGBQUAD *b)
{
	DWORD sum;
	sum  = (a->rgbRed   - b->rgbRed)   * (a->rgbRed   - b->rgbRed);
	sum += (a->rgbGreen - b->rgbGreen) * (a->rgbGreen - b->rgbGreen);
	sum += (a->rgbBlue  - b->rgbBlue)  * (a->rgbBlue  - b->rgbBlue);
	return sum;
}


// --- DIB FUNCTIONS --------------------------------------------------

//! Redimensions or retiles the contents of the DIB
/*!
	\param src
*/
// CHK: unpadded works
// DESC: redimensions/(un)tiles a dib into a column of tiles with 
//   dimensions dstW x tileH. Can also work in reverse (if srcW<dstW)
// NOTE: padding pixels may cause problems
CLDIB *dib_redim(CLDIB *src, int dstW, int tileH, int tileN)
{
	if(src == NULL)
		return NULL;

	int srcW, srcH, srcB, srcP;
	dib_get_attr(src, &srcW, &srcH, &srcB, &srcP);

	// Force byte alignment
	if( (dstW*srcB&7) )
		return NULL;

	// setup redim
	int srcR= srcW*srcB>>3, dstR= dstW*srcB>>3;	// bytes/row

	RECORD srcRec= { srcR, srcH, dib_get_img(src) };
	RECORD dstRec= { dstR, 0, 0 };

	int ii;
	BYTE *srcD= NULL;

	if(srcR&3)	// depad for src
	{
		srcD= (BYTE*)malloc(srcR*srcH);
		if(srcD == NULL)
			return NULL;
		for(ii=0; ii<srcH; ii++)
			memcpy(&srcD[ii*srcR], &srcRec.data[ii*srcP], srcR);
		srcRec.data= srcD;
	}
	BOOL bOK= data_redim(&srcRec, &dstRec, tileH, tileN);
	SAFE_FREE(srcD);
	if(!bOK)
		return NULL;

	CLDIB *dst= dib_alloc(dstW, dstRec.height, srcB, NULL, dib_is_topdown(src));
	if(dst == NULL)
		return NULL;

	int dstH= dib_get_height(dst);	// bytes/row
	int dstP= dib_get_pitch(dst);
	BYTE *dstD= dib_get_img(dst);

	if(dstR&3)	// repad for dst
	{
		for(ii=0; ii<dstH; ii++)
			memcpy(&dstD[ii*dstP], &dstRec.data[ii*dstR], dstP);
	}
	else
		memcpy(dstD, dstRec.data, dstP*dstH);
	SAFE_FREE(dstRec.data);

	memcpy(dib_get_pal(dst), dib_get_pal(src), 
		dib_get_nclrs(src)*RGB_SIZE);

	return dst;
}

// --- DATA FUNCTIONS -------------------------------------------------

// Matrix redimensioner/tiler
// Rearranges a dib into a column of tiles of tileW x tileH dimensions.
// The resulting dib is tileW by tileH*tileN in size.
// The widths are in bytes, not pixels

// NOTE: is applied in the following ways
//   if(tileW<srcW)
//     create a vertical tile array.
//     if srcW != multiple of tileW, a tile is cut
//   if(tileW>srcW)
//     'decolumn', create a tile matrix
//     if tileW != a multiple of srcW, a tile is padded in
//   if(tileW==srcW)
//     nothing to do, return a clone 
	// matrix vs array configuration
	// tN: tile count
    //           width   |  height  | m=w/tw   | n=h/th
	// tile:      tW     |    tH    |   1      |   1
	// array:   aW=tW    | aH=tN*tH | aM=1     | aN=tN
	// matrix:  mW=mM*tW | mH=mN*tH | mM=mW/aW | mN=(tN+mM-1)/mM
	//
	//
	//     <---- mW ---->          -aW-
	// |  +----+----+----+        +----+
	// |  | tW |    |    | |    | |    |  |
	// |  |  0 |  1 |mM-1| | tH | |  0 |  |
	// |  +----+----+----+        +----+  |
	// |  | mM |  4 |    |        |    |  |
	// mH                         |  1 |  |
    // |  |    |    |    |        |    |  |
 	// |  +----+----+----+        +----+  |
	// |  |    |    |    |        |    |  |
	// |  |tN-2|tN-1|    |
	// |  +----+----+----+        |    |  |
	//                            +----+  |
	//                            |    |  |
	//                            |tN-1|  aH=tN*tH
	//                            +----+  |

// src: w, h, data of source
// dst: w of destination; other fields are output
// tileH: height of a tile
// tileN: # output tiles; if <=0, number is taken from source dims.
BOOL data_redim(const RECORD *src, RECORD *dst, int tileH, int tileN)
{
	int ii, jj;
	int srcW= src->width, srcH= src->height, dstW= dst->width;

	// must be able to fit at least one tile
	if(srcH < tileH)
		return FALSE;

	BYTE *srcL, *dstL;

	if(srcW == dstW)		// srcW == tileH, already tiled
	{
		dst->height= srcH;
		dst->data= (BYTE*)malloc(srcW*srcH);
		memcpy(dst->data, src->data, srcW*srcH);
	}
	else if(srcW > dstW)	// --- matrix -> array
	{
		int tileW= dstW, tileS= tileW*tileH;
		int srcU= srcW/tileW, srcV= srcH/tileH;
		if(tileN<=0 || tileN>srcU*srcV)
			tileN= srcU*srcV;

		dst->height= tileN*tileH;
		dst->data= (BYTE*)malloc(tileN*tileS);

		dstL= dst->data;
		for(ii=0; ii<tileN; ii++)
		{
			// find tile start
			srcL= &src->data[(ii/srcU)*srcW*tileH + (ii%srcU)*tileW];
			// copy tile, line by line
			for(jj=0; jj<tileH; jj++)
			{
				memcpy(dstL, srcL+jj*srcW, tileW);
				dstL += tileW;
			}
		}
	}
	else					// --- array -> matrix
	{
		int tileW= srcW, tileS= tileW*tileH;

		if(tileN<=0 || tileN*tileH>srcH)
			tileN= srcH/tileH;

		int dstU= dstW/tileW, dstV= (tileN+dstU-1)/dstU;
		dst->height= dstV*tileH;
		dst->data= (BYTE*)malloc(dstW*dstV*tileH);
		memset(dst->data, 0, dstW*dstV*tileH);

		srcL= src->data;
		for(ii=0; ii<tileN; ii++)
		{
			// find tile start
			dstL= &dst->data[(ii/dstU)*dstW*tileH + (ii%dstU)*tileW];
			// copy tile, line by line
			for(jj=0; jj<tileH; jj++)
			{
				memcpy(dstL+jj*dstW, srcL, tileW);
				srcL += tileW;
			}
		}						
	}

	return TRUE;
}

// --------------------------------------------------------------------
// Redox functions
// --------------------------------------------------------------------


//! Reduce the number of colors of dib and/or merge with an external palette.
/*!	\param dib	DIB to reduce the palette of. Must be paletted.
*	\param extPal	External palette record. \a dib will use this 
*	  and its own palette. Can be NULL. The new reduced palette goes here too.
*	\return	Number of reduced colors, or 0 if not 8bpp or if the reduced
*	  palette would exceed 256 entries.
*/
int dib_pal_reduce(CLDIB *dib, RECORD *extPal)
{
	// Only for 8bpp (for now)
	if(dib == NULL || dib_get_bpp(dib) != 8)
		return 0;

	int ii, jj, kk, ix, iy;

	int dibW, dibH, dibP;
	dib_get_attr(dib, &dibW, &dibH, NULL, &dibP);

	BYTE *dibD= dib_get_img(dib);

	// Get palette histogram
	int histo[256];

	memset(histo, 0, sizeof(histo));
	for(iy=0; iy<dibH; iy++)
		for(ix=0; ix<dibW; ix++)
			histo[dibD[iy*dibP+ix]]++;

	// Allocate room for new palette and init with ext pal
	// NOTE: extPal is assumed reduced!
	// NOTE: double-size for rdxPal for worst-case scenario.
	// NOTE: the *Clr things are just to make comparisons easier.
	//		 pointers ftw!

	int nn=0;
	RGBQUAD *rdxPal= (RGBQUAD*)malloc(512*RGB_SIZE);
	COLORREF *rdxClr= (COLORREF*)rdxPal, *dibClr= (COLORREF*)dib_get_pal(dib);

	memset(rdxPal, 0, 512*RGB_SIZE);
	if(extPal && extPal->data)
	{
		memcpy(rdxPal, extPal->data, rec_size(extPal));
		nn= extPal->height;
	}

	// PONDER: always keep index 0 ?

	// Create reduced palette and prep tables for pixel conversion.
	DWORD srcI[PAL_MAX], dstI[PAL_MAX];

	kk=0;
	for(ii=0; ii<PAL_MAX; ii++)
	{
		if(histo[ii])
		{
			for(jj=0; jj<nn; jj++)
				if(rdxClr[jj] == dibClr[ii])
					break;
			// Match: add color to table
			if(jj == nn)
			{
				rdxClr[jj]= dibClr[ii];
				nn++;
			}
			srcI[kk]= jj;
			dstI[kk]= ii;
			kk++;
		}
	}

	// PONDER: what *should* happen if nn > PAL_MAX ?
	// Fail, trunc or re-quantize?
	// Currently just fail.
	if(nn > PAL_MAX)
		return 0;

	//  Update palette and remap pixels
	memcpy(dibClr, rdxClr, PAL_MAX*RGB_SIZE);
	dib_pixel_replace(dib, dstI, srcI, kk);

	// Update rdxPal's data
	if(extPal)
	{
		extPal->width= RGB_SIZE;
		extPal->height= nn;
		free(extPal->data);

		extPal->data= (BYTE*)malloc(nn*RGB_SIZE);
		memcpy(extPal->data, rdxClr, nn*RGB_SIZE);
	}

	return nn;
}


// --------------------------------------------------------------------
// FILE READ/WRITE INTERFACE
// --------------------------------------------------------------------


fnDibLoad dib_load= dib_load_dflt;	//!< File reader function pointer
fnDibSave dib_save= dib_save_dflt;	//!< File writer function pointer

//! Set the file-reading interface to \a proc.
/*!	\return	current load procedure
*/
fnDibLoad dib_set_load_proc(fnDibLoad proc)
{
	fnDibLoad old_proc= dib_load;
	dib_load= proc;
	return old_proc;
}

//! Set the file-writing interface to \a proc.
/*!	\return	current load procedure
*/
fnDibSave dib_set_save_proc(fnDibSave proc)
{
	fnDibSave old_proc= dib_save;
	dib_save= proc;
	return old_proc;
}

//! Default/dummy file-reader (does nothing)
CLDIB *dib_load_dflt(const char *fpath, void *extra)
{
	return NULL;
}

//! Default/dummy file-writer (does nothing)
BOOL dib_save_dflt(const CLDIB *dib, const char *fpath, void *extra)
{
	return FALSE;
}


// EOF
