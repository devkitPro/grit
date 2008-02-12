//
//! \file grit_main.cpp
//!   Entry file for cmd-line grit.
//! \date 20050913 - 20070331
//! \author cearn
//
// === NOTES ===
// * 20070331, jv: 
//   - Fixed directory options for -fx.
//   - Added '-ft!', '-We', '-Ww', '-Ws' options
//   - changes default output to asm.
// * 20060729,jv: added vector-based grit_get_opts; I may mod 
//   other functions to use strvec for subroutines later too.
// * 20060729.jv: PONDER: now that I am using STL, would this be a 
//   good time to go for strings iso char*s as well?
 

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

void grit_dump(GRIT_REC *gr, FILE *fp);
void grit_dump_short(GRIT_REC *gr, FILE *fp, const char *pre);

#define ARRAY_N(array) sizeof(array)/sizeof(array[0])

typedef std::vector<char*> strvec;


// --------------------------------------------------------------------
// CONSTANTS
// --------------------------------------------------------------------



const char cVersion[]= "grit v" GRIT_VERSION;

const char help_text[]= 
"GIT: GBA Image Transmogrifier. (v" GRIT_VERSION ", " GRIT_BUILD ")\n"
"  Converts bitmap files into something the GBA can use.\n"
"usage: grit srcfile [opts]\n\n"
"--- Palette options (base: \"-p\") ---\n"
"-p | -p!       Include or exclude pal data [inc]\n"
"-pu{8,16,32}   Pal data-type: u8, u16 , u32 [u16]\n"
"-pz{!,l,h,r}   Pal compression: none , lz77, huff, RLE [none]\n"
"-ps{n}         Pal range start [0]\n"
"-pe{n}         Pal range end (exclusive) [pal size]\n"
"-pw{n}         Pal width [pal size]. Overrides -pe\n"
"-pT{n}         NEW: Transparent palette index; swaps with index 0 [0]\n"
//"-p[s:e]        Pal range: [s, e> (e exclusive)\n"
//"-p[s+n]        Pal range: [s,s+n>\n"
//"-p[n]          Pal range: [0,n> (def=all)\n"
"--- Graphics options (base: \"-g\") ---\n"
"-g | -g!       Include  or exclude gfx data [inc]\n"
"-gu{8,16,32}   Gfx data type: u8, u16, u32 [u32]\n"
"-gz{!,l,h,r}   Gfx compression: none, lz77, huff, RLE [none]\n"
"-ga{n}         Gfx pixel offset (non-zero pixels) [0]\n"
//"-gA{n}         Gfx pixel offset n (def=0) (all pixels) [0]\n"
"-gb | -gt      Gfx format, bitmap or tile [tile]\n"
"-gB{n}         Gfx bit depth (1, 2, 4, 8, 16) [img bpp]\n"
"-gT{n}         NEW: Transparent color; rrggbb hex or 16bit BGR hex [FF00FF]\n"
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
"-mu{8,16,32}   Map data type: u8, u16, u32 [u16]\n"
"-mz{!,l,h,r}   Map compression: none, lz77, huff, RLE [none]\n"
"-ma{n}         Map tile offset n (non-zero entries) [0]\n"
//"-mA{n}         Map tile offset n (all entries) [0]\n"
"-mR{t|p|f}     Tile reduction: (t)iles, (p)al, (f)lipped \n"
"                 options can be combined [-mRtpf]\n"
"-mR{4,8,a}     Common tile reduction combos: reg 4bpp (-mRtpf), \n"
"                 reg 8bpp (-mRtf), affine (-mRt), respectively\n"
"-mR!           No tile reduction (not advised)\n"
"-mL{f,s,a}     Map layout: reg flat, reg sbb, affine [reg flat]\n"
"--- Meta/Obj options (base: \"-M\") ---\n"
//"-M | -M!       Include or exclude (def) metamap data\n"
//"-M[w,h]        Metatile / obj sizes (in tiles!)\n"
"-Mh{n}         Metatile height (in tiles!) [1]\n"
"-Mw{n}         Metatile width (in tiles!) [1]\n"
"-MRp           Metatile reduction (pal only) [none]\n"
"--- File / var options ---\n"
"-ft{!,c,s,b,g} File type (no output, C, GNU asm, bin, gbfs) [.s]\n"
"-fa            File append\n"
"-fh | -fh!     Create header or not [create header]\n"
"-ff{name}      Additional options read from flag file [dst-name.grit]\n"
"-fx{name}      External tileset file\n"
"-o{name}       Destination filename [based on source]\n"
"-s{name}       Symbol base name [based from dst]\n"
"--- Misc ---\n"
"-q             Quiet mode; no report at the end\n"
"-U{8,16,32}    All data type: u8, u16, u32\n"
"-W{n}          Warning/log level 1, 2 or 3 [1]\n"
"-Z{!,l,h,r}    All compression: none, lz77, huff, RLE\n";


// --------------------------------------------------------------------
// PROTOTYPES
// --------------------------------------------------------------------


int grit_parse_cprs(const char *key, int argc, char **argv);
BOOL grit_parse_pal(GRIT_REC *gr, int argc, char **argv);
BOOL grit_parse_img(GRIT_REC *gr, int argc, char **argv);
BOOL grit_parse_map(GRIT_REC *gr, int argc, char **argv);
BOOL grit_parse(GRIT_REC *gr, int argc, char **argv);


void get_opts(strvec *av, int argc, char **argv);


// --------------------------------------------------------------------
// FUNCTIONS
// --------------------------------------------------------------------


//! Searches for compression options
/*!
	\param key	Base compression options (-Z, -gz, etc)
	\return	GRIT_CPRS_foo flag, or -1 if no sub-flag found.
*/
int grit_parse_cprs(const char *key, int argc, char **argv)
{
	// compression
	char *str= CLI_STR(key, "");
	switch(*str)
	{
	case '\0':
		return -1;
	case 'l':
		return GRIT_CPRS_LZ77;
	case 'h':	
		return GRIT_CPRS_HUFF;
	case 'r':	
		return GRIT_CPRS_RLE;
	case '!':	
	default:
		return 0;
	}
}

//! Searches for palette options
BOOL grit_parse_pal(GRIT_REC *gr, int argc, char **argv)
{
	int val;
	if( CLI_BOOL("-p!") == FALSE)
	{
		gr->pal_flags |= GRIT_INCL;	
		// data type
		val= CLI_INT("-pu", 0);	// 8, 16, 32
		if(val != 0)
			BF_SET(gr->pal_flags, val>>4, GRIT_U);

		// compression
		val= grit_parse_cprs("-pz", argc, argv);
		if(val != -1)
			BF_SET2(gr->pal_flags, val, GRIT_CPRS);

		// range
		gr->pal_start= CLI_INT("-ps", 0);

		if( (val= CLI_INT("-pn", -1)) >= 0)
			gr->pal_end= gr->pal_start + val;
		else if( (val= CLI_INT("-pe", -1)) >= 0)
			gr->pal_end= val;
	}
	else
		gr->pal_flags &= ~GRIT_INCL;


	// Transparent pal-id
	if( (val= CLI_INT("-pT", -1)) >= 0)
	{
		gr->pal_flags |= GRIT_PAL_TRANS;
		gr->pal_trans= val;
	}

	return TRUE;
}

//! Searches for image options
BOOL grit_parse_img(GRIT_REC *gr, int argc, char **argv)
{
	int val;
	char *pstr;

	if(CLI_BOOL("-g!") == FALSE)
	{
		gr->img_flags |= GRIT_INCL;

		// data type
		val= CLI_INT("-gu", 0);	// 8, 16, 32
		if(val != 0)
			BF_SET(gr->img_flags, val>>4, GRIT_U);
		// compression
		val= grit_parse_cprs("-gz", argc, argv);
		if(val != -1)
			BF_SET2(gr->img_flags, val, GRIT_CPRS);
		// pixel offset
		gr->img_ofs= CLI_INT("-ga", 0);
	}
	else
		gr->img_flags &= ~GRIT_INCL;

	// tile or bitmap format
	if(CLI_BOOL("-gt"))
		gr->img_flags &= ~GRIT_IMG_BMP;
	else if(CLI_BOOL("-gb"))
		gr->img_flags |= GRIT_IMG_BMP;

	// bitdepth override
	val= CLI_INT("-gB", 0);
	if(val != 0)
		gr->img_bpp= val;

	// TODO: Transparency
	if( CLI_BOOL("-gT") )
	{
		// String can be (0x?)[0-9A-Fa-f]{4,6}
		// <5 hexits means 16bit BGR color
		// >4 means rrggbb color
		pstr= CLI_STR("-gT", "FF00FF");
		if(pstr[0] != '!')
		{
			gr->img_trans= str2rgb(pstr);
			gr->img_flags |= GRIT_IMG_TRANS;
		}
		else
			gr->img_flags |= GRIT_IMG_BMP_A;
	}

	return TRUE;
}

//! Searches for tilemap options
BOOL grit_parse_map(GRIT_REC *gr, int argc, char **argv)
{
	int val;
	char *pstr;

	// no map opts or excluded	
	if(!CLI_BOOL("-m") || CLI_BOOL("-m!"))
	{
		gr->map_flags &= ~GRIT_INCL;
		return TRUE;
	}

	gr->map_flags |= GRIT_INCL;

	// data type
	val= CLI_INT("-mu", 0);	// 8, 16, 32
	if(val != 0)
		BF_SET(gr->map_flags, val>>4, GRIT_U);

	// compression
	val= grit_parse_cprs("-mz", argc, argv);
	if(val != -1)
		BF_SET2(gr->map_flags, val, GRIT_CPRS);

	// tile offset
	gr->map_ofs= CLI_INT("-ma", 0);

	// tile reduction
	pstr= CLI_STR("-mR", "");
	switch(pstr[0])
	{
	case '\0':
		break;
	case '!':
		gr->map_flags &= ~GRIT_RDX_ON; break;
	case '4':
		BF_SET2(gr->map_flags, GRIT_RDX_REG4, GRIT_RDX); break;
	case '8':
		BF_SET2(gr->map_flags, GRIT_RDX_REG8, GRIT_RDX); break;
	case 'a':
		BF_SET2(gr->map_flags, GRIT_RDX_AFF , GRIT_RDX); break;
	default:
		gr->map_flags &=~GRIT_RDX_MASK;
		gr->map_flags |= GRIT_RDX_ON;
		// check individual flags
		// PONDER: what about GRIT_RDX_BLANK ???
		if(strchr(pstr, 'f'))
			gr->map_flags |= GRIT_RDX_FLIP;
		if(strchr(pstr, 'p'))
			gr->map_flags |= GRIT_RDX_PAL;
	}

	// map format
	pstr= CLI_STR("-mL", "");
	switch(pstr[0])
	{
	case 'f':
		BF_SET2(gr->map_flags, GRIT_MAP_FLAT, GRIT_MAP_LAY); break;
	case 's':
		BF_SET2(gr->map_flags, GRIT_MAP_REG , GRIT_MAP_LAY); break;
	case 'a':
		BF_SET2(gr->map_flags, GRIT_MAP_AFF , GRIT_MAP_LAY); break;
	}
	return TRUE;
}

//! Parse options related to files
/*!	\todo	Fix -fx stuff. Maybe move some of it into validation as well.
*/
BOOL grit_parse_file(GRIT_REC *gr, int argc, char **argv)
{
	int ii;

	char *pstr, *pext, str[MAXPATHLEN];

	// header
	if( CLI_BOOL("-fh!") )
		gr->file_flags &= ~GRIT_FILE_H;

	// append mode
	if( CLI_BOOL("-fa") )
		gr->file_flags |= GRIT_FILE_CAT;


	// --- Output file type ---

	pstr= CLI_STR("-ft", "");
	switch(pstr[0])
	{
	case 'c':
		BF_SET2(gr->file_flags, GRIT_FILE_C   , GRIT_FTYPE); break;
	case 's':
		BF_SET2(gr->file_flags, GRIT_FILE_S   , GRIT_FTYPE); break;
	case 'b':
		BF_SET2(gr->file_flags, GRIT_FILE_BIN , GRIT_FTYPE); break;
	case 'g':
		BF_SET2(gr->file_flags, GRIT_FILE_GBFS, GRIT_FTYPE); break;
	case '!':
		gr->file_flags |= GRIT_FILE_NO_OUT;	break;
	default:
		BF_SET2(gr->file_flags, GRIT_FILE_S   , GRIT_FTYPE); break;

	// TODO:
	//case 'o':
	//	BF_SET2(gr->file_flags,  GRIT_FILE_O   , GRIT_FTYPE); break;
	}

	// --- Symbol name ---

	strrepl(&gr->sym_name, CLI_STR("-s", ""));


	// --- Output name ---

	pstr= CLI_STR("-o", "");
	if( !isempty(pstr) )
	{
		pstr= strrepl(&gr->dst_path, pstr);
		path_fix_sep(pstr);

		// Get type from name if not mentioned explicitly
		pext= path_get_ext(pstr);
		if( !CLI_BOOL("-ft") && !isempty(pext) )
		{
			for(ii=0; ii<ARRAY_N(cFileTypes); ii++)
			{
				if(strcasecmp(pext, cFileTypes[ii]) == 0)
				{
					BF_SET(gr->file_flags,  ii, GRIT_FTYPE);
					break;
				}
			}
		}
		lprintf(LOG_STATUS, "Output file: %s.\n", gr->shared->path);
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
		strrepl(&gr->shared->path, str);
		gr->img_flags |= GRIT_IMG_SHARED;

		lprintf(LOG_STATUS, "Ext file: %s.\n", gr->shared->path);	
	}

	return TRUE;
}

//! Parse logging options.
/*!	\note	Even though a \a gr is passes, it doesn't actually do 
*		anything to it. The log level is in the return value
*/
int grit_parse_log(GRIT_REC *gr, int argc, char **argv)
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
BOOL grit_parse(GRIT_REC *gr, int argc, char **argv)
{
	int val;

	// === GLOBAL options ===

	// datatype: 8, 16, 32
	val= CLI_INT("-U", 0);
	if(val != 0)
	{
		val >>= 4;	// 0, 1, 2 (hax)
		BF_SET(gr->pal_flags, val, GRIT_U);
		BF_SET(gr->img_flags, val, GRIT_U);
		BF_SET(gr->map_flags, val, GRIT_U);
	}

	// Overall compression
	val= grit_parse_cprs("-Z", argc, argv);
	if(val != -1)
	{
		BF_SET2(gr->pal_flags, val, GRIT_CPRS);
		BF_SET2(gr->img_flags, val, GRIT_CPRS);
		BF_SET2(gr->map_flags, val, GRIT_CPRS);
	}

	grit_parse_pal(gr, argc, argv);
	grit_parse_img(gr, argc, argv);
	grit_parse_map(gr, argc, argv);

	// Image range
	// PONDER: what about adjusting for image size?
	gr->area_left= CLI_INT("-al", 0);
	if( (val= CLI_INT("-aw", -1)) != -1)
		gr->area_right= gr->area_left + val;
	else if( (val= CLI_INT("-ar", -1)) != -1)
		gr->area_right= val;

	gr->area_top= CLI_INT("-at", 0);
	if( (val= CLI_INT("-ah", -1)) != -1)
		gr->area_bottom= gr->area_top + val;
	else if( (val= CLI_INT("-ab", -1)) != -1)
		gr->area_bottom= val;

	// === META options ===

	// Meta size (in tiles!)
	// PONDER: range input?
	gr->meta_width = CLI_INT("-Mw", 1);

	gr->meta_height= CLI_INT("-Mh", 1);
	// meta palette reduce
	if( CLI_BOOL("-MRp") )
		gr->map_flags |= GRIT_META_PAL;


	grit_parse_file(gr, argc, argv);
	
	return TRUE;
}



//! Load an external tile file.
/*!
*	\todo	Move this somewhere proper, and expand its functionality. 
*	  As soon as I know what it should do anyway.
*/
BOOL grit_load_ext_tiles(GRIT_REC *gr)
{
	lprintf(LOG_STATUS, "Loading tile file.\n");

	if(dib_load == NULL)
	{
		lprintf(LOG_WARNING, 
"  File reader not initialized. Tilefile load failed.\n");
		return FALSE;
	}

	GRIT_SHARED *grs= gr->shared;

	if(isempty(grs->path))
	{
		lprintf(LOG_WARNING, 
"  No tilefile path. Tilefile load failed.\n");
		return FALSE;
	}

	CLDIB *dib= NULL, *dib2= NULL;
	dib= dib_load(grs->path, NULL);

	if(dib == NULL)
	{
		lprintf(LOG_WARNING, 
"  Can't load tile file \"%s\".\n", grs->path);
		return FALSE;	
	}

	// Convert to 8xh@8 if necessary

	if( dib_get_bpp(dib) != 8 )
	{
		lprintf(LOG_WARNING, 
"  External tileset must be 8bpp. Converting.\n");

		dib2= dib_convert(dib, 8, 0);
		dib_free(dib);
		dib= dib2;
	}

	// TODO: allow for metatiled sets
	// PONDER: shouldn't this go somewhere else?
	if( dib_get_width(dib) != 8 )
	{
		lprintf(LOG_WARNING, 
"  External tileset not in tiles yet. Doing so now.\n");

		dib2= dib_redim(dib, 8, 8, 0);
		dib_free(dib);
		dib= dib2;
	}

	dib_free(grs->dib);
	grs->dib= dib;

	return TRUE;
}

BOOL grit_save_ext_tiles(GRIT_REC *gr)
{
	lprintf(LOG_STATUS, "Saving tile file.\n");

	GRIT_SHARED *grs= gr->shared;

	if(grs->dib == NULL)
		return TRUE;

	if(dib_save == NULL)
	{
		lprintf(LOG_WARNING, 
"  File writer not initialized. Tilefile save failed.\n");
		return FALSE;
	}

	if( isempty(grs->path))
	{
		lprintf(LOG_WARNING, 
"  No tilefile path. Tilefile save failed.\n");
		return FALSE;
	}

	if( dib_save(grs->dib, grs->path, NULL) == FALSE)
	{
		lprintf(LOG_WARNING, "  Can't save tiles to \"%s\"", grs->path);
		return FALSE;
	}

	return TRUE;
}


// --------------------------------------------------------------------
// --------------------------------------------------------------------


//! Get all the options for a grit-run, cli args + from file
/*!	\todo	Change for multi-file runs
*/
void gather_opts(strvec *av, int argc, char **argv)
{
	int ii;
	char str[1024], *pstr;

	// Load cmd-line args
	for(ii=0; ii<argc; ii++)
		av->push_back(strdup(argv[ii]));
	
	// Test for flag file (-ff {path}), or 'default' flag file
	path_repl_ext(str, argv[1], "grit", MAXPATHLEN);

	// if flag file, load args from file
	pstr = cli_str("-ff", argc, argv, str);

	// Attempt to open
	FILE *fp= fopen(pstr, "r"); 
	if(fp == NULL)
	{
		pstr= path_repl_ext(str, str, "grit", MAXPATHLEN);
		fp= fopen(pstr, "r");
		if(fp == NULL)
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
			av->push_back(strdup(pstr));
			pstr= strtok(NULL, seps);
		}
	}

	fclose(fp);
}

BOOL prep_run(GRIT_REC *gr, const char *fpath)
{
	grit_clear(gr);
	grit_init(gr);

	strrepl(&gr->src_path, fpath);

	gr->src_dib= dib_load(gr->src_path, NULL);
	if(gr->src_dib == NULL)
	{
		lprintf(LOG_ERROR, "\"%s\" not found or can't be read.\n", 
			gr->src_path);
		return FALSE;;
	}
	grit_init_from_dib(gr);

	return TRUE;
}

int main(int argc, char **argv)
{
	if( argc < 2 || !strcmp(argv[1], "-?") || !strcmp(argv[1], "--help") )
	{
		fputs(help_text, stdout);
		return 0;
	}

	fprintf(stdout, "---%s ---\n", cVersion);

	int ii;
	GRIT_REC *gr= NULL;
	strvec av, fpaths;

	// Primary Initialization
	gr= grit_alloc();

	FreeImage_Initialise();
	fiInit();

	gather_opts(&av, argc, argv);

	argc= av.size();
	argv= &av[0];

	// Init logger
	log_init(grit_parse_log(NULL, argc, argv), NULL);

	// Get source paths
	for(ii=0; ii<argc-1; ii++)
	{
		if(argv[ii+1][0] == '-')
			break;
		fpaths.push_back(strdup(argv[ii+1]));
		lprintf(0, "** src[%2d] : %s\n", ii, fpaths[ii]); 
	}

	char str[MAXPATHLEN];
	lprintf(LOG_STATUS, "cwd: %s\n.", getcwd(str, MAXPATHLEN));


	try
	{
		if(fpaths.size() == 0)
			throw "no source images!";

		// semi-dummy init for shared options
		prep_run(gr, fpaths[0]);
		grit_parse(gr, argc, argv);

		//# Fix shared options
		GRIT_SHARED *grs= gr->shared;
		grs->img_bpp= gr->img_bpp;

		// TODO: Read ext tile file
		if(gr->img_flags & GRIT_IMG_SHARED)
			grit_load_ext_tiles(gr);


		// NOTE: while the below 'works', it is not very efficient 
		// with data that could be shared between iterations 
		// (like external files and such). This should be remedied
		// in the future
		for(ii=0; ii<fpaths.size(); ii++)
		{
			if( !prep_run(gr, fpaths[ii]) )
				continue;

			// Parse options to init rest of GRIT_REC
			// Yes, re-parse for each file; I'm not convinced
			// it'd work otherwise yet
			grit_parse(gr, argc, argv);


			// Run and stuff
			if( !grit_run(gr) )
				lprintf(0, "Conversion failed :( \n");

		}

		// TODO: Save ext tiles
		if(gr->img_flags & GRIT_IMG_SHARED)
			grit_save_ext_tiles(gr);

		lprintf(LOG_NONE, "Conversion finished.\n");
	}
	catch(const char *msg)
	{
		lprintf(LOG_ERROR, "%s\n", msg);
	}

	// Primary de-init
	log_exit();

	for(ii=0; ii<fpaths.size(); ii++)
		free(fpaths[ii]);

	for(ii=0; ii<av.size(); ii++)
		free(av[ii]);

	FreeImage_DeInitialise();
	grit_free(gr);

	return 0;
}


// EOF
