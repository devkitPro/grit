//
//! \file grit_core.cpp
//!   Core grit routines
//! \date 20050814 - 20070227
//! \author cearn
//
// === NOTES === 
// * 20070227, jv: logging functions.
// * 20061010, jv: '-fa' now uses src for the sym-name if none is given.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

//#include <sys/param.h>

#include <cldib_core.h>
#include "grit.h"


// --------------------------------------------------------------------
// CONSTANTS
// --------------------------------------------------------------------


static const char __grit_app_string[]=
"\tExported by Cearn's GBA Image Transmogrifier\n"
"\t( http://www.coranac.com )";


const char *cFileTypes[4]= {"c", "s", "bin", "gbfs" /*, "o"*/};
const char *cAffix[5]= { "Pal", "Tiles", "Bitmap", "Map", "MetaMap" };
const char *cTypes[3]= { "u8", "u16", "u32" };
const char *cCprs[4]= { "not", "lz77", "huf", "rle" };


// --------------------------------------------------------------------
// GLOBALS
// --------------------------------------------------------------------


//! Application string pointer.
/*!
	\c grit_preface() allows room for a string specific to the 
	application. Alias this pointer to that string for your own 
	messages. Multi-lined strings are safe; a comment will be placed 
	on every new line.
*/
const char *grit_app_string= __grit_app_string;


// --------------------------------------------------------------------
// FUNCTIONS
// --------------------------------------------------------------------

//! GRIT_REC 'Constructor'.
GRIT_REC *grit_alloc()
{
	GRIT_REC *gr= (GRIT_REC*)malloc(sizeof(GRIT_REC));
	if(gr == NULL)
		return NULL;

	GRIT_SHARED *grs= grs_alloc();
	if(grs == NULL)
	{	free(gr);	return NULL;	}

	memset(gr, 0, sizeof(GRIT_REC));

	gr->shared= grs;

	return gr;
}

//! GRIT_REC 'Destructor'.
void grit_free(GRIT_REC *gr)
{
	if(gr == NULL)
		return;

	grs_free(gr->shared);
	grit_clear(gr);
	free(gr);
}


//! Initializes \a gr to default values.
void grit_init(GRIT_REC *gr)
{
	if(gr == NULL)
		return;

	// file/var info
	gr->file_flags= GRIT_FILE_S | GRIT_FILE_H;

	// palette
	gr->pal_flags= GRIT_INCL | GRIT_U16;
	gr->pal_start= 0;
	gr->pal_end= 256;
	// image
	gr->img_flags= GRIT_INCL | GRIT_U32 | GRIT_IMG_TILE;
	gr->img_ofs= 0;
	gr->img_bpp= 8;
	// area (tl inclusive, rb exclusive)
	gr->area_left=  0;
	gr->area_top=   0;
	gr->area_right= 8;	// mod later
	gr->area_bottom=8;	// mod later
	// map
	gr->map_flags= GRIT_EXCL | GRIT_U16 | GRIT_MAP_FLAT | GRIT_RDX_REG8;
	gr->map_ofs= 0;
	gr->meta_width= 1;
	gr->meta_height= 1;

	// NOTE: do NOT init shared here!
}


//! Frees allocations and zeroes everything (except shared).
void grit_clear(GRIT_REC *gr)
{
	if(gr == NULL)
		return;

	free(gr->src_path);
	dib_free(gr->src_dib);

	free(gr->dst_path);
	free(gr->sym_name);

	// internals
	dib_free(gr->_dib);
	free(gr->_pal_rec.data);
	free(gr->_img_rec.data);
	free(gr->_map_rec.data);
	free(gr->_meta_rec.data);

	GRIT_SHARED *grs= gr->shared;
	memset(gr, 0, sizeof(GRIT_REC));
	gr->shared= grs;
}


//! Initialize palette and gfx-related functions from the DIB.
BOOL grit_init_from_dib(GRIT_REC *gr)
{
	if(gr == NULL)
		return FALSE;

	CLDIB *dib= gr->src_dib;
	if(dib == NULL)
	{
		lprintf(LOG_ERROR, "No bitmap to initialize GRIT_REC from.\n");
		return FALSE;
	}

 	int nclrs = dib_get_nclrs(dib);
	gr->pal_end= ( nclrs ? nclrs : 256 );

	if(dib_get_bpp(dib) > 8)
		gr->img_bpp= 16;
	else
		gr->img_bpp= dib_get_bpp(dib);

	gr->area_right= dib_get_width(dib);
	gr->area_bottom=dib_get_height(dib);

	return TRUE;
}

//! Main grit hub.
/*! After filling in a grit-rec, pass it to this function and you're 
	pretty much done.
	\param gr A filled (but not necessarily validated) grit-rec.
	\return \c TRUE for success, \c FALSE for failure
	\todo Create a set of individual error messages for the various 
	  stages.
*/
BOOL grit_run(GRIT_REC *gr)
{
	time_t aclock;
	struct tm *newtime;
	char str[96];

	time( &aclock );
	newtime = localtime( &aclock );
	strftime(str, 96, "Started run at: %Y-%m-%d, %H:%M:%S\n", newtime);

	lprintf(LOG_STATUS, str);

	if(grit_validate(gr) == FALSE)
		return FALSE;
		
	if(grit_prep(gr) == FALSE)
		return FALSE;

	if(~gr->file_flags & GRIT_FILE_NO_OUT)
		if(grit_export(gr) == FALSE)
			return FALSE;

	lprintf(LOG_STATUS, "Run completed :).\n");
	return TRUE;
}


void grit_dump(GRIT_REC *gr, FILE *fp)
{
	fprintf(fp, "%12s %s\n", "src path", 
		isempty(gr->src_path) ? "--" : gr->src_path);
	fprintf(fp, "%12s %s\n", "dst path",  
		isempty(gr->dst_path)  ? "--" : gr->dst_path); 
	fprintf(fp, "%12s %s\n", "sym name", 
		isempty(gr->sym_name) ? "--" : gr->sym_name);
	fprintf(fp, "%12s %08x\n", "file opt", gr->file_flags);

	fputs("--- pal ---\n", fp);
	fprintf(fp, "%12s %08x\n", "pal opt", gr->pal_flags);
	fprintf(fp, "%12s [%d, %d>\n", "pal range", gr->pal_start, gr->pal_end);

	fputs("--- image ---\n", fp);
	fprintf(fp, "%12s %08x\n", "img opt", gr->img_flags);
	fprintf(fp, "%12s %d\n", "img bpp", gr->img_bpp);
	fprintf(fp, "%12s %d\n", "img ofs", gr->img_ofs);
	fprintf(fp, "%12s (%d,%d)-(%d,%d)\n", "img range", 
		gr->area_left, gr->area_top, gr->area_right, gr->area_bottom);

	fputs("--- map ---\n", fp);
	fprintf(fp, "%12s %08x\n", "map opt", gr->map_flags);
	fprintf(fp, "%12s %d\n", "map ofs", gr->map_ofs);
	fprintf(fp, "%12s [%d, %d]\n", "meta size", 
		gr->meta_width, gr->meta_height);
}

// {src} >{>} {dst-dir}/{dst-title}.{dst-ext} {+ .h}
// { {sym}Pal : {cprs}, u{n}, [{ps}-{pe}>}
// { {sym}{Tiles/Bitmap} : {cprs}, u{n}, {n} bpp, +{ofs} }
// { {sym}Map : {cprs}, u{n}, -{tfp}, {layout}, +{ofs}
// Area : ({al},{at})-({ar},{ab}){ Meta : {w},{h} }
void grit_dump_short(GRIT_REC *gr, FILE *fp, const char *pre)
{
	if(pre==NULL)
		pre= "";

	int val;

	fputs(pre, fp);
	fprintf(fp, "%s >%s %s.%s %s\n", 
		gr->src_path, (gr->file_flags&GRIT_FILE_CAT ? ">":""), 
		gr->dst_path, (gr->file_flags&GRIT_FILE_H ? "+.h":"") );

	val= gr->pal_flags;
	if(val&GRIT_INCL)
	{
		fputs(pre, fp);
		fprintf(fp, "%s%s : %s cprs, %s, [%d,%d>\n", 
			gr->sym_name, cAffix[E_PAL],
			cCprs[BF_GET(val,GRIT_CPRS)], cTypes[BF_GET(val,GRIT_U)], 
			gr->pal_start, gr->pal_end);
	}
	val=gr->img_flags;
	if(val&GRIT_INCL)
	{
		fputs(pre, fp);
		fprintf(fp, "%s%s : %s cprs, %s, %dbpp, +%d\n", 
			gr->sym_name, cAffix[(val&GRIT_IMG_BMP ? E_BM : E_TILE)],
			cCprs[BF_GET(val,GRIT_CPRS)], cTypes[BF_GET(val,GRIT_U)], 
			gr->img_bpp, gr->img_ofs);
	}
	val= gr->map_flags;
	if(val&GRIT_INCL)
	{
		fputs(pre, fp);
		fprintf(fp, "%s%s : %s cprs, %s, ", 
			gr->sym_name, cAffix[E_MAP],
			cCprs[BF_GET(val,GRIT_CPRS)], cTypes[BF_GET(val,GRIT_U)]);
		if(gr->map_flags & GRIT_RDX_ON)
		{
			fputs("-t", fp);
			if(val & GRIT_RDX_FLIP)
				fputs("f", fp);
			if(val & GRIT_RDX_PAL)
				fputs("p", fp);
			fputs(", ", fp);
		}
		const char *layouts[]={ "reg flat", "reg sbb", "affine" };
		fprintf(fp, "%s, +%d\n",
			layouts[BF_GET(val,GRIT_MAP_LAY)], gr->map_ofs);
	}
	fputs(pre, fp);
	fprintf(fp, "Area : (%d,%d)-(%d,%d)    ", 
		gr->area_left, gr->area_top, gr->area_right, gr->area_bottom);
	fprintf(fp, "Meta: %d,%d\n", gr->meta_width, gr->meta_height);
}

// --- Validation -----------------------------------------------------

//! Validate parameters.
/*!	Makes sure all the settings in \a gr are valid. For example, 
	tilemap and bitmap mode are mutually exclusive; bpp must be 
	1, 2, 4, 8, or 16 and more of such kind. Some things are fatal
	(those two are), but some can be accounted for like reversed 
	pal start and end indices and non-aligned areas.
	\todo Add path and var-name validation.
*/
BOOL grit_validate(GRIT_REC *gr)
{
	lprintf(LOG_STATUS, 
"Validatating gr.\n");


	int tmp;
	u32 flags;
	char str[MAXPATHLEN];

	// source dib MUST be loaded already!
	if(gr->src_dib == NULL)
	{
		lprintf(LOG_ERROR, 
"  No input bitmap. Validation failed.\n");
		return FALSE;
	}

	// --- paths & names ---
	if(~gr->file_flags & GRIT_FILE_NO_OUT)
	{
		// Must have either src or dst paths
		if( isempty(gr->src_path) && isempty(gr->dst_path) )
		{
			lprintf(LOG_ERROR, 
"  No input or output paths. Validation failed.\n");
		}

		if(gr->src_path == NULL)
		{
			strrepl(&gr->src_path, "");
			lprintf(LOG_WARNING, 
"  No explicit src path.\n");
		}
		else
			path_fix_sep(gr->src_path);

		// No dst path? Get from source
		if(isempty(gr->dst_path))
		{
			strrepl(&gr->dst_path, path_get_name(gr->src_path));
			lprintf(LOG_WARNING, 
"  No explicit dst path. Borrowing from src path.\n");
		}

		// Fix extension
		path_repl_ext(str, gr->dst_path, 
			cFileTypes[BF_GET(gr->file_flags, GRIT_FTYPE)], MAXPATHLEN);
		strrepl(&gr->dst_path, str);
		path_fix_sep(gr->dst_path);

		// If no symbol name:
		// - append mode : from src
		// - create mode : from dst 
		if(isempty(gr->sym_name))
		{
			if(gr->file_flags & GRIT_FILE_CAT)			
			{
				path_get_title(str, gr->src_path, MAXPATHLEN);
				strrepl(&gr->sym_name, str);
		
				lprintf(LOG_WARNING, 
"  No explicit symbol name. In append mode, so using src title.\n");
			}
			else
			{
				path_get_title(str, gr->dst_path, MAXPATHLEN);
				strrepl(&gr->sym_name, str);

				lprintf(LOG_WARNING, 
"  No explicit symbol name. In overwrite mode, so using dst title.\n");
			}
		}
		str_fix_ident(gr->sym_name, gr->sym_name, MAXPATHLEN);
	}

	// --- options ---

	// bpp must be 2^n; truecolor WILL be bitmaps
	int bpp= gr->img_bpp;
	switch(bpp)
	{
	case 0:
		gr->img_bpp=8;
	case 1: case 2: case 4: case 8:
		break;
	case 16: case 24: case 32:
		gr->img_bpp= 16;
		gr->img_flags |= GRIT_IMG_BMP;	// set to bitmap

		gr->map_flags= 0;				// no map 
		gr->pal_flags &= ~GRIT_INCL;	// no pal either
		break;
	default:
		lprintf(LOG_ERROR, "  Bad bpp (%d).\n", bpp);
		return FALSE;
	}


	// bitmap - tilemap mismatch
	if((gr->img_flags&GRIT_IMG_BMP) && gr->map_flags&GRIT_INCL)
	{
		lprintf(LOG_ERROR,
"  Option mismatch: Can't map true-color bitmaps.\n");
		return FALSE;
	}

	// binary cannot be appended
	flags= gr->file_flags&GRIT_FTYPE_MASK;
	if(flags==GRIT_FILE_BIN)
	{
		gr->file_flags &= ~GRIT_FILE_CAT;
		lprintf(LOG_WARNING, 
"  Can't append to binary files. Switching to override mode.");
	}

	// --- ranges ---

	// palette
	if(gr->pal_flags & GRIT_INCL)
	{
		if(gr->pal_start > gr->pal_end)
		{
			lprintf(LOG_WARNING, 
"  Palette: start (%d) > end (%d): swapping.\n", gr->pal_start, gr->pal_end);

			SWAP3(gr->pal_start, gr->pal_end, tmp);
		}

		if(gr->pal_start<0)
		{
			lprintf(LOG_WARNING, 
"  Palette: start (%d) < 0. Clamping to 0.\n", gr->pal_start);
			gr->pal_start= 0;
		}

		int nclrs= dib_get_nclrs(gr->src_dib);
		if(nclrs != 0 && gr->pal_end > nclrs)
		{
			lprintf(LOG_WARNING, 
"  Palette: end (%d) > #colors (%d). Clamping to %d.\n", nclrs);
			gr->pal_end= nclrs;
		}
	}
	// area size
	if((gr->img_flags&GRIT_INCL) || (gr->map_flags&GRIT_INCL))
	{
		int al, at, ar, ab;		// area
		int aw, ah;				// area size
		int blw, blh;			// block size

		al= gr->area_left;	at= gr->area_top;
		ar= gr->area_right;	ab= gr->area_bottom;
		// normalize
		if(al > ar)
		{
			lprintf(LOG_WARNING, 
"  Area: left (%d) > right (%d). Swapping.\n", gr->area_left, gr->area_right);

			SWAP3(al, ar, tmp);
		}

		if(at > ab)
		{
			lprintf(LOG_WARNING, 
"  Area: top (%d) > bottom (%d). Swapping.\n", gr->area_top, gr->area_bottom);

			SWAP3(at, ab, tmp);
		}

		if(ar == 0)
			ar= dib_get_width(gr->src_dib);
		if(ab == 0)
			ab= dib_get_height(gr->src_dib);

		aw= ar-al;
		ah= ab-at;
		
		// metatile size (in px)
		// area MUST fit at least one metatile
		// Tiles: either 
		// - one meta tile
		// - one screenblock if in SBB layout
		// PONDER: non-integral metatile/sbb ??
		if(~gr->img_flags & GRIT_IMG_BMP)
		{
			if(~gr->map_flags & GRIT_MAP_REG)
			{
				if(gr->meta_width <= 0)
					gr->meta_width= 1;
				blw= gr->meta_width*8;

				if(gr->meta_height <= 0)
					gr->meta_height= 1;
				blh= gr->meta_height*8;
			}
			else
				blw= blh= 256;
		}
		else	// Bitmap: no restrictions
			blw= blh= 1;

		// area must be multiple of metas (rounding up here)
		aw= (aw+blw-1)/blw * blw;
		ah= (ah+blh-1)/blh * blh;

		// PONDER: but what about the image size?
		//  *shrug* what about it?
		ar= al+aw;
		ab= at+ah;	

		gr->area_left= al;	gr->area_top= at;
		gr->area_right= ar;	gr->area_bottom= ab;
	}

	// PONDER: issue dump for status-log?

	lprintf(LOG_STATUS, "Validation succeeded.\n");
	return TRUE;
}

// EOF
