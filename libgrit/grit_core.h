//
//! \file grit_core.h
//!   grit core header file
//! \date 20050814 - 20070314
//! \author cearn
//
// === NOTES === 
// * 20070413,jv
//   IMPORTANT NOTICE: with the multi-file and shared stuff, 
//   the whole GRIT_REC thing is now thoroughly fucked up. It should
//   be refactored into a single-run struct (TGritEntry) and a 
//   global manager struct (TGrit). I cannot say when this will happen, 
//   only that it should and that many things will change because of it.
// * 20060727,jv: enum'ed GRIT and SE flags
// * 20060727,jv: added alpha stuff

#ifndef __GRIT_CORE_H__
#define __GRIT_CORE_H__

#include "cldib_core.h"


/*! \addtogroup grpGrit
*	\{
*/


// === TYPES ==========================================================

// bool _is_ a byte, although windows/h and freeimage.h disagree, *sigh*
// And they typedef the bugger too, so a simple #ifndef BOOL wouldn't 
// work :(
// Good thing these usually come in pairs
#if !defined(BOOL) && !defined(TRUE)
#error "Sorry dude, find a BOOL, TRUE and FALSE first."
#endif

typedef unsigned char  u8,  byte;
typedef unsigned short u16, hword, COLOR;
typedef unsigned int   u32, word;

typedef signed char  s8;
typedef signed short s16; 
typedef signed int   s32;


// --------------------------------------------------------------------
// CONSTANTS
// --------------------------------------------------------------------


// TODO: add tileset path and dib

enum eGRIT_FLAGS
{
// pal/img/map common opts (t=p,g,m) ---
	GRIT_EXCL		=		 0,	//!< Exclude from export. `-{t}!'
	GRIT_INCL		=	0x0001,	//!< Include in export. `-{t}'

	GRIT_U8			=		 0,	//!< Export as byte array. `-{t}u8'
	GRIT_U16		=	0x0002,	//!< Export as halfword array. `-{t}u16'
	GRIT_U32		=	0x0004,	//!< Export as word array. `-{t}u32'

	GRIT_U_MASK		=	0x0006,
	GRIT_U_SHIFT	=		 1,

	GRIT_CPRS_NONE	=		 0,	//!< No compression. `-{t}z!'
	GRIT_CPRS_LZ77	=	0x0010,	//!< LZ77 compression (LZ77UnCompVram compatible). `-{t}zl'
	GRIT_CPRS_HUFF	=	0x0020,	//!< 8bit Huffman compression (might be buggy). `-{t}zh'
	GRIT_CPRS_RLE	=	0x0030,	//!< 8bit RLE compression. `-{t}zr'
//	GRIT_CPRS_DIFF	=	0x0080,

	GRIT_CPRS_MASK	=	0x00F0,
	GRIT_CPRS_SHIFT	=		 4,

	// --- pal opts ---
	GRIT_PAL_TRANS	=	0x0400,	//!< Non-zero transparent palette entry `-pT' 
	GRIT_PAL_SHARED =	0x0800,	//!< Shared palette `-pS'

	// --- img opts ---
	GRIT_IMG_TILE	=		 0,	//!< 8x8 Tiled graphics `-gt'
	GRIT_IMG_BMP	=	0x0100,	//!< Bitmap graphics `-gb'
	GRIT_IMG_BMP_A	=	0x0200,	//!< Full alpha-bit ` -gb -gT!' 
	GRIT_IMG_TRANS	=	0x0400,	//!< Transparent color `-gb -gT {xxx}' 
	GRIT_IMG_SHARED =	0x0800,	//!< Shared Tiles

	// --- map opts ---
	// tile reduction
	// These can be combined: 
	// -mRtfp (or -mRfp, -mRftp, etc) means tiles, flip & pal reduction
	GRIT_RDX_NONE	=		 0,	//!< No tile reduction (not advised) -mR!'
	GRIT_RDX_ON		=	0x0100,	//!< Tile reduction on `-mR'
	GRIT_RDX_TILE	=		 0,	//!< Reduce for all tiles `-mRt'
//	GRIT_RDX_BLANK	=	0x0200,	//!< Reduce for blank tiles only `-mRb'
	GRIT_RDX_FLIP	=	0x0400,	//!< Reduce for flipped tiles `-mRf'
	GRIT_RDX_PAL	=	0x0800,	//!< Reduce for palette-swapped tiles `-mRp'
	GRIT_RDX_AFF	=	0x0100,	//!< Recommended rdx flags for affine bgs  `-mRa' (= -mRt)
	GRIT_RDX_REG4	=	0x0D00,	//!< Recommended rdx flags for 4bpp reg bgs `-mR4' (= -mRtfp)
	GRIT_RDX_REG8	=	0x0500,	//!< Recommended rdx flags for 8bpp reg bgs `-mR8' (= -mRtf)

	GRIT_META_PAL	=	0x1000,	//!<

	GRIT_RDX_MASK	=	0x3F00,
	GRIT_RDX_SHIFT	=		 8,

	// layout
	GRIT_MAP_FLAT		=		 0,	//!< Flat regular tilemap layout -mLf'
	GRIT_MAP_REG		=	0x4000,	//!< Screenblocked regular tilemap layout -mLr'
	GRIT_MAP_AFF		=	0x8000,	//!< Affine tilemap layout -mLa'

	GRIT_MAP_LAY_MASK	=	0xC000,
	GRIT_MAP_LAY_SHIFT	=		14,

//	GRIT_MAP_COLS		= 0x010000,
//	GRIT_MAP_EXTTILE	= 0x020000,	//!< TODO: Use an external tileset

	// --- file/var opts ---
	// file types
	GRIT_FILE_C		=		 0,	//<! Output in C arrays `-fc'
	GRIT_FILE_S		=	0x0001,	//<! Output in GAS source arrays `-fs'
	GRIT_FILE_BIN	=	0x0002,	//<! Output in raw binary `-fb'
	GRIT_FILE_GBFS	=	0x0003,	//<! Output in GBFS archive `-fg'
//	GRIT_FILE_O		=	0x0004,

	GRIT_FTYPE_MASK	=	0x000F,
	GRIT_FTYPE_SHIFT=		 0,
	// misc
	GRIT_FILE_H		=	0x0010,	//!< Create header file `-fh'
	GRIT_FILE_CAT	=	0x0020,	//!< Append for existing file `-fa'

	GRIT_FILE_NO_IN	=	0x0100,	//!< No input from files
	GRIT_FILE_NO_OUT=	0x0200,	//!< No output to files
};

// --- offset flags ---
#define OFS_BASE0   (1<<31)

// --- GBA constants ---
// screen map flags
enum eSE_FLAGS
{
	SE_HFLIP		=	0x0400,	//!< Horizontal flip flag
	SE_VFLIP		=	0x0800,	//!< Vertical flip flag

	SE_ID_MASK		=	0x03FF,
	SE_ID_SHIFT		=		 0,
	SE_FLIP_MASK	=	0x0C00,
	SE_FLIP_SHIFT	=		10,
	SE_PBANK_MASK	=	0xF000,
	SE_PBANK_SHIFT	=		12,
};

#define GBA_RED_MASK	0x001F
#define GBA_RED_SHIFT		 0
#define GBA_GREEN_MASK	0x03E0
#define GBA_GREEN_SHIFT		 5
#define GBA_BLUE_MASK	0x7C00
#define GBA_BLUE_SHIFT		10

#define NDS_ALPHA		0x8000


enum eTypes
{	E_U8=1, E_U16=2, E_U32=4	};

enum eAffix
{	E_PAL=0, E_TILE=1, E_BM=2, E_MAP=3, E_META=4	};

extern const char *cFileTypes[4];
extern const char *cAffix[5];
extern const char *cTypes[3];
extern const char *cCprs[4];


// --------------------------------------------------------------------
// CLASSES
// --------------------------------------------------------------------


//! Struct for shared information
typedef struct GRIT_SHARED
{
	int		 log_level;	//!< Git logging level
	char	*path;		//!< Path to external path (can be NULL)
	char	*sym_name;	//!< Shared symbol name (unsused)
	int		 img_bpp;	//!< Bitdepth for shared image (unused)
	CLDIB	*dib;		//!< External tileset DIB (can be NULL)
	RECORD	 pal_rec;	//!< Shared palette (unused)
} GRIT_SHARED;


//! Basic grit struct
typedef struct GRIT_REC 
{
// public:

// Source stuff
	char	*src_path;		//!< Path to source bitmap
	CLDIB	*src_dib;		//!< Source bitmap
// file/var info
	u32		 file_flags;	//!< File flags
	char	*dst_path;		//!< Output path directory
	char	*sym_name;		//!< Output symbol name
// Area ( [l,r>, [t,r> )
	int		 area_left;		//!< Export rect, left 
	int		 area_top;		//!< Export rect, top 
	int		 area_right;	//!< Export rect, right 
	int		 area_bottom;	//!< Export rect, bottom 
// Image
	u32		 img_flags;		//!< Graphics flags
	u32		 img_ofs;		//!< pixel offset for packing
	int		 img_bpp;		//!< Output bitdepth
	RGBQUAD	 img_trans;		//!< Transparent color
// Map
	u32		 map_flags;		//!< Tilemap flags
	u32		 map_ofs;		//!< Map-entry offset
	int		 meta_width;	//!< Meta-tile/object width (in tiles)
	int		 meta_height;	//!< Meta-tile/object height (in tiles)
// Palette ( [s,e> )
	u32		 pal_flags;		//!< Palette flags
	int		 pal_start;		//!< First palette entry to export
	int		 pal_end;		//!< Final palette entry to export (exclusive)
	u32		 pal_trans;		//!< Transparent palette entry
	//char *tile_file;
// Shared information
	GRIT_SHARED *shared;
// Private: keep the f#^$k off
	CLDIB	*_dib;		//!< Internal work bitmap
	RECORD	 _img_rec;	//!< Output graphics data
	RECORD	 _map_rec;	//!< Output tilemap data
	RECORD	 _meta_rec;	//!< Output metatile data
	RECORD	 _pal_rec;	//!< Output palette data


} GRIT_REC;


// --------------------------------------------------------------------
// GLOBALS
// --------------------------------------------------------------------


extern const char *grit_app_string;


// --------------------------------------------------------------------
// PROTOTYPES
// --------------------------------------------------------------------

// === GRIT_REC ===

//BOOL grit_parse(GRIT_REC *gr, int argc, char **argv);	// parse cmdline

GRIT_REC *grit_alloc();					// alloc gr
void grit_free(GRIT_REC *gr);			// de-alloc gr

void grit_init(GRIT_REC *gr);			// set members to default values
BOOL grit_init_from_dib(GRIT_REC *gr);	// extra inits from src_dib parameters
void grit_clear(GRIT_REC *gr);			// clears internal allocations

BOOL grit_run(GRIT_REC *gr);


BOOL grit_validate(GRIT_REC *gr);	// validate input
BOOL grit_prep(GRIT_REC *gr);		// prepare data (conv, cprs, etc)
BOOL grit_export(GRIT_REC *gr);		// export data

BOOL grit_compress(RECORD *dst, const RECORD *src, u32 flags);


// void grit_dump(GRIT_REC *gr, FILE *fp);

// === GRIT_SHARED ===

GRIT_SHARED *grs_alloc();
void grs_free(GRIT_SHARED *grs);
void grs_clear(GRIT_SHARED *grs);
void grs_run(GRIT_REC *grs, GRIT_REC *gr_base);


// --------------------------------------------------------------------
// MACROS
// --------------------------------------------------------------------


//! Create a GBA BGR color
#define GBA_RGB16(r, g, b)	( (r) | ((g)<<5) | ((b)<<10) )

#define GRIT_CHUNK(gr, y)  ( 1<<BF_GET(gr->y##_flags, GRIT_U) )
#define GRIT_IS_BMP(gr)		( gr->img_flags&GRIT_IMG_BMP )

/*!	\}	*/

// --------------------------------------------------------------------
// INLINES
// --------------------------------------------------------------------


#endif // __GRIT_CORE_H__




// EOF
