//
//! \file grit_main.cpp
//!   Entry file for cmd-line grit.
//! \date 20050913 - 20080211
//! \author cearn
//
/* === NOTES ===
  * 20080223,jv: 
    - put version and completion indicators under LOG_STATUS.
	- error codes for main() ... but they still need work.
  * 20080211,jv:
	Added:
	-gS : shared graphics (without the use of an external file like -fx)
	-pS : shared palette
	-O : shared output file
	-S : shared symbol base
	-Z0 (and related) No compression, but with cprs-like header.
	-ftr : grf file : RIFFed binary data.
  * 20070331, jv: 
	- Fixed directory options for -fx.
	- Added '-ft!', '-We', '-Ww', '-Ws' options
	- changes default output to asm.
  * 20060729,jv: added vector-based grit_get_opts; I may mod 
	other functions to use strvec for subroutines later too.
  * 20060729.jv: PONDER: now that I am using STL, would this be a 
	good time to go for strings iso char*s as well?
*/

// PONDER, are these really necessary?
#ifndef _MSC_VER
#include <sys/param.h>
#include <unistd.h>
#include <strings.h>
#endif	// _MSC_VER

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <vector>

#include <cldib.h>
#include <grit.h>
#include <FreeImage.h>

#include "cli.h"
#include "fi.h"

void grit_dump(GritRec *gr, FILE *fp);
void grit_dump_short(GritRec *gr, FILE *fp, const char *pre);

// typedef std::vector<char*> strvec;


// --------------------------------------------------------------------
// CONSTANTS
// --------------------------------------------------------------------

#ifndef GRIT_VERSION
#define GRIT_VERSION	"0.8"
#endif

#ifndef GRIT_BUILD
#define GRIT_BUILD		"20080304"
#endif

const char cVersion[]= "grit v" GRIT_VERSION;

const char help_text[]= 
"GRIT: GBA Raster Image Transmogrifier. (v" GRIT_VERSION ", " GRIT_BUILD ")\n"
"  Converts bitmap files into something the GBA can use.\n"
"usage: grit srcfile(s) [opts]\n\n"
"--- Palette options (base: \"-p\") ---\n"
"-p | -p!       Include or exclude pal data [inc]\n"
"-pu(8|16|32)   Pal data-type: u8, u16 , u32 [u16]\n"
"-pz[!lhr0]     Pal compression: off, lz77, huff, RLE, off+header [off]\n"
"-ps{n}         Pal range start [0]\n"
"-pe{n}         Pal range end (exclusive) [pal size]\n"
"-pn{n}         Pal count [pal size]. Overrides -pe\n"
"-pS			NEW: shared palette\n"
"-pT{n}         Transparent palette index; swaps with index 0 [0]\n"
//"-p[s:e]        Pal range: [s, e> (e exclusive)\n"
//"-p[s+n]        Pal range: [s,s+n>\n"
//"-p[n]          Pal range: [0,n> (def=all)\n"
"--- Graphics options (base: \"-g\") ---\n"
"-g | -g!       Include  or exclude gfx data [inc]\n"
"-gu(8|16|32)   Gfx data type: u8, u16, u32 [u32]\n"
"-gz[!lhr0]     Gfx compression: off, lz77, huff, RLE, off+header [off]\n"
"-ga{n}         Gfx pixel offset (non-zero pixels) [0]\n"
//"-gA{n}         Gfx pixel offset n (def=0) (all pixels) [0]\n"
"-gb | -gt      Gfx format, bitmap or tile [tile]\n"
"-gB{n}         Gfx bit depth (1, 2, 4, 8, 16) [img bpp]\n"
"-gS			NEW: Shared graphics\n"
"-gT{n}         Transparent color; rrggbb hex or 16bit BGR hex [FF00FF]\n"
//"-g[l:r,t:b]    Area: x in [l,r>, y in [t,b>\n"
//"-g[l+w,t+h]    Area: x in [l,l+w>, y in [t,t+h>\n"
//"-g[w,h]        Area: x in [0,w>, y in [0,h> (def=full bitmap)\n"
"-al{n}         Area left [0]\n"
"-ar{n}         Area right (exclusive) [img width]\n"
"-aw{n}         Area width [img width]. Overrides -ar\n"
"-at{n}         Area top [0]\n"
"-ab{n}         Area bottom (exclusive) [img height]\n"
"-ah{n}         Area height [img height]. Overrides -ab\n"
"--- Map options (base: \"-m\") ---\n"
"-m | -m!       Include or exclude map data [exc]\n"
"-mu(8|16|32)   Map data type: u8, u16, u32 [u16]\n"
"-mz[!lhr0]     Map compression: off, lz77, huff, RLE, off+header [off]\n"
"-ma{n}         Map tile offset n (non-zero entries) [0]\n"
//"-mA{n}         Map tile offset n (all entries) [0]\n"
"-mR{t,p,f}     Tile reduction: (t)iles, (p)al, (f)lipped \n"
"                 options can be combined [-mRtpf]\n"
"-mR[48a]       Common tile reduction combos: reg 4bpp (-mRtpf), \n"
"                 reg 8bpp (-mRtf), affine (-mRt), respectively\n"
"-mR!           No tile reduction (not advised)\n"
"-mL[fsa]       Map layout: reg flat, reg sbb, affine [reg flat]\n"
"--- Meta/Obj options (base: \"-M\") ---\n"
//"-M | -M!       Include or exclude (def) metamap data\n"
//"-M[w,h]        Metatile / obj sizes (in tiles!)\n"
"-Mh{n}         Metatile height (in tiles!) [1]\n"
"-Mw{n}         Metatile width (in tiles!) [1]\n"
"-MRp           Metatile reduction (pal only) [none]\n"
"--- File / var options ---\n"
"-ft[!csbgr]    File type (no output, C, GNU asm, bin, gbfs, grf) [.s]\n"
"-fr            NEW: Enable GRF-format for .c or .s\n"
"-fa            File append\n"
"-fh | -fh!     Create header or not [create header]\n"
"-ff{name}      Additional options read from flag file [dst-name.grit]\n"
"-fx{name}      External tileset file\n"
"-o{name}       Destination filename [based on source]\n"
"-s{name}       Symbol base name [based from dst]\n"
"-O{name}		NEW: Destination file for shared data\n"
"-S{name}		NEW: Symbol base name for shared data\n"
"--- Misc ---\n"
"-q             Quiet mode; no report at the end\n"
"-U(8|16|32)    All data type: u8, u16, u32\n"
"-W{n}          Warning/log level 1, 2 or 3 [1]\n"
"-Z[!lhr0]      All compression: off, lz77, huff, RLE, off+header [off]\n"
"\nNew options: -fr, -ftr, -gS, -O, -pS, -S, -Z0 (et al)\n";


// --------------------------------------------------------------------
// PROTOTYPES
// --------------------------------------------------------------------

bool grit_parse(GritRec *gr, const strvec &opts);

int grit_parse_cprs(const char *key, const strvec &opts);
bool grit_parse_pal(GritRec *gr, const strvec &opts);
bool grit_parse_gfx(GritRec *gr, const strvec &opts);
bool grit_parse_map(GritRec *gr, const strvec &opts);
bool grit_parse_file(GritRec *gr, const strvec &opts);
int grit_parse_log(GritRec *gr, const strvec &opts);

bool grit_load_ext_tiles(GritRec *gr);
bool grit_save_ext_tiles(GritRec *gr);

void gather_opts(strvec &opts, int argc, char **argv);
bool prep_run(GritRec *gr, const char *fpath);

int run_individual(GritRec *gr, const strvec &opts, const strvec &fpaths);
int run_shared(GritRec *gr, const strvec &opts, const strvec &fpaths);
int run_main(int argc, char **argv);


// --------------------------------------------------------------------
// FUNCTIONS
// --------------------------------------------------------------------


//! Searches for compression options
/*!
	\param key	Base compression options (-Z, -gz, etc)
	\return	GRIT_CPRS_foo flag, or -1 if no sub-flag found.
*/
int grit_parse_cprs(const char *key, const strvec &opts)
{
	// compression
	char *str= CLI_STR(key, "");
	switch(*str)
	{
	case 'h':	return GRIT_CPRS_HUFF;
	case 'l':	return GRIT_CPRS_LZ77;
	case 'r':	return GRIT_CPRS_RLE;
	case '!':	return GRIT_CPRS_OFF;
	case '0':	return GRIT_CPRS_HEADER;
	default:
		return -1;		
	}
}

//! Searches for palette options
bool grit_parse_pal(GritRec *gr, const strvec &opts)
{
	int val;
	if( CLI_BOOL("-p!") == false)
	{
		gr->palProcMode= GRIT_EXPORT;

		gr->palDataType= CLI_INT("-pu", 16)>>4;
		if( (val= grit_parse_cprs("-pz", opts)) != -1 )
			gr->palCompression= val;

		// Range
		gr->palStart= CLI_INT("-ps", 0);

		if( (val= CLI_INT("-pn", -1)) >= 0)
			gr->palEnd= gr->palStart + val;
		else if( (val= CLI_INT("-pe", -1)) >= 0)
			gr->palEnd= val;
	}
	else
		gr->palProcMode= GRIT_EXCLUDE;

	// Transparent pal-id
	if( (val= CLI_INT("-pT", -1)) >= 0)
	{
		gr->palHasAlpha= true;
		gr->palAlphaId= val;
	}

	// Shared palette
	if(CLI_BOOL("-pS"))
		gr->palIsShared= true;

	return true;
}

//! Searches for graphics options
bool grit_parse_gfx(GritRec *gr, const strvec &opts)
{
	int val;
	char *pstr;

	if(CLI_BOOL("-g!") == false)
	{
		gr->gfxProcMode= GRIT_EXPORT;

		gr->gfxDataType= CLI_INT("-gu", 32)>>4;
		if( (val= grit_parse_cprs("-gz", opts)) != -1 )
			gr->gfxCompression= val;

		// pixel offset
		gr->gfxOffset= CLI_INT("-ga", 0);
	}
	else
		//# PONDER: can img really be excluded from processing?
		gr->gfxProcMode= GRIT_EXCLUDE;

	//# PONDER: this should probably be replaced by other kinds of formats.
	//# -gf(t|b|ba|tx\d) ?

	// Tile or bitmap format
	if(CLI_BOOL("-gt"))
		gr->gfxMode= GRIT_GFX_TILE;
	else if(CLI_BOOL("-gb"))
		gr->gfxMode= GRIT_GFX_BMP;

	// Bitdepth override
	if( (val= CLI_INT("-gB", 0)) != 0)
		gr->gfxBpp= val;

	// Transparency
	if( CLI_BOOL("-gT") )
	{
		// String can be (0x?)[0-9A-Fa-f]{4,6}
		// <5 hexits means 16bit BGR color
		// >4 means rrggbb color
		pstr= CLI_STR("-gT", "FF00FF");
		if(pstr[0] != '!')
		{
			gr->gfxAlphaColor= str2rgb(pstr);
			gr->gfxHasAlpha= true;
		}
		else
			gr->gfxMode= GRIT_GFX_BMP_A;
	}

	// Shared graphics (internal only)
	if(CLI_BOOL("-gS"))
		gr->gfxIsShared= true;

	return true;
}

//! Searches for tilemap options
bool grit_parse_map(GritRec *gr, const strvec &opts)
{
	int val;
	char *pstr;

	// no map opts or excluded	
	if(!CLI_BOOL("-m") || CLI_BOOL("-m!"))
	{
		gr->mapProcMode= GRIT_EXCLUDE;
		return true;
	}

	gr->mapProcMode= GRIT_EXPORT;

	gr->mapDataType= CLI_INT("-mu", 16)>>4;
	if( (val= grit_parse_cprs("-mz", opts)) != -1 )
		gr->mapCompression= val;

	// Tile offset
	gr->mapOffset= CLI_INT("-ma", 0);

	// Tile reduction
	pstr= CLI_STR("-mR", "");
	switch(pstr[0])
	{
	case '\0':	break;	// No option.
	case '!':	gr->mapRedux= GRIT_RDX_OFF;		break;
	case '4':	gr->mapRedux= GRIT_RDX_REG4;	break;
	case '8':	gr->mapRedux= GRIT_RDX_REG8;	break;
	case 'a':	gr->mapRedux= GRIT_RDX_AFF;		break;
	default:
		gr->mapRedux= GRIT_RDX_TILE;
		if(strchr(pstr, 'f'))
			gr->mapRedux |= GRIT_RDX_FLIP;
		if(strchr(pstr, 'p'))
			gr->mapRedux |= GRIT_RDX_PAL;
	}

	// Map format
	pstr= CLI_STR("-mL", "");
	switch(pstr[0])
	{
	case 'f':	gr->mapLayout= GRIT_MAP_FLAT;	break;
	case 's':	gr->mapLayout= GRIT_MAP_REG;	break;
	case 'a':	gr->mapLayout= GRIT_MAP_AFFINE;	break;
	}

	return true;
}

//! Parse options related to files
/*!	\todo	Fix -fx stuff. Maybe move some of it into validation as well.
*/
bool grit_parse_file(GritRec *gr, const strvec &opts)
{
	int ii;
	char *pstr, *pext, str[MAXPATHLEN];

	gr->bHeader= !CLI_BOOL("-fh!");
	gr->bAppend= CLI_BOOL("-fa");

	// --- Output file type ---

	pstr= CLI_STR("-ft", "");
	switch(pstr[0])
	{
	case '!':	gr->bExport= false;				break;
	case 'b':	gr->fileType= GRIT_FTYPE_BIN;	break;
	case 'g':	gr->fileType= GRIT_FTYPE_GBFS;	break;
	case 'c':	gr->fileType= GRIT_FTYPE_C;		break;
	case 'r':	gr->fileType= GRIT_FTYPE_GRF;	break;
	//case 'o':	gr->fileType= GRIT_FTYPE_O;		break;
	case 's':
	default:	gr->fileType= GRIT_FTYPE_S;	
	}

	// Special compression things for Riff:
	if(CLI_BOOL("-ftr") || CLI_BOOL("-fr"))
	{
		gr->bRiff= true;
		if(gr->gfxCompression==GRIT_CPRS_OFF)
			gr->gfxCompression= GRIT_CPRS_HEADER;

		if(gr->mapCompression==GRIT_CPRS_OFF)
			gr->mapCompression= GRIT_CPRS_HEADER;

		if(gr->palCompression==GRIT_CPRS_OFF)
			gr->palCompression= GRIT_CPRS_HEADER;
	}

	// --- Symbol name ---
	strrepl(&gr->symName, CLI_STR("-s", ""));


	// --- Output name ---

	pstr= CLI_STR("-o", "");
	if( !isempty(pstr) )
	{
		pstr= strrepl(&gr->dstPath, pstr);
		path_fix_sep(pstr);

		// Get type from name if not mentioned explicitly
		pext= path_get_ext(pstr);
		if( !CLI_BOOL("-ft") && !isempty(pext) )
		{
			for(ii=0; ii<countof(cFileTypes); ii++)
			{
				if(strcasecmp(pext, cFileTypes[ii]) == 0)
				{
					gr->fileType= ii;
					break;
				}
			}
		}
		lprintf(LOG_STATUS, "Output file: '%s'.\n", gr->dstPath);
	}

	// --- External file (ext tileset) ---

	pstr= CLI_STR("-fx", "");
	if( !isempty(pstr) )
	{
		strcpy(str, pstr);
		path_fix_sep(str);

		// Check for bitdepth support
		FREE_IMAGE_FORMAT fif= FreeImage_GetFIFFromFilename(str);
		FI_SUPPORT_MODE fsm=  fiGetSupportModes(fif);

		// No 8bpp support for filetype? Change to bmp
		if(~fsm & FIF_MODE_EXP_8BPP)
		{
			lprintf(LOG_WARNING, 
"Filetype of %s doesn't allow 8bpp export. Switching to bmp.\n", 
				path_get_name(str));
			path_repl_ext(str, str, "bmp", MAXPATHLEN);
				
		}
		strrepl(&gr->shared->tilePath, str);
		gr->gfxIsShared= true;

		lprintf(LOG_STATUS, "Ext file: %s.\n", gr->shared->tilePath);	
	}

	return true;
}


//! Parse logging options.
/*!	\note	Even though a \a gr is passes, it doesn't actually do 
*		anything to it. The log level is in the return value
*/
int grit_parse_log(GritRec *gr, const strvec &opts)
{
	char *pstr= CLI_STR("-W", "");

	switch(pstr[0])
	{
	case '1': case 'e':
		return LOG_ERROR;
	case '2': case 'w':
		return LOG_WARNING;
	case '3': case 's':
		return LOG_STATUS;
	}

#ifdef _DEBUG
	return LOG_STATUS;
#else
	return LOG_ERROR;
#endif
}

//! Parsing hub
/*! 
	\note Source stuff needs to be taken care of first!!
*/
bool grit_parse(GritRec *gr, const strvec &opts)
{
	int val;

	// === GLOBAL options ===

	// datatype: 8, 16, 32
	if( (val= CLI_INT("-U", 0)) != 0)
	{
		val >>= 4;	// 0, 1, 2 (hax)
		gr->palDataType= val;
		gr->gfxDataType= val;
		gr->mapDataType= val;
	}

	// Overall compression
	if( (val= grit_parse_cprs("-Z", opts)) != -1)
	{
		gr->palCompression= val;
		gr->gfxCompression= val;
		gr->mapCompression= val;
	}

	grit_parse_pal(gr, opts);
	grit_parse_gfx(gr, opts);
	grit_parse_map(gr, opts);

	// Image range
	// PONDER: what about adjusting for image size?
	gr->areaLeft= CLI_INT("-al", 0);
	if( (val= CLI_INT("-aw", -1)) != -1)
		gr->areaRight= gr->areaLeft + val;
	else if( (val= CLI_INT("-ar", -1)) != -1)
		gr->areaRight= val;

	gr->areaTop= CLI_INT("-at", 0);
	if( (val= CLI_INT("-ah", -1)) != -1)
		gr->areaBottom= gr->areaTop + val;
	else if( (val= CLI_INT("-ab", -1)) != -1)
		gr->areaBottom= val;

	// === (Meta)tiling options ===

	// -gt effectively means 8x8 tiles; Only use -tw and -th 
	// under -gb. This will chage when I get the non-8x8@8 tile-mapping
	// module in place.
	// Yes I know this sounds odd. When your time machine is finished, 
	// please go back to 2004-ish and tell me dividing things into 
	// either tiled or bitmapped graphics would lead to trouble later on.
	if(CLI_BOOL("-gb"))
	{
		gr->tileWidth= CLI_INT("-tw", 1);
		gr->tileHeight= CLI_INT("-th", 1);
	}

	// Meta size (in tiles!)
	// PONDER: range input?
	gr->metaWidth = CLI_INT("-Mw", 1);
	gr->metaHeight= CLI_INT("-Mh", 1);

	// meta palette reduce
	if( CLI_BOOL("-MRp") )
		gr->mapRedux |= GRIT_META_PAL;

	grit_parse_file(gr, opts);
	
	return true;
}

//! Special parse for shared data (grs assumed to be set-up first)
bool grit_parse_shared(GritRec *gr, const strvec &opts)
{
	int val;
	char *pstr;

	// datatype: 8, 16, 32
	if( (val= CLI_INT("-U", 0)) != 0)
	{
		val >>= 4;	// 0, 1, 2 (hax)
		gr->palDataType= val;
		gr->gfxDataType= val;
	}

	// Overall compression
	if( (val= grit_parse_cprs("-Z", opts)) != -1)
	{
		gr->palCompression= val;
		gr->gfxCompression= val;
	}

	grit_parse_pal(gr, opts);
	grit_parse_gfx(gr, opts);	

	grit_parse_file(gr, opts);

	// --- Tweakage. ---

	pstr= CLI_STR("-S", "");
	if(!isempty(pstr))
		strrepl(&gr->symName, pstr);
	
	pstr= CLI_STR("-O", "");			
	if(!isempty(pstr))
		strrepl(&gr->dstPath, pstr);

	if(!gr->gfxIsShared)
		gr->gfxProcMode &= ~GRIT_OUTPUT;
		
	if(!gr->palIsShared && !gr->gfxIsShared)
		gr->palProcMode &= ~GRIT_OUTPUT;

	if(gr->palIsShared)
	{
		gr->palStart= 0;
		gr->palEnd= gr->shared->palRec.height;
	}
		
	return true;
}


// --------------------------------------------------------------------
// External files
// --------------------------------------------------------------------


//! Load an external tile file.
/*!
	\todo	Move this somewhere proper, and expand its functionality. 
	  As soon as I know what it should do anyway.
*/
bool grit_load_ext_tiles(GritRec *gr)
{
	lprintf(LOG_STATUS, "Loading tile file.\n");

	if(dib_load == NULL)
	{
		lprintf(LOG_WARNING, "  File reader not initialized. Tilefile load failed.\n");
		return false;
	}

	GritShared *grs= gr->shared;

	if(isempty(grs->tilePath))
	{
		lprintf(LOG_WARNING, "  No tilefile path. Tilefile load failed.\n");
		return false;
	}

	CLDIB *dib= NULL, *dib2= NULL;
	dib= dib_load(grs->tilePath, NULL);

	if(dib == NULL)
	{
		lprintf(LOG_WARNING, "  Can't load tile file \"%s\".\n", grs->tilePath);
		return false;	
	}

	// Convert to 8xh@8 if necessary

	if( dib_get_bpp(dib) != 8 )
	{
		lprintf(LOG_WARNING, "  External tileset must be 8bpp. Converting.\n");

		dib2= dib_convert(dib, 8, 0);
		dib_free(dib);
		dib= dib2;
	}

	// TODO: allow for metatiled sets
	// PONDER: shouldn't this go somewhere else?
	if( dib_get_width(dib) != 8 )
	{
		lprintf(LOG_WARNING, "  External tileset not in tile format yet. Doing so now.\n");

		dib2= dib_redim(dib, 8, 8, 0);
		dib_free(dib);
		dib= dib2;
	}

	lprintf(LOG_STATUS, "  External tileset `%s' loaded\n", grs->tilePath);

	dib_free(grs->dib);
	grs->dib= dib;

	return true;
}

bool grit_save_ext_tiles(GritRec *gr)
{
	GritShared *grs= gr->shared;

	lprintf(LOG_STATUS, "Saving tile file: `%s'.\n", grs->tilePath);

	if(grs->dib == NULL)
		return true;

	if(dib_save == NULL)
	{
		lprintf(LOG_WARNING, "  File writer not initialized. Tilefile save failed.\n");
		return false;
	}

	if(isempty(grs->tilePath))
	{
		lprintf(LOG_WARNING, "  No tilefile path. Tilefile save failed.\n");
		return false;
	}

	if(dib_save(grs->dib, grs->tilePath, NULL) == false)
	{
		lprintf(LOG_WARNING, "  Can't save tiles to `%s'", grs->tilePath);
		return false;
	}

	return true;
}


// --------------------------------------------------------------------
// --------------------------------------------------------------------


//! Get all the options for a grit-run, cli args + from file
/*!	\todo	Change for multi-file runs
*/
void gather_opts(strvec &opts, int argc, char **argv)
{
	int ii;
	char str[MAXPATHLEN], *pstr;

	// Load cmd-line args
	for(ii=0; ii<argc; ii++)
		opts.push_back(strdup(argv[ii]));
	
	// Test for flag file (-ff {path}), or 'default' flag file
	path_repl_ext(str, argv[1], "grit", MAXPATHLEN);

	// if flag file, load args from file
	pstr= cli_str("-ff", opts, str);

	// Open grit-option file. Bug out if not found.
	FILE *fp= fopen(pstr, "r");
	if(fp == NULL)
	{
		lprintf(LOG_STATUS, "No .grit file\n");
		return;
	}


	// Get flags
	const char seps[]= "\n\r\t ";

	while( !feof(fp) )
	{
		fgets(str, 1024, fp);
		// Find comment and end string there
		if( pstr= strchr(str, '#') )
			*pstr= '\0';
		
		// Tokenize arguments
		pstr= strtok(str, seps);
		while( pstr != NULL )
		{
			opts.push_back(strdup(pstr));
			pstr= strtok(NULL, seps);
		}
	}

	fclose(fp);
}

bool prep_run(GritRec *gr, const char *fpath)
{
	grit_clear(gr);
	grit_init(gr);

	strrepl(&gr->srcPath, fpath);

	gr->srcDib= dib_load(gr->srcPath, NULL);
	if(gr->srcDib == NULL)
	{
		lprintf(LOG_ERROR, "\"%s\" not found or can't be read.\n", 
			gr->srcPath);
		return false;;
	}
	grit_init_from_dib(gr);

	return true;
}

// NOTE: 20080203: I need a way to keep track of what kind of conversion 
// we're doing. For now, this is it.
void grs_set_mode(GritShared *grs, const strvec &opts, const strvec &paths)
{
	bool bShared;

	bShared= CLI_BOOL("-fx") || CLI_BOOL("-gS") || CLI_BOOL("-pS");

	grs->sharedMode  = paths.size()>1 ? GRS_MULTI : GRS_SINGLE;
	if(bShared)
		grs->sharedMode |= GRS_SHARED; 
}


//! Run for file(s) individually.
/*!	\return 0 if successfull.
*/
int run_individual(GritRec *gr, const strvec &opts, const strvec &fpaths)
{
	int ii;

	lprintf(LOG_STATUS, "Individual runs.\n");

	//# TODO: Tweak troublesome options for multi-file runs.

	for(ii=0; ii<fpaths.size(); ii++)
	{
		lprintf(LOG_STATUS, "Input file %s\n", fpaths[ii]);

		if( !prep_run(gr, fpaths[ii]) )
			continue;

		grit_parse(gr, opts);

		// Run and stuff
		if( !grit_run(gr) )
		{
			lprintf(LOG_ERROR, "Conversion failed for %s :( \n", fpaths[ii]);
			return EXIT_FAILURE;
		}
	}
	return EXIT_SUCCESS;
}

//! Run for file(s) with shared data.
/*!	\return 0 if successfull.
*/
int run_shared(GritRec *gr, const strvec &opts, const strvec &fpaths)
{
	int ii;
	GritShared *grs= gr->shared;

	lprintf(LOG_STATUS, "Shared-data run.\n");

	//# TODO: Tweak troublesome option combos.

	// --- semi-dummy init for shared options ---
	prep_run(gr, fpaths[0]);
	grit_parse(gr, opts);

	grs->gfxBpp= gr->gfxBpp;

	// Read external tile file
	if(gr->gfxIsShared)
		grit_load_ext_tiles(gr);

	for(ii=0; ii<fpaths.size(); ii++)
	{
		lprintf(LOG_STATUS, "Input file %s\n", fpaths[ii]);

		if( !prep_run(gr, fpaths[ii]) )
			continue;

		// Parse options to init rest of GritRec
		// Yes, re-parse for each file; I'm not convinced
		// it'd work otherwise yet
		grit_parse(gr, opts);

		// Remove elements that are supposed to be shared.
		if(gr->gfxIsShared)
			gr->gfxProcMode &= ~GRIT_OUTPUT;

		if(gr->palIsShared || gr->gfxIsShared)
			gr->palProcMode &= ~GRIT_OUTPUT;

		// Run and stuff
		if( !grit_run(gr) )
		{
			lprintf(LOG_ERROR, "Conversion failed at %s :( \n", fpaths[ii]);
			return EXIT_FAILURE;			
		}		
	}

	// --- Shared data conversion ---

	// Save ext tiles
	if(gr->gfxIsShared)
		grit_save_ext_tiles(gr);

	// Special init for shared data (move to ~E~ later).
	grit_clear(gr);
	grit_init(gr);
	strrepl(&gr->srcPath, fpaths[0]);

	grit_parse_shared(gr, opts);

	grs_run(grs, gr);

	lprintf(LOG_STATUS, "Conversion finished.\n");

	return EXIT_SUCCESS;
}

//! Do a grit run.
int run_main(int argc, char **argv)
{
	int ii;
	int result= EXIT_FAILURE;
	GritRec *gr= NULL;
	strvec opts, fpaths;

	try 
	{
		// --- Option and file inits ---
		gather_opts(opts, argc, argv);
		log_init(grit_parse_log(NULL, opts), NULL);

		for(ii=1; ii<opts.size(); ii++)
		{
			if(opts[ii][0] == '-')
				break;
			fpaths.push_back(strdup(opts[ii]));
		}
		if(fpaths.size() == 0)
			throw "No source files.\n";


		gr= grit_alloc();
		if(gr == NULL)
			throw "Couldn't allocate GritRec memory.\n";

		lprintf(LOG_STATUS, "---%s ---\n", cVersion);

		// --- Run according to share-mode ---
		grs_set_mode(gr->shared, opts, fpaths);

		if(gr->shared->sharedMode & GRS_SHARED)
			result= run_shared(gr, opts, fpaths);
		else
			result= run_individual(gr, opts, fpaths);

		lprintf(LOG_STATUS, "Done!\n");

		// --- Finish up ---
		grit_free(gr);

		for(ii=0; ii<fpaths.size(); ii++)
			free(fpaths[ii]);

		for(ii=0; ii<opts.size(); ii++)
			free(opts[ii]);

		log_exit();

	}
	catch(const char *msg)
	{
		lprintf(LOG_ERROR, "%s", msg);
		result= EXIT_FAILURE;
	}

	return result;
}

int main(int argc, char **argv)
{
	if( argc < 2 || !strcmp(argv[1], "-?") || !strcmp(argv[1], "--help") )
	{
		fprintf(stdout, "---%s ---\n", cVersion);
		fputs(help_text, stdout);
		return EXIT_SUCCESS;
	}

	// Initialize, run and exit ---
	FreeImage_Initialise();
	fiInit();

	int result= run_main(argc, argv);

	FreeImage_DeInitialise();

	return result;
}


// EOF
