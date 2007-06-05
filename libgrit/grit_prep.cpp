//
//! \file grit_prep.cpp
//!   Data preparation routines
//! \date 20050814 - 20070226
//! \author cearn
//
// === NOTES === 
// * 20061209,CLD: Fixed NDS alpha stuff. 
// * 20060727,CLD: TODO: alpha clr(id) corrections
// * 20060727,CLD: use RGB16 for pal conversion
// * 20060727,CLD: endian stuff for prep_map, prep_img, prep_pal
// * PONDER: external tile dib for grit_prep_map
// * PONDER: tile sort for grit_prep_map
// * PONDER: all records for grit_tile_reduce?

#include <stdio.h>
#include <stdlib.h>
//#include <sys/param.h>

#include <cldib.h>
#include "grit.h"


// --------------------------------------------------------------------
// PROTOTYPES
// --------------------------------------------------------------------


BOOL grit_prep_work_dib(GRIT_REC *gr);
BOOL grit_prep_tiles(GRIT_REC *gr);

BOOL grit_prep_map(GRIT_REC *gr);
BOOL grit_prep_img(GRIT_REC *gr);
BOOL grit_prep_pal(GRIT_REC *gr);

u16 grit_find_tile_pal(BYTE *tileD);
BOOL grit_tile_cmp(BYTE *test, BYTE *base, u32 x_xor, u32 y_xor, BYTE mask);
CLDIB *grit_tile_reduce(RECORD *dst, CLDIB *srcDib, u32 flags, CLDIB *extDib);
RECORD *grit_meta_reduce(RECORD *dst, const RECORD *src, int metaN, u32 flags);


// --------------------------------------------------------------------
// FUNCTIONS
// --------------------------------------------------------------------


//! Data preparation hub
/*!	Converts and prepares the bitmap for export. 
	
*/
BOOL grit_prep(GRIT_REC *gr)
{
	// TODO: clear internals
	lprintf(LOG_STATUS, "Preparing data.\n");

	if(grit_prep_work_dib(gr) == FALSE)
	{
		lprintf(LOG_ERROR, "  No work DIB D: .\n");
		return FALSE;
	}

	// NOTE: don't check for -g! just yet: -g! + -m is possible too
	// if tiles: prep tiles & map
	if(~gr->img_flags & GRIT_IMG_BMP)
	{
		grit_prep_tiles(gr);
		if(gr->map_flags & GRIT_INCL)
			grit_prep_map(gr);
	}

	if(gr->img_flags & GRIT_INCL)
		grit_prep_img(gr);
	
	if(gr->pal_flags & GRIT_INCL)
		grit_prep_pal(gr);


	lprintf(LOG_STATUS, "Data preparation complete.\n");		
	return TRUE;
}

//! Sets up work bitmap of desired size and 8 or 16 bpp.
/*!	This basically does two things. First, create a bitmap from the 
	designated area of the source bitmap. Then, converts it 8 or 
	16 bpp, depending on \a gr.img_bpp. Conversion to lower bpp is 
	done later, when it's more convenient. The resultant bitmap is 
	put into \a gr._dib, and will be used in later preparation.
*/
BOOL grit_prep_work_dib(GRIT_REC *gr)
{
	int ii, nn;
	RGBQUAD *rgb;

	lprintf(LOG_STATUS, "Work-DIB creation.\n");		

	// --- resize ---
	CLDIB *dib= dib_copy(gr->src_dib, 
		gr->area_left, gr->area_top, gr->area_right, gr->area_bottom, 
		FALSE);
	if(dib == NULL)
	{
		lprintf(LOG_ERROR, "  Work-DIB creation failed.\n");		
		return FALSE;
	}
	// ... that's it? Yeah, looks like.

	// --- resample (to 8 or 16) ---
	// 
	int dibB= dib_get_bpp(dib);

	// Convert to 16bpp, but ONLY for bitmaps
	if( gr->img_bpp == 16 && GRIT_IS_BMP(gr) )
	{
		if(dibB != 16)
		{
			lprintf(LOG_WARNING, 
"  converting from %d bpp to %d bpp.\n", dibB, gr->img_bpp);
			CLDIB *dib2= dib_convert(dib, 16, 0);

			// If paletted src AND -pT AND NOT -gT[!]
			//   use trans color pal[T]
			if( dibB <=8 && (gr->pal_flags & GRIT_PAL_TRANS) 
				&& ( gr->img_flags & (GRIT_IMG_TRANS|GRIT_IMG_BMP_A) )==0 )
			{
				rgb= &dib_get_pal(dib)[gr->pal_trans];
				lprintf(LOG_WARNING, 
"  pal->true-color conversion with transp pal-id option.\n"
"    using color %02X%02X%02X", 
					rgb->rgbRed, rgb->rgbGreen, rgb->rgbBlue);
				
				gr->img_flags |= GRIT_IMG_TRANS;
				gr->img_trans= *rgb;
			}

			dib_free(dib);

			if(dib2 == NULL)
			{
				lprintf(LOG_ERROR, "prep: Bpp conversion failed.\n");	
				return FALSE;
			}
			dib= dib2;
		}

		// --- Dealing with 16bpp images ---
		// Modes: 
		// 555      | GBA           | swap R_B   
		// 555+A=1  | NDS           | swap R_B + add NDS_ALPHA
		// 555+A    | NDS, transp   | swap R_B + NDS_ALPHA if !trans-clr
		// src-pal + -pT -> NDS, transp

		// Swap palette bRGB to bBGR
		// And resolve -gT
		nn= dib_get_size_img(dib)/2;
		WORD *dibD2= (WORD*)dib_get_img(dib);

		// Single transparent color
		if(gr->img_flags & GRIT_IMG_TRANS)
		{
			rgb= &gr->img_trans;
			WORD clr= RGB16(rgb->rgbBlue, rgb->rgbGreen, rgb->rgbRed), wd;

			lprintf(LOG_STATUS, 
"  converting to: 16bpp BGR, alpha=1, except for 0x%04X.\n", clr);

			for(ii=0; ii<nn; ii++)
			{
				wd= swap_rgb16(dibD2[ii]);
				dibD2[ii]= (wd == clr ? wd : wd | NDS_ALPHA);	
			}	
		}
		else if(gr->img_flags & GRIT_IMG_BMP_A)
		{
			lprintf(LOG_STATUS, "converting to: 16bpp BGR, alpha=1.\n");

			for(ii=0; ii<nn; ii++)
				dibD2[ii]= swap_rgb16(dibD2[ii]) | NDS_ALPHA;
		}
		else
		{
			lprintf(LOG_STATUS, "converting to: 16bpp, BGR.\n");

			for(ii=0; ii<nn; ii++)
				dibD2[ii]= swap_rgb16(dibD2[ii]);				
		}
	}
	else if(dibB != 8)	// otherwise, convert to 8bpp
	{
		lprintf(LOG_WARNING, 
"  converting from %d bpp to %d bpp.\n", dibB, gr->img_bpp);
		
		CLDIB *dib2= dib_convert(dib, 8, 0);

		dib_free(dib);
		if(dib2 == NULL)
		{
			lprintf(LOG_ERROR, "  Bpp conversion failed.\n");	
			return FALSE;
		}
		dib= dib2;
	}

	// palette transparency additions
	if(dib_get_bpp(dib)==8)
	{
		// If img-trans && !pal-trans:
		//   Find img-trans in palette and use that
		if( (gr->img_flags & GRIT_IMG_TRANS) && 
			(~gr->pal_flags & GRIT_PAL_TRANS) )
		{
			rgb= &gr->img_trans;
			RGBQUAD *pal= dib_get_pal(dib);

			lprintf(LOG_WARNING, 
"  tru/pal -> pal conversion with transp color option.\n"
"    looking for color %02X%02X%02X in palette.\n", 
				rgb->rgbRed, rgb->rgbGreen, rgb->rgbBlue);
			
			UINT ii_min= 0, dist, dist_min;
			dist_min= rgb_dist(rgb, &pal[0]);

			for(ii=1; ii<256; ii++)
			{
				dist= rgb_dist(rgb, &pal[ii]);
				if(dist < dist_min)
				{
					ii_min= ii;
					dist_min= dist;
				}
			}

			// HACK: count 'match' only if average error is < +/-14
			if(dist_min < 576)
			{
				gr->pal_flags |= GRIT_PAL_TRANS;
				gr->pal_trans= ii_min;
			}
		}

		// Swap alpha and pixels palette entry
		if(gr->pal_flags&GRIT_PAL_TRANS)
		{
			lprintf(LOG_STATUS, 
"  palette transparency: pal[%d].\n", gr->pal_trans);
			BYTE *imgD= dib_get_img(dib);
			nn= dib_get_size_img(dib);

			for(ii=0; ii<nn; ii++)
			{
				if(imgD[ii] == 0)
					imgD[ii]= gr->pal_trans;
				else if(imgD[ii] == gr->pal_trans)
					imgD[ii]= 0;
			}
			RGBQUAD tmp, *pal= dib_get_pal(dib);
			SWAP3(pal[0], pal[gr->pal_trans], tmp);
		}
	}

	dib_free(gr->_dib);
	gr->_dib= dib;
	
	lprintf(LOG_STATUS, "Work-DIB creation complete: %dx%d@%d.\n", 
		dib_get_width(gr->_dib), dib_get_height(gr->_dib), 
		dib_get_bpp(gr->_dib));		
	return TRUE;
}

//! Rearranges the work dib into a strip of 8x8 tiles.
/*!	This only runs for tiled images and can have up to three 
	stages. First, Rearrange into screen-blocks (option -mLr); then 
	into metatiles (-M[w h]), then base-tiles.
*/
BOOL grit_prep_tiles(GRIT_REC *gr)
{
	lprintf(LOG_STATUS, "Tile preparation.\n");		

	CLDIB *dib= NULL;

	// sbb redim
	if(gr->map_flags & GRIT_MAP_REG)
	{
		lprintf(LOG_STATUS, "  tiling to 256x256 tiles.\n");
		dib= dib_redim(gr->_dib, 256, 256, 0);
		if(dib == NULL)
		{
			lprintf(LOG_ERROR, "  SBB tiling failed.\n");		
			return FALSE;
		}
		dib_free(gr->_dib);
		gr->_dib= dib;
	}

	// meta redim
	int mw= gr->meta_width, mh= gr->meta_height;
	if(mw*mh>1)
	{
		lprintf(LOG_STATUS, "  tiling to %dx%d tiles.\n", mw*8, mh*8);
		dib= dib_redim(gr->_dib, mw*8, mh*8, 0);
		if(dib == NULL)
		{
			lprintf(LOG_ERROR, "  meta tiling failed.\n");		
			return FALSE;
		}
		dib_free(gr->_dib);
		gr->_dib= dib;
	}

	// tile redim
	lprintf(LOG_STATUS, "  tiling to 8x8 tiles.\n");

	dib= dib_redim(gr->_dib, 8, 8, 0);
	if(dib == NULL)
	{
		lprintf(LOG_ERROR, "  8x8 tiling failed.\n");		
		return FALSE;
	}
	dib_free(gr->_dib);
	gr->_dib= dib;

	lprintf(LOG_STATUS, "Tile preparation complete.\n");		
	return TRUE;
}

//! Prepares map and meta map.
/*!	Does map creation and layout, tileset reduction and map 
	compression. Updates \a gr._dib with the new tileset, and fills 
	in \a gr._map_rec and \a gr._meta_rec.
	\note The work bitmap must be 8 bpp here, and already rearranged 
	to a tile strip, which are the results of \c grit_prep_work_dib() 
	and \c grit_prep_tiles(), respectively.
*/
BOOL grit_prep_map(GRIT_REC *gr)
{
	lprintf(LOG_STATUS, "Map preparation.\n");		

	if(dib_get_bpp(gr->_dib) != 8)
	{
		lprintf(LOG_ERROR, 
"  Mapping only works on 8bpp images.\n");
		return FALSE;
	}

	int ii;
	u32 flags;

	// map variabes
	int mapW, mapH, mapN;
	u16 *mapD;
	RECORD mapRec= { 0, 0, NULL };
	
	flags= gr->map_flags;
	mapW= (gr->area_right-gr->area_left)/8;
	mapH= (gr->area_bottom-gr->area_top)/8;
	mapN= mapW*mapH;

	// tileset variables
	BYTE *tileD;
	CLDIB *tileDib= gr->_dib, *rdxDib;

	// --- Create base map ---
	if(~flags & GRIT_RDX_ON)	// no reduction; tileD is all tiles
	{
		lprintf(LOG_STATUS, 
"  No tile reduction.\n");

		mapRec.width= 2;
		mapRec.height= mapN;
		mapRec.data= (BYTE*)malloc(2*mapN);
		mapD= (u16*)mapRec.data;

		// point to tile data (== work dib)
		tileD= dib_get_img(tileDib);
		// map with palette info
		for(ii=0; ii<mapN; ii++)
			mapD[ii]= grit_find_tile_pal(&tileD[ii*64]) | (ii&SE_ID_MASK);
	}
	else					// reducing tiles
	{
		lprintf(LOG_STATUS, 
"  Performing tile reduction: tiles%s%s\n", 
			(flags & GRIT_RDX_FLIP ? ", flip" : ""), 
			(flags & GRIT_RDX_PAL  ? ", palette" : "") );

		// TODO: add external dib
		if(gr->img_flags & GRIT_IMG_SHARED)
			rdxDib= grit_tile_reduce(&mapRec, tileDib, flags, NULL);
		else
			rdxDib= grit_tile_reduce(&mapRec, tileDib, flags, 
				gr->shared->dib);


		if(rdxDib == NULL)
		{
			free(mapRec.data);
			lprintf(LOG_ERROR, 
"  creation of tile DIB failed.\n");
			return FALSE;
		}
		dib_free(gr->_dib);
		gr->_dib= rdxDib;

		// Make extra copy for external tile dib
		// PONDER: do I really need a copy for this?
		if(gr->img_flags & GRIT_IMG_SHARED)
		{
			dib_free(gr->shared->dib);
			gr->shared->dib= dib_clone(rdxDib);
		}

	}
	mapD= (u16*)mapRec.data;

	// --- Map tile offset ---
	if(gr->map_ofs)
	{
		lprintf(LOG_ERROR, 
"  Applying tile offset (0x%08X).\n", gr->map_ofs);

		u32 se, base= gr->map_ofs&~OFS_BASE0;
		BOOL bBase0= gr->map_ofs&OFS_BASE0;
		for(ii=0; ii<mapN; ii++)
		{
			se= mapD[ii]&SE_ID_MASK;
			if(bBase0 || se)
				se += base;
			mapD[ii] &= ~SE_ID_MASK;
			mapD[ii] |= (se & SE_ID_MASK);
		}
	}

	// --- Create meta map ---
	int metaW, metaH, metaN;
	RECORD metaRec= { 0, 0, NULL };

	metaW= gr->meta_width;
	metaH= gr->meta_height;
	metaN= metaW*metaH;
	if(metaN > 1)
	{
		lprintf(LOG_STATUS, 
"  Performing metatile reduction (%d, %d).\n", metaW, metaH);

		RECORD *rec= grit_meta_reduce(&metaRec, &mapRec, metaN, flags);
		// Relay mapRec to use the reduced metatile set
		if(rec)
		{
			rec_alias(&mapRec, rec);
			free(rec);
		}
		// PONDER: metamap compression?
	}
	mapD= (u16*)mapRec.data;

	// Layout for affine (basically halfword->byte)
	if(flags & GRIT_MAP_AFF)
	{
		lprintf(LOG_STATUS, 
"  Converting to affine layout.\n", metaW, metaH);

		int size= rec_size(&mapRec);
		BYTE *buf= (BYTE*)malloc(size);
		if(buf != NULL)
		{
			size /= 2;
			for(ii=0; ii<size; ii++)
				buf[ii]= (BYTE)mapD[ii];
			free(mapRec.data);
			mapRec.data= buf;
			mapRec.width= 1;
			mapRec.height= size;
		}
	}
	if( BYTE_ORDER == BIG_ENDIAN && (~flags & GRIT_MAP_AFF) )
		data_byte_rev(mapRec.data, mapRec.data, rec_size(&mapRec), 2);		

	// compress map
	grit_compress(&mapRec, &mapRec, flags);
	rec_alias(&gr->_map_rec, &mapRec);
	rec_alias(&gr->_meta_rec, &metaRec);

	lprintf(LOG_STATUS, "Map preparation complete.\n");		
	return TRUE;
}

//! Image data preparation.
/*!	Prepares the work dib for export, i.e. converts to the final 
	bitdepth, compresses the data and fills in \a gr._img_rec.
*/
BOOL grit_prep_img(GRIT_REC *gr)
{
	lprintf(LOG_STATUS, "Graphics preparation.\n");		

	int srcB= dib_get_bpp(gr->_dib);	// should be 8 or 16 by now
	int srcP= dib_get_pitch(gr->_dib);
	int srcS= dib_get_size_img(gr->_dib);
	BYTE *srcD= dib_get_img(gr->_dib);

	int dstB= gr->img_bpp;
	// # dst bytes, with # src pixels as 'width'
	int dstS= dib_align(srcS*8/srcB, dstB);
	dstS= ALIGN4(dstS);
	BYTE *dstD= (BYTE*)malloc(dstS);
	if(dstD == NULL)
	{
		lprintf(LOG_ERROR, "  Can't allocate graphics data.\n");
		return FALSE;
	}

	// Convert to final bitdepth
	// NOTE: do not use dib_convert here, because of potential
	//   problems with padding
	// NOTE: we're already at 8 or 16 bpp here, with 16 bpp already 
	//   accounted for. Only have to do 8->1,2,4
	// TODO: offset
	if(srcB == 8 && srcB != dstB)
	{
		lprintf(LOG_STATUS, 
"  Bitpacking: %d -> %d.\n", srcB, dstB);
		data_bit_pack(dstD, srcD, srcS, srcB, dstB, 0);
	}
	else
		memcpy(dstD, srcD, dstS);

	RECORD rec= { 1, dstS, dstD };

	if( BYTE_ORDER == BIG_ENDIAN && gr->img_bpp == 16 )
		data_byte_rev(rec.data, rec.data, rec_size(&rec), 2);		

	// attach and compress graphics
	grit_compress(&rec, &rec, gr->img_flags);
	rec_alias(&gr->_img_rec, &rec);

	lprintf(LOG_STATUS, "Graphics preparation complete.\n");		
	return TRUE;
}

//! Palette data preparation
/*!	Converts palette to 16bit GBA colors, compresses it and fills in 
	\a gr._pal_rec.	
*/
BOOL grit_prep_pal(GRIT_REC *gr)
{
	lprintf(LOG_STATUS, "Palette preparation.\n");		

	int ii, nclrs, palS;
	COLOR *palOut;
	RGBQUAD *palIn;

	nclrs= gr->pal_end - gr->pal_start;
	if(dib_get_nclrs(gr->_dib) < nclrs && nclrs != 0)
		nclrs= dib_get_nclrs(gr->_dib);

	palS= ALIGN4(nclrs*sizeof(COLOR));
	palOut= (COLOR*)malloc(palS);
	palIn= &dib_get_pal(gr->_dib)[gr->pal_start];

	for(ii=0; ii<nclrs; ii++)
		palOut[ii]= RGB16(palIn[ii].rgbBlue, palIn[ii].rgbGreen, 
			palIn[ii].rgbRed);

	RECORD rec= { 2, palS/2, (BYTE*)palOut };

	if( BYTE_ORDER == BIG_ENDIAN )
		data_byte_rev(rec.data, rec.data, rec_size(&rec), 2);		

	// attach and compress palette
	grit_compress(&rec, &rec, gr->pal_flags);
	rec_alias(&gr->_pal_rec, &rec);

	lprintf(LOG_STATUS, "Palette preparation complete.\n");		
	return TRUE;
}


// --------------------------------------------------------------------
// HELPERS
// --------------------------------------------------------------------


//! Checks a tile for the palette bank. 
/*!
	\param tileD Tile data array (8x8\@8)
	\return pal_id\<\<12.
*/
u16 grit_find_tile_pal(BYTE *tileD)
{
	int ii;
	for(ii=0; ii<64; ii++)
		if( (tileD[ii]&0x0F) != 0)
			return (tileD[ii]&0xF0)<<8;
	
	return 0;
}

//! Compares 8x8 tiles \a test and \a base. 
/*!	Both tiles must be 8x8\@8
	\param test Data of the tile to test
	\param base Data of the tile to compare \a test to.
	\param x_xor Use 7 for horz-flip check, 0 for normal.
	\param y_xor Use 7 for vert-flip check, 0 for normal.
	\param mask Checks only specific bits.
	  (0x0F for pal-banked, 0xFF for full palette)
	\return \c TRUE if tiles are equal, \c FALSE of they're not.
*/
BOOL grit_tile_cmp(BYTE *test, BYTE *base, u32 x_xor, u32 y_xor, BYTE mask)
{
	int ix, iy;
	for(iy=0; iy<8; iy++)
	{
		for(ix=0; ix<8; ix++)
		{	
			if( (test[(iy^y_xor)*8+(ix^x_xor)]&mask) != (base[iy*8+ix]&mask) )
				return FALSE;
		}
	}
	return TRUE;
}

//! Creates tilemap and reduces tileset
/*!
	\param dst		Record to receive the reduced map.
	\param srcDib	Dib to map and tile-reduce.
	\param flags	Reduction flags (\c GRIT_RDX_xxx combo).
	\param extDib	External tileset.
	\return	Reduced tileset.
	\note	Both \a srcDib and \a extDib must already be in 8bpp 
		tiled format!
*/
CLDIB *grit_tile_reduce(RECORD *dst, CLDIB *srcDib, u32 flags, CLDIB *extDib)
{
	// ASSERT(dst);
	// ASSERT(srcDib);

	if(dib_get_bpp(srcDib) != 8)
		return NULL;

	int srcW= dib_get_width(srcDib), srcH= dib_get_height(srcDib);
	int srcN= srcW*srcH/64;		// # tiles in srcDib
	int rdxN;					// # reduced tiles
	
	CLDIB *rdxDib;

	if(extDib)
	{
		lprintf(LOG_STATUS, "Appending to external tileset.\n");
				
		// PONDER: check/fix tile_dib stuff here?
		//   no, in validation (later)

		int extH= dib_get_height(extDib);
		rdxN= extH/8;
		rdxDib= dib_alloc(8, (srcN + rdxN)*8, 8, NULL);
		memcpy(dib_get_img(rdxDib), dib_get_img(extDib), rdxN*64);
	}
	else
	{
		lprintf(LOG_STATUS, "Creating new tileset.\n");	
			
		rdxN= 1;
		rdxDib= dib_alloc(8, (srcN+rdxN)*8, 8, NULL);
		memset(dib_get_img(rdxDib), 0, 64);
	}

	// assume worst case, srcN separate tiles (+1 for empty)
	if(rdxDib == NULL)
	{
		lprintf(LOG_ERROR, "Can't allocate reduced tileset.\n");		
		return NULL;
	}

	int mapS= 2*srcN;
	free(dst->data);
	dst->width= 2;
	dst->height= srcN;
	dst->data= (BYTE*)malloc(mapS);
	if(dst->data == NULL)
	{
		lprintf(LOG_ERROR, "Can't allocate map buffer.\n");		
		dib_free(rdxDib);
		return NULL;
	}
	memset(dst->data, 0, mapS);

	// # tiles in reduces set. Start at 1 because I want an empty 
	// tile at the start
	u32 se, clrmask;
	clrmask= (flags&GRIT_RDX_PAL ? 0x0F : 0xFF);
	u16 *mapD= (u16*)dst->data;

	int ii, jj;
	BYTE *srcD= dib_get_img(srcDib), *srcL= srcD;
	BYTE *rdxD= dib_get_img(rdxDib), *rdxL= rdxD;

	for(ii=0; ii<srcN; ii++)	// loop over all tiles
	{
		rdxL= rdxD;
		se= 0;
		for(jj=0; jj<rdxN; jj++)	// look over reduced tiles
		{
			// check for direct match
			if(grit_tile_cmp(srcL, rdxL, 0, 0, clrmask) )
				break;
			// try flipped match, if desired
			if(flags & GRIT_RDX_FLIP)
			{
				if(grit_tile_cmp(srcL, rdxL, 0, 7, clrmask) )
				{	se |= SE_VFLIP;		break;	}
				if(grit_tile_cmp(srcL, rdxL, 7, 0, clrmask) )
				{	se |= SE_HFLIP;		break;	}
				if(grit_tile_cmp(srcL, rdxL, 7, 7, clrmask) )
				{	se |= SE_FLIP_MASK;	break;	}
			}
			rdxL += 64;
		}

		se |= jj & SE_ID_MASK;
		// no match, add this tile to the set
		if(jj == rdxN)
		{
			memcpy(&rdxD[rdxN*64], srcL, 64);	// PONDER: isn't this rdxL ?
			rdxN++;
		}
		// Add palette info (Although adding pal-info even for 8bpp 
		// wouldn't matter on the gba, it _would_ matter in terms of
		// compression and meta-tiling.)
		if(flags & GRIT_RDX_PAL)
			se |= grit_find_tile_pal(srcL);

		mapD[ii]= (u16)se;
		srcL += 64;
	}

	// reduction complete, crop rdx dib and return home
	CLDIB *dib= dib_alloc(8, rdxN*8, 8, rdxD);
	if(dib != NULL)
		memcpy(dib_get_pal(dib), dib_get_pal(srcDib), 
			dib_get_nclrs(srcDib)*RGB_SIZE);

	dib_free(rdxDib);

	return dib;
}

//! Creates meta-map and reduces metatile set
/*
	\param dst Record to receive metamap.
	\param src Record of the map to create metamap of.
	\param tileN Number of tiles in a metatile.
	\param flags Reduction flags.
	\returns Metatile set (i.e. map-entry groups).
*/
RECORD *grit_meta_reduce(RECORD *dst, const RECORD *src, int tileN, u32 flags)
{
	// ASSERT(src);
	// ASSERT(dst);
	// ASSERT(metaN);
	if(dst==NULL || src==NULL || src->data==NULL)
		return NULL;

	int srcS= rec_size(src), srcN= srcS/2;
	int dstS= srcS/tileN, dstN=dstS/2;

	u16 *srcD= (u16*)src->data;		// src: original map
	u16 *dstD= (u16*)malloc(dstS);	// dst: metamap
	u16 *mtsD= (u16*)malloc(srcS);	// mts: metatile set

	if(dstD==NULL || mtsD==NULL)
	{
		free(dstD);
		free(mtsD);
		return NULL;
	}

	int ii, jj, kk, mm=0;

	u32 mme, mask= (flags&GRIT_META_PAL ? 0x0FFF : 0xFFFF);
	u16 *srcL= srcD, *mtsL= mtsD;
	for(ii=0; ii<dstN; ii++)	// check all metamap entries
	{
		mtsL= mtsD;
		for(jj=0; jj<mm; jj++)
		{
			for(kk=0; kk<tileN; kk++)
				if( (srcL[kk]^mtsL[kk]) & mask )
					break;
			if(kk == tileN)	// found a match
				break;
			mtsL += tileN;
		}
		mme= jj;
		if(jj == mm)	// new metatile, register it
		{
			memcpy(&mtsD[mm*tileN], srcL, tileN*2);
			mm++;			
		}
		if(flags & GRIT_META_PAL)	// add metal tile palett info
		{
			for(jj=0; jj<tileN; jj++)
			{
				if( (srcL[jj]&0xF000) != 0)
				{	mme |= (srcL[jj]&0xF000);	break;	}
			}
		}

		dstD[ii]= mme;
		srcL += tileN;
	}

	// attach metamap
	dst->width= 2;
	dst->height= dstN;
	free(dst->data);
	dst->data= (BYTE*)dstD;

	// create metatile record
	int mtsS= 2*mm*tileN;
	RECORD *mts= (RECORD*)malloc(sizeof(RECORD));

	mts->width= 2;
	mts->height= mtsS/2;
	mts->data= (BYTE*)malloc(mtsS);
	// PONDER: now why am I copying this again?
	memcpy(mts->data, mtsD, mtsS);		
	free(mtsD);

	return mts;
}

// EOF
