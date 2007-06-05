//
//! \file cldib_tmap.cpp
//!  Tilemap functionality
//! \date 20070414 - 20070414
//! \author cearn
// === NOTES ===

#include "cldib.h"

// --------------------------------------------------------------------
// PROTOTYPES
// --------------------------------------------------------------------

/*!	\addtogroup	grpTmap	*/
/*!	\{	*/

int dib_get_pbank(CLDIB *dib);
int dib_set_pbank(CLDIB *dib, int pbank);
BOOL dib_tilecmp(CLDIB *dib, CLDIB *tileset, int tid, DWORD mask);
TMapEntry dib_find(CLDIB *dib, CLDIB *tileset, int tileN, DWORD flags);


/*!	\}	*/


// --------------------------------------------------------------------
// FUNCTIONS
// --------------------------------------------------------------------


// === Book-keeping ===

//! Allocates room for a tilemap.
TTileMap *tmap_alloc()
{
	TTileMap *tmap= (TTileMap*)malloc(sizeof(TTileMap));
	memset(tmap, 0, sizeof(TTileMap));
	return tmap;	
}


//! Frees the memory of a tilemap.
/*!	\note	Will remove both the map and tileset.
*/
void tmap_free(TTileMap *tmap)
{
	if(tmap == NULL)
		return;

	free(tmap->data);
	dib_free(tmap->tiles);

	free(tmap);
}

//! Attach (or detach) a tileset to/from a tilemap.
/*!	\return	Previously attached tileset.
*/
CLDIB *tmap_attach(TTileMap *tmap, CLDIB *dib)
{
	CLDIB *old= tmap->tiles;
	tmap->tiles= dib;
	return old;
}


//! Initializes a tilemap.
/*!	\param tmap	Tilemap to init. Previous data will be removed.
*	\param mapW		Map width.
*	\param mapH		Map height.
*	\param tileW	Tile width.
*	\param tileH	Tile height.
*	\param flags	Tilemap flags (TMAP_*).
*/
void tmap_init(TTileMap *tmap, int mapW, int mapH, 
	int tileW, int tileH, DWORD flags)
{
	free(tmap->data);
	dib_free(tmap->tiles);

	tmap->width= mapW;
	tmap->height= mapH;
	tmap->data= (TMapEntry*)malloc(mapW*mapH*sizeof(TMapEntry));

	tmap->tile_width= tileW;
	tmap->tile_height= tileH;
	tmap->tiles= NULL;
	tmap->flags= flags;
}


// === Operations ===



//! Create a tilemap with reduced tileset from a DIB.
/*!	\param tmap	Tilemap to init.
*	\param dib	DIB to init from. Must be 8 or 32bpp.
*	\param tileW	Tile width.
*	\param tileH	Tile height.
*	\param flags	Tilemap flags (reduction options and such).
*	\param extTiles	External tileset to use as a base for internal tileset.
*	\return	Success status.
*	\note	On bitdepth: Both bitmaps must be of the same bitdepth, 
*	  which must be either 8bpp or 32bpp.
*	\note	If \a dib's sizes aren't multiples of the tiles, the incomplete
*	  rows will be truncated.
*	\note	\a extTiles is considered a column of tiles, not a matrix. 
*	  only the first column will be considered.
*/
BOOL tmap_init_from_dib(TTileMap *tmap, CLDIB *dib, int tileW, int tileH, 
	DWORD flags, CLDIB *extTiles)
{
	//# Safety checks
	if(tmap==NULL || dib==NULL || tileW<1 || tileH<1)
		return FALSE;

	//# Set up new tilemap and tileset
	int dibW, dibH, dibB, dibP;
	dib_get_attr(dib, &dibW, &dibH, &dibB, &dibP);

	int mapW= dibW/tileW, mapH= dibH/tileH, mapN= mapW*mapH;

	if(mapW==0 || mapH==0)
		return FALSE;

	// Init new tileset
	int rdxN;
	CLDIB *rdx;

	if(extTiles)
	{
		if(dibB != dib_get_bpp(extTiles))
			return FALSE;

		rdxN= dib_get_height(extTiles)/tileH;
		rdx= dib_copy(extTiles, 0, 0, tileW, (mapN+rdxN)*tileH, FALSE);
	}
	else
	{
		rdxN= 1;					
		rdx= dib_alloc(tileW, (mapN+rdxN)*tileH, dibB, NULL);
		memset(dib_get_img(rdx), 0, dib_get_pitch(rdx)*tileH);
		dib_pal_cpy(rdx, dib);
	}

	if(rdx == NULL)
		return FALSE;

	TMapEntry *mapD= (TMapEntry*)malloc(mapW*mapH*sizeof(TMapEntry)), me;

	// Create temporary tile for comparisons
	CLDIB *tmpDib= dib_copy(dib, 0, 0, tileW, tileH, FALSE);
	int tmpP= dib_get_pitch(tmpDib);	
	BYTE *tmpD= dib_get_img(tmpDib);
	
	int iy, tx, ty;
	for(ty=0; ty<mapH; ty++)
	{
		for(tx=0; tx<mapW; tx++)
		{
			// Prep comparison DIB
			for(iy=0; iy<tileH; iy++)
				memcpy(&tmpD[iy*tmpP], 
					dib_get_img_at(dib, tx*tileW, ty*tileH+iy), tmpP);
			
			me= dib_find(tmpDib, rdx, rdxN, flags);

			// Not found? Add to tileset
			if(me.index >= rdxN)
			{
				memcpy(dib_get_img_at(rdx, 0, tileH*rdxN), tmpD,
					dib_get_size_img(tmpDib));
				rdxN++;
			}

			mapD[ty*mapW+tx]= me;
		}
	}

	// Shrink tileset
	dib_free(tmpDib);
	tmpDib= dib_copy(rdx, 0, 0, tileW, rdxN*tileH, FALSE);
	dib_free(rdx);

	// Attach map and tileset to tmap
	free(tmap->data);
	dib_free(tmap->tiles);

	tmap->width= mapW;
	tmap->height= mapH;
	tmap->data= mapD;
	tmap->tile_width= tileW;
	tmap->tile_height= tileH;
	tmap->tiles= tmpDib;
	tmap->flags= flags;

	return TRUE;
}

//! Render a tilemap to a DIB (8, 16, 24, 32).
/*!	Converts a tile map and its tileset to a full bitmap in the 
*	bitdepth of the tileset. Can also render a porttion of the map.
*	\param tmap	Tilemap to render.
*	\param rect	Portion of map to render. If NULL, it renders the whole thing.
*	\note	Assumes the tileset is a column.
*	\note	Bitdepth must me either 8 or 32 bpp.
*/
CLDIB *tmap_render(TTileMap *tmap, const RECT *rect)
{
	// Safety checks
	if(tmap == NULL || tmap->data == NULL || tmap->tiles == NULL)
		return NULL;

	CLDIB *tileDib= tmap->tiles;
	int tileW, tileH, tileB, tileP, tileS;
	dib_get_attr(tileDib, &tileW, NULL, &tileB, &tileP);

	// PONDER is this really necessary?
	if( tileB <8 )
		return NULL;
	if(tileW < tmap->tile_width)
		return NULL;

	tileW= tmap->tile_width, tileH= tmap->tile_height;
	tileS= tileP*tileH;

	int mapW= tmap->width, mapH= tmap->height, mapP= mapW;
	TMapEntry *mapD= tmap->data, me;

	int tx, ty, iy;

	// Portion render: change map W, H, D accordingly
	if(rect)
	{	
		tx= MAX(0, rect->left);
		ty= MAX(0, rect->top);
		mapD += ty*mapP + tx;
		mapW= MIN(mapW, rect->right)-tx;
		mapH= MIN(mapH, rect->bottom)-ty;						
	}

	// --- Create new dib ---
	int dibW= tileW*mapW, dibH= tileH*mapH, dibP;
	CLDIB *dib= dib_alloc(dibW, dibH, tileB, NULL);
	dib_pal_cpy(dib, tileDib);
	dibP= dib_get_pitch(dib);

	// Temp tile dib
	CLDIB *tmpDib= dib_copy(tileDib, 0, 0, tileW, tileH, FALSE);
	BYTE *tmpD= dib_get_img(tmpDib);

	for(ty=0; ty<mapH; ty++)
	{
		for(tx=0; tx<mapW; tx++)
		{
			me= mapD[ty*mapP+tx];
			
			// --- Flip and mask on temp dib ---
			memcpy(tmpD, dib_get_img_at(tileDib, 0, tileH*me.index), tileS);

			if(me.attr & ME_HFLIP)
				dib_hflip(tmpDib);
			if(me.attr & ME_VFLIP)
				dib_vflip(tmpDib);

			if(tileB == 8 && (tmap->flags & TMAP_PALSWAP))
				dib_set_pbank(tmpDib, BF_GET(me.attr, ME_PBANK));

			// Render temp dib
			BYTE *dibD= dib_get_img_at(dib, tx*tileW, ty*tileH);
			for(iy=0; iy<tileH; iy++)
				memcpy(&dibD[iy*dibP], &tmpD[iy*tileP], tileW*tileB/8);
		}
	}

	dib_free(tmpDib);
	return dib;
}


// === Utilities ===

//! Find the palette-bank for a DIB.
/*!	\note	For 8bpp only.
*/
int dib_get_pbank(CLDIB *dib)
{
	int dibW, dibH, dibB, dibP;
	dib_get_attr(dib, &dibW, &dibH, &dibB, &dibP);
	if(dibB != 8)
		return 0;

	int ix, iy, clrid;
	BYTE *dibD= dib_get_img(dib);

	for(iy=0; iy<dibH; iy++)
	{
		for(ix=0; ix<dibW; ix++)
		{
			clrid= dibD[iy*dibP+ix];
			if(clrid&0x0F)
				return (clrid>>4)&0x0F;
		}
	}
	return 0;
}


//! Find the palette-bank for a DIB.
/*!	\note	For 8bpp only.
*/
BOOL dib_set_pbank(CLDIB *dib, int pbank)
{
	int dibW, dibH, dibB, dibP;
	dib_get_attr(dib, &dibW, &dibH, &dibB, &dibP);
	if(dibB != 8)
		return FALSE;

	int ix, iy;
	BYTE *dibD= dib_get_img(dib);

	pbank <<= 4;

	for(iy=0; iy<dibH; iy++)
		for(ix=0; ix<dibW; ix++)
			dibD[iy*dibP+ix]= (dibD[iy*dibP+ix]&15) | pbank;

	return TRUE;
}


//! Compare \a dib to tile \a tid from \a tileset (8+)
/*!	\note	\a dib indicates tile attributes. If these don't match 
*	  with \a tileset, there will be trouble. YHBW.
*	\note	PONDER: Endian effects???
*/
BOOL dib_tilecmp(CLDIB *dib, CLDIB *tileset, int tid, DWORD mask)
{
	int tileW, tileH, tileB, tileP;
	dib_get_attr(dib, &tileW, &tileH, &tileB, &tileP);

	int ix, iy, ib, nb;	// x,y, byte indices, #bytes/pixel

	nb= tileB/8;
	tileW *= nb;

	BYTE *dibD= dib_get_img(dib), *dibL;
	BYTE *tileD= dib_get_img_at(tileset, 0, tid*tileH), *tileL;

	for(ib=0; ib<nb; ib++)
	{
		dibL= &dibD[ib];
		tileL= &tileD[ib];
		for(iy=0; iy<tileH; iy++)
		{
			for(ix=0; ix<tileW; ix += nb)
				if( (dibL[ix]&mask) != (tileL[ix]&mask) )
					return FALSE;
				
			dibL += tileP;
			tileL += tileP;
		}
		mask >>= 8;
	}
	return TRUE;
	
}


//! Find tile \a dib inside a tileset.
/*!	\param dib		Dib to compare.
*	\param tileset	Tileset to find \a dib in.
*	\param tileN	Number of used tiles in \a tileset.
*	\param mask		Color mask for pixels.
*	\return	Map entry with found information. If not found, the index 
*	  will be equal to \a tileN.
*	\note	\a dib represents a single tile. As such, it gives the 
*	  size and bitdepth of the tiles. If you want flipped tiled, 
*	  flip \a dib outside.
*/
TMapEntry dib_find(CLDIB *dib, CLDIB *tileset, int tileN, DWORD flags)
{
	int ii;
	int dibB= dib_get_bpp(dib);
	DWORD mask;
	TMapEntry me= { tileN, 0 };

	if( dibB == 8 && (flags &TMAP_PALSWAP) )
	{
		mask= 0x0F;
		BF_SET(me.attr, dib_get_pbank(dib), ME_PBANK);
	}
	else
		mask= 0xFFFFFFFF;
	

	// --- Straight ---
	for(ii=0; ii<tileN; ii++)
	{
		if(dib_tilecmp(dib, tileset, ii, mask))
		{
			me.index= ii;
			return me;
		}
	}

	// --- Flips, if requested ---
	// TODO: could be compressed a little.

	if(~flags & TMAP_FLIP)
		return me;

	dib= dib_clone(dib);	// Create temp copy for flipping

	do
	{
		// --- H-flip ---
		dib_hflip(dib);
		for(ii=0; ii<tileN; ii++)
			if(dib_tilecmp(dib, tileset, ii, mask))
				break;
		if(ii<tileN)
		{	me.attr |= ME_HFLIP;	break;	}

		// --- HV-flip ---
		dib_vflip(dib);
		
		for(ii=0; ii<tileN; ii++)
			if(dib_tilecmp(dib, tileset, ii, mask))
				break;
		if(ii<tileN)
		{	me.attr |= ME_HFLIP | ME_VFLIP;	break;	}

		// --- V-flip ---
		dib_hflip(dib);
		me.attr &= ~ME_HFLIP;
		for(ii=0; ii<tileN; ii++)
			if(dib_tilecmp(dib, tileset, ii, mask))
				break;
		if(ii<tileN)
		{	me.attr |= ME_VFLIP;	break;	}

	} while(0);
	
	dib_free(dib);
	me.index= ii;

	return me;
}


// EOF
