//
//! \file cldib_tmap.h
//!  Tilemap functionality
//! \date 20070414 - 20070414
//! \author cearn
// === NOTES ===
// * 20070414,jv: Tilemap stuff seems to be working. Need more testing for 
//   32bpp though.

#ifndef __CLDIB_TMAP_H__
#define __CLDIB_TMAP_H__

#include "cldib.h"


/*! \addtogroup grpTmap
/*!	\{	*/


// --------------------------------------------------------------------
// CONSTANTS
// --------------------------------------------------------------------


//! \name Map Entry flags
//\{
#define ME_HFLIP			( 1<<10)	//!< Horizontal flip flag
#define ME_VFLIP			( 2<<10)	//!< Vertical flip flag

#define ME_ID_MASK			0x03FF
#define ME_ID_SHIFT				 0
#define ME_FLIP_MASK		0x0C00
#define ME_FLIP_SHIFT			10
#define ME_PBANK_MASK		0xF000
#define ME_PBANK_SHIFT			12
//\}

//! \name TileMap flags
//\{
#define TMAP_TYPE_BITMAP	(0<< 1)	//!< Tiles are bitmaps
#define TMAP_TYPE_META		(1<< 1)	//!< Tiles are metatiles (N/A)
#define TMAP_FLIP			(1<<10)	//!< Allows flipped tiles.
#define TMAP_PALSWAP		(1<<11)	//!< Allows pal swapping (8bpp only)

#define TMAP_DFLT		(TMAP_TYPE_BITMAP|TMAP_FLIP)
#define TMAP_DFLT_4		(TMAP_TYPE_BITMAP|TMAP_FLIP|TMAP_PALSWAP)
//\}


// --------------------------------------------------------------------
// CLASSES
// --------------------------------------------------------------------


//! Generic map entry struct
typedef struct TMapEntry
{
	WORD index;				//!< Tile index
	union {
		WORD attr;			//!< Tile attributes (ME_FLAGS)
		struct {
			WORD tile	:10;
			WORD hflip	: 1;
			WORD vflip	: 1;
			WORD pal	: 4;
		};
	};
} TMapEntry;


//! Generic tilemap struct
typedef struct TTileMap
{
	int width;			//!< Map width.
	int height;			//!< Map Height.
	TMapEntry *data;	//!< Tile data.
	int tile_width;		//!< Tile width.
	int tile_height;	//!< Tile height.
	CLDIB *tiles;		//!< Tileset.
	DWORD flags;		//!< Tilemap flags.
} TTilemap;



/*!	\}	*/


// --------------------------------------------------------------------
// MACROS
// --------------------------------------------------------------------


// --------------------------------------------------------------------
// PROTOTYPES
// --------------------------------------------------------------------


TTileMap *tmap_alloc();
void tmap_free(TTileMap *tmap);
CLDIB *tmap_attach(TTileMap *tmap, CLDIB *dib);

void tmap_init(TTileMap *tmap, int mapW, int mapH, 
	int tileW, int tileH, DWORD flags);
BOOL tmap_init_from_dib(TTileMap *tmap, CLDIB *dib, int tileW, int tileH, 
	DWORD flags, CLDIB *extTiles);

CLDIB *tmap_render(TTileMap *tmap, const RECT *rect);


// --------------------------------------------------------------------
// INLINES
// --------------------------------------------------------------------


#endif // __CLDIB_TMAP_H__

// EOF
