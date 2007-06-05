//
//! \file grit_xp.cpp
//!   Exporter routines
//! \date 20050814 - 20070401
//! \author cearn
//
// === NOTES ===
// * 20070401: 
//   - Fixed directory issues
//   - Updated preface
// * 20061121: xp to source now use block tags so that append *replaces* 
//   old blocks instead of just tagging new ones on. This is a Good Thing.
//   What isn't a good think is how xp_h(), xp_c(), xp_gas() now have 
//   large and identical start/begin blocks to make it happen. The 
//   functions I use to search/copy in the existing files could prolly 
//   be improved too.
//   Also, I'm using tmpnam(), which apparently isn't recommended due to 
//   safety, but I don't know a suitable, portable alternative.
// * strupr for non-msvc
// * PATH_MAX -> MAXPATHLEN; added path_fix_sep() use.

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <cldib_core.h>
#include "grit.h"


// --------------------------------------------------------------------
// CONSTANTS
// --------------------------------------------------------------------


static const char GBFS_magic[] = "PinEightGBFS\r\n\032\n";


// --------------------------------------------------------------------
// CLASSES
// --------------------------------------------------------------------


typedef struct GBFS_FILE
{
  char magic[16];    /* "PinEightGBFS\r\n\032\n" */
  u32  total_len;    /* total length of archive */
  u16  dir_off;      /* offset in bytes to directory */
  u16  dir_nmemb;    /* number of files */
  char reserved[8];  /* for future use */
} GBFS_FILE;

typedef struct GBFS_ENTRY
{
  char name[24];     /* filename, nul-padded */
  u32  len;          /* length of object in bytes */
  u32  data_offset;  /* in bytes from beginning of file */
} GBFS_ENTRY;

#define GBFL_SIZE sizeof(GBFS_FILE)
#define GBEN_SIZE sizeof(GBFS_ENTRY)


// --------------------------------------------------------------------
// PROTOTYPES
// --------------------------------------------------------------------


// C export
BOOL grit_xp_c(GRIT_REC *gr);

// GAS export
BOOL grit_xp_gas(GRIT_REC *gr);


// BIN export
BOOL grit_xp_bin(GRIT_REC *gr);

// GBFS export
BOOL grit_xp_gbfs(GRIT_REC *gr);

// ELF export
//BOOL grit_xp_o(GRIT_REC *gr);
//BOOL xp_array_o(FILE *fp, const char *varname, 
//	const BYTE *data, int len, int chunk);
//BOOL xp_data_o(FILE *fp, const BYTE *data, int len, int chunk);

// misc
void grit_xp_decl(FILE *fp, int chunk, const char *name, int affix, int len);
BOOL grit_xp_h(GRIT_REC *gr);
BOOL grit_preface(GRIT_REC *gr, FILE *fp, const char *cmt);


// --------------------------------------------------------------------
// FUNCTIONS
// --------------------------------------------------------------------


//! Hub for export to file.
/*!
	Call this after all preparation is complete to export to the
	various file formats.
*/
BOOL grit_export(GRIT_REC *gr)
{
	// header file
	if(gr->file_flags & GRIT_FILE_H)
		grit_xp_h(gr);

	// and the rest
	switch(gr->file_flags & GRIT_FTYPE_MASK)
	{
	case GRIT_FILE_C:
		grit_xp_c(gr);	break;
	case GRIT_FILE_S:
		grit_xp_gas(gr);	break;
	case GRIT_FILE_BIN:
		grit_xp_bin(gr);	break;
	case GRIT_FILE_GBFS:
		grit_xp_gbfs(gr);	break;
	//case GRIT_FILE_O:
	//	grit_xp_o(gr);	break;
	default:
		return FALSE;
	}

	return TRUE;
}

// ====================================================================
// === EXPORT TO C ====================================================

//! Export to C source file.
BOOL grit_xp_c(GRIT_REC *gr)
{

	char str[MAXPATHLEN];
	char tag[MAXPATHLEN], fpath[MAXPATHLEN], tmppath[MAXPATHLEN];
	long pos= -1;
	FILE *fin, *fout;
	BOOL bAppend= gr->file_flags & GRIT_FILE_CAT;

	// Prep begin tag
	sprintf(tag, "//{{BLOCK(%s)",gr->sym_name);

	// Open 'output' file
	strcpy(fpath, gr->dst_path);


	lprintf(LOG_STATUS, 
"Export to C: %s into %s .\n", gr->sym_name, fpath);		

	// File doesn't exist -> write-mode only
	if(!file_exists(fpath))
		bAppend=FALSE;

	if(bAppend)
	{
		// Open temp and input file
		tmpnam(tmppath);
		if( (fout=fopen(tmppath, "w")) == NULL)
			return FALSE;

		fin= fopen(fpath, "r");
		pos= file_find_tag(fout, fin, tag);
	}
	else
		fout= fopen(fpath, "w");

	// Add blank line before new block
	if(pos == -1)
		fputc('\n', fout);

	// --- Start grit-block ---

	fprintf(fout, "%s\n\n", tag);

	grit_preface(gr, fout, "//");

	if(gr->pal_flags & GRIT_INCL)
	{
		strcpy(str, gr->sym_name);
		strcat(str, cAffix[E_PAL]);
		xp_array_c(fout, str, gr->_pal_rec.data, rec_size(&gr->_pal_rec),
			GRIT_CHUNK(gr, pal));
	}

	if(gr->img_flags & GRIT_INCL)
	{
		strcpy(str, gr->sym_name);
		strcat(str, (GRIT_IS_BMP(gr) ? cAffix[E_BM] : cAffix[E_TILE]) );
		xp_array_c(fout, str, gr->_img_rec.data, rec_size(&gr->_img_rec),
			GRIT_CHUNK(gr, img));
	}

	if(gr->map_flags & GRIT_INCL)
	{
		strcpy(str, gr->sym_name);
		strcat(str, cAffix[E_MAP]);
		xp_array_c(fout, str, gr->_map_rec.data, rec_size(&gr->_map_rec),
			GRIT_CHUNK(gr, map));

		if(gr->meta_width*gr->meta_height >1)
		{
			strcpy(str, gr->sym_name);
			strcat(str, cAffix[E_META]);
			xp_array_c(fout, str, gr->_meta_rec.data, rec_size(&gr->_meta_rec),
				GRIT_CHUNK(gr, map));
		}
	}

	sprintf(tag, "//}}BLOCK(%s)",gr->sym_name);
	fprintf(fout, "%s\n", tag);

	// --- End grit-block ---

	if(bAppend)
	{
		// Skip till end tag and copy rest
		file_find_tag(NULL, fin, tag);
		file_copy(fout, fin, -1);

		// close files and rename
		fclose(fout);
		fclose(fin);
		remove(fpath);
		rename(tmppath, fpath);
	}
	else
	{
		fclose(fout);		// close files
	}

	return TRUE;
}


// ====================================================================
// === EXPORT TO GNU ASM ==============================================

//! Export to GNU Assembly
// Yes, it is almost identical to c. Maybe I'll merge the two 
// later.
BOOL grit_xp_gas(GRIT_REC *gr)
{
	char str[MAXPATHLEN];
	char tag[MAXPATHLEN], fpath[MAXPATHLEN], tmppath[MAXPATHLEN];
	long pos= -1;
	FILE *fin, *fout;
	BOOL bAppend= gr->file_flags & GRIT_FILE_CAT;

	// Prep begin tag
	sprintf(tag, "@{{BLOCK(%s)",gr->sym_name);

	// Open 'output' file
	strcpy(fpath, gr->dst_path);


	lprintf(LOG_STATUS, 
"Export to GNU asm: %s into %s .\n", gr->sym_name, fpath);		

	// File doesn't exist -> write-mode only
	if(!file_exists(fpath))
		bAppend=FALSE;

	if(bAppend)
	{
		// Open temp and input file
		tmpnam(tmppath);
		if( (fout=fopen(tmppath, "w")) == NULL)
			return FALSE;

		fin= fopen(fpath, "r");
		pos= file_find_tag(fout, fin, tag);
	}
	else
		fout= fopen(fpath, "w");

	// Add blank line before new block
	if(pos == -1)
		fputc('\n', fout);

	// --- Start grit-block ---

	fprintf(fout, "%s\n\n", tag);

	grit_preface(gr, fout, "@");

	if(gr->pal_flags & GRIT_INCL)
	{
		strcpy(str, gr->sym_name);
		strcat(str, cAffix[E_PAL]);
		xp_array_gas(fout, str, gr->_pal_rec.data, rec_size(&gr->_pal_rec),
			GRIT_CHUNK(gr, pal));
	}

	if(gr->img_flags & GRIT_INCL)
	{
		strcpy(str, gr->sym_name);
		strcat(str, (GRIT_IS_BMP(gr) ? cAffix[E_BM] : cAffix[E_TILE]) );
		xp_array_gas(fout, str, gr->_img_rec.data, rec_size(&gr->_img_rec),
			GRIT_CHUNK(gr, img));
	}

	if(gr->map_flags & GRIT_INCL)
	{
		strcpy(str, gr->sym_name);
		strcat(str, cAffix[E_MAP]);
		xp_array_gas(fout, str, gr->_map_rec.data, rec_size(&gr->_map_rec),
			GRIT_CHUNK(gr, map));

		if(gr->meta_width*gr->meta_height >1)
		{
			strcpy(str, gr->sym_name);
			strcat(str, cAffix[E_META]);
			xp_array_gas(fout, str, gr->_meta_rec.data, rec_size(&gr->_meta_rec),
				GRIT_CHUNK(gr, map));
		}
	}

	sprintf(tag, "@}}BLOCK(%s)",gr->sym_name);
	fprintf(fout, "%s\n", tag);

	// --- End grit-block ---

	if(bAppend)
	{
		// Skip till end tag and copy rest
		file_find_tag(NULL, fin, tag);
		file_copy(fout, fin, -1);

		// close files and rename
		fclose(fout);
		fclose(fin);
		remove(fpath);
		rename(tmppath, fpath);
	}
	else
	{
		fclose(fout);		// close files
	}

	return TRUE;
}


// ====================================================================
// === EXPORT TO BIN ==================================================


//! Export to binary files
/*!
	\note Binary files cannot be appended, and are separate files.
*/
BOOL grit_xp_bin(GRIT_REC *gr)
{
	char name[MAXPATHLEN], str[MAXPATHLEN];
	char *fmodes[]= {"a+b", "wb"}, *fmode;

	lprintf(LOG_STATUS, 
"Export to binary files.\n");		

	path_repl_ext(str, gr->dst_path, NULL, MAXPATHLEN);
	
	fmode= fmodes[gr->file_flags & GRIT_FILE_CAT ? 0 : 1];

	if(gr->pal_flags & GRIT_INCL)
	{
		sprintf(name, "%s.pal.bin", str);
		xp_data_bin(name, gr->_pal_rec.data, rec_size(&gr->_pal_rec), 
			fmode);
	}

	if(gr->img_flags & GRIT_INCL)
	{
		sprintf(name, "%s.img.bin", str);
		xp_data_bin(name, gr->_img_rec.data, rec_size(&gr->_img_rec), 
			fmode);
	}

	if(gr->map_flags & GRIT_INCL)
	{
		sprintf(name, "%s.map.bin", str);
		xp_data_bin(name, gr->_map_rec.data, rec_size(&gr->_map_rec), 
			fmode);

		if(gr->meta_width*gr->meta_height > 1)
		{
			sprintf(name, "%s.meta.bin", str);
			xp_data_bin(name, gr->_meta_rec.data, rec_size(&gr->_meta_rec), 
				fmode);
		}
	}
	return TRUE;
}


// ====================================================================
// === EXPORT TO GBFS =================================================

int gbfs_namecmp(const void *a, const void *b);
int grit_gbfs_entry_init(GBFS_ENTRY *gben, const RECORD *rec, 
	const char *basename, int affix);

//! Export to GBFS file
/*! The 'append' mode for this function is special. Yes, it adds 
	new data to the file, but it also <i>replaces</i> old data of 
	the same name. Yes, this is proper procedure and should happen 
	in the others as well, but it's just easier in a binary 
	environment. The header file will still have double definitions,
	though.
	\todo	Code is inconsistent with the C/asm exporters. Rectify.
*/
BOOL grit_xp_gbfs(GRIT_REC *gr)
{
	int ii, jj;

	// for new data
	int gr_count;
	BYTE *gr_data[4];
	GBFS_ENTRY gr_gben[4];

	// for total data
	int gb_count;
	GBFS_ENTRY *gbenL= gr_gben, *gbenD= NULL;

	ii=0;
	// --- register the various fields ---
	// palette
	if(gr->pal_flags & GRIT_INCL)
	{
		grit_gbfs_entry_init(&gr_gben[ii], &gr->_pal_rec, 
			gr->sym_name, E_PAL);
		gr_data[ii++]= gr->_pal_rec.data;
	}
	// image
	if(gr->img_flags & GRIT_INCL)
	{
		grit_gbfs_entry_init(&gr_gben[ii], &gr->_img_rec, 
			gr->sym_name, (GRIT_IS_BMP(gr) ? E_BM : E_TILE));
		gr_data[ii++]= gr->_img_rec.data;
	}
	// map
	if(gr->map_flags & GRIT_INCL)
	{
		grit_gbfs_entry_init(&gr_gben[ii], &gr->_map_rec, 
			gr->sym_name, E_MAP);
		gr_data[ii++]= gr->_map_rec.data;

		// meta map
		if(gr->meta_width*gr->meta_height > 1)
		{
			grit_gbfs_entry_init(&gr_gben[ii], &gr->_meta_rec, 
				gr->sym_name, E_META);
			gr_data[ii++]= gr->_meta_rec.data;
		}
	}
	gb_count= gr_count= ii;

	// --- create header and finish entries ---
	// Create basic header
	GBFS_FILE gbhdr;
	memcpy(gbhdr.magic, GBFS_magic, sizeof(GBFS_magic));
	gbhdr.dir_off= GBFL_SIZE;

	char name[MAXPATHLEN];
	strcpy(name, gr->dst_path);
	path_fix_sep(name);

	lprintf(LOG_STATUS, 
"Export to GBFS: %s into %s .\n", gr->sym_name, name);		


	FILE *fout= fopen("gbfs.tmp", "wb");
	if(fout == NULL)
		return FALSE;

	u32 fpos= GBFL_SIZE + gr_count*GBEN_SIZE;
	fseek(fout, fpos, SEEK_SET);	// skip directory

	// Insert mode
	// Replace old with same name; insert new
	if(gr->file_flags & GRIT_FILE_CAT)
	{
		FILE *fin= fopen(name, "rb");
		do
		{
			if(fin == NULL)
				break;

			GBFS_FILE oldhdr;
			// read and check file's header
			if(fread(&oldhdr, GBFL_SIZE, 1, fin) < 1)
			{	break;	}
			if(memcmp(oldhdr.magic, GBFS_magic, 16))
				break;

			// if we're here, we have a valid GBFS

			int old_count= oldhdr.dir_nmemb;
			gbenD= (GBFS_ENTRY*)malloc(old_count*GBEN_SIZE);
			if(gbenD==NULL)
				break;

			// Read directory
			fread(gbenD, GBEN_SIZE, old_count, fin);
			// check for obsoletes
			for(ii=0; ii<gr_count; ii++)
			{
				gbenL= (GBFS_ENTRY*)bsearch(&gr_gben[ii], gbenD, 
					old_count, GBEN_SIZE, gbfs_namecmp);
				if(gbenL == NULL)
					continue;
				// Obsolete found; remove from list;
				jj= (gbenL-gbenD);
				old_count--;
				memmove(gbenL, gbenL+1, (old_count-jj)*GBEN_SIZE);
			}
			// NOTE: old_count may have been modified
			gb_count= gr_count+old_count;

			// obsoletes are removed; write the rest to fout
			BYTE buf[1024];
			fpos= GBFL_SIZE + gb_count*GBEN_SIZE;
			fseek(fout, fpos, SEEK_SET);

			for(ii=0; ii<old_count; ii++)
			{
				fseek(fin, gbenD[ii].data_offset, SEEK_SET);
				gbenD[ii].data_offset= fpos;
				
				jj= gbenD[ii].len >> 10;
				while(jj--)
				{
					fread(buf, 1024, 1, fin);
					fwrite(buf, 1024, 1, fout);
				}
				if((jj= gbenD[ii].len & 1023) != 0)
				{
					fread(buf, jj, 1, fin);
					fwrite(buf, jj, 1, fout);
				}

				// pad to 16byte boundary
				for(fpos=ftell(fout); fpos & 0x0F; fpos++)
					fputc(0, fout);
			}

			// Combine lists
			gbenL= (GBFS_ENTRY*)malloc(gb_count*GBEN_SIZE);
			memcpy(gbenL, gr_gben, gr_count*GBEN_SIZE);
			memcpy(&gbenL[gr_count], gbenD, old_count*GBEN_SIZE);
			free(gbenD);
			gbenD= gbenL;
		} while(0);

		if(fin)
			fclose(fin);
	}
	// At this point, either: 
	// * Insert-mode failed or wasn't issued 
	//   -> gb_count==gr_count
	//   -> fpos is past directory
	//   -> gbenL= gr_gben ; short list
	// * Insert-mode worked:
	//   -> gb_count= full count ; 
	//   -> fpos is past up-to-date entries
	//   -> gbenL=gbenD contains all GBFS_ENTRYs; 

	// --- Write to file ---
	
	// Write entries' data
	// NOTE: only new data; 'old' data is already take care of
	fpos= ftell(fout);
	for(ii=0; ii<gr_count; ii++)
	{
		gbenL[ii].data_offset= fpos;
		fwrite(gr_data[ii], gbenL[ii].len, 1, fout);

		// pad to 16byte boundary
		for(fpos=ftell(fout); fpos & 0x0F; fpos++)
			fputc(0, fout);
	}

	gbhdr.dir_nmemb= gb_count;
	gbhdr.total_len= fpos;

	// Sort entries _after_ writing the data
	// otherwise the lengths may be screwed up
	qsort(gbenL, gb_count, GBEN_SIZE, gbfs_namecmp);

	// Write header and directory
	rewind(fout);		
	fwrite(&gbhdr, GBFL_SIZE, 1, fout);
	fwrite(gbenL, GBEN_SIZE, gb_count, fout);

	SAFE_FREE(gbenD);
	fclose(fout);

	// rename to real name
	remove(name);
	rename("gbfs.tmp", name);

	return TRUE;
}

// for qsort
int gbfs_namecmp(const void *a, const void *b)
{
	return memcmp(a, b, 24);
}

//! Fills a GBFS_ENTRY.
/*!
	\param gben Entry to fill
	\param rec Record to register
	\param basename Base name for the the entry (cut to 21 letters)
	\param affix Affix for the name.
	\return Uhm ... nothing, really.
	\note The names of GBFS_ENTRY's are 24 chars max, so long names 
	  are cut off. But, because the affix is an integral name is 
	  part of the full name, the \a basename is cut to 21, after 
	  which the affix is added. The whole thing is then cut to 24 
	  chars.
*/
int grit_gbfs_entry_init(GBFS_ENTRY *gben, const RECORD *rec, 
	const char *basename, int affix)
{
	char name[32];
	strncpy(name, basename, 21);
	name[21]= '\0';
	strcat(name, cAffix[affix]);

	strncpy(gben->name, name, 24);
	gben->len= rec_size(rec);
	gben->data_offset= 0;

	return 1;
}


// ====================================================================
// === EXPORT TO ELF ==================================================

//! Export to GNU object file
/*!
	\note Non-functional. For future use
*/
BOOL grit_xp_o(GRIT_REC *gr)
{
	return TRUE;
}


//! Export a single array to GNU object file (for future use)
BOOL xp_array_o(FILE *fp, const char *varname, 
	const BYTE *data, int len, int chunk);

// ====================================================================
// === HELPERS ========================================================


//! Write C declaration and size define to file
/*!
	\param fp. FILE pointer
	\param chunk # bytes per datatype (1,2,4)
	\param name Base name of the variable
	\param affix Data-affix index (for &ldquo;Pal&rdquo;, 
		&ldquo;Map&rdquo;, etc)
	\param len Size in bytes. Will be ceilinged for the type. 
		(though it probably already is)
	\remark Example use: grit_xp_decl(fp, 2, "foo", E_MAP, 256); 
		gives <br>
		[code]<br><code>
		#define fooMapLen 256<br>
		extern const unsigned short fooMap[128];</code><br>
		[/code]
*/
void grit_xp_decl(FILE *fp, int chunk, const char *name, int affix, int len)
{
	fprintf(fp, "#define %s%sLen %d\n", name, cAffix[affix], len);
	fprintf(fp, "%s %s %s%s[%d];\n\n", cTypeSpec, cCTypes[chunk], 
		name, cAffix[affix], (len+chunk-1)/chunk);
}

//! Creates header file with the declaration list
/*
	\note The header file is for declarations only; it does NOT have 
		actual data in it. Data in headers is bad, so don't have 
		data in headers, mkay? Mkay!
*/
BOOL grit_xp_h(GRIT_REC *gr)
{
	char str[MAXPATHLEN];
	char tag[MAXPATHLEN], fpath[MAXPATHLEN], tmppath[MAXPATHLEN];
	long pos= -1;
	FILE *fin, *fout;
	BOOL bAppend= gr->file_flags & GRIT_FILE_CAT;

	// Prep begin tag
	sprintf(tag, "//{{BLOCK(%s)",gr->sym_name);

	// Open 'output' file
	path_repl_ext(fpath, gr->dst_path, "h", MAXPATHLEN);

	// File doesn't exist -> write-mode only
	if(!file_exists(fpath))
		bAppend=FALSE;

	if(bAppend)
	{
		// Open temp and input file
		tmpnam(tmppath);
		if( (fout=fopen(tmppath, "w")) == NULL)
			return FALSE;

		fin= fopen(fpath, "r");
		pos= file_find_tag(fout, fin, tag);
	}
	else
		fout= fopen(fpath, "w");

	// Add blank line before new block
	if(pos == -1)
		fputc('\n', fout);

	// --- Start grit-block ---

	fprintf(fout, "%s\n\n", tag);

	grit_preface(gr, fout, "//");

	// include guards
	strcpy(str, gr->sym_name);
	strupr(str);
	fprintf(fout, "#ifndef __%s__\n#define __%s__\n\n", str, str);

	// Palette comments
	if(gr->pal_flags & GRIT_INCL)
		grit_xp_decl(fout, GRIT_CHUNK(gr,pal), gr->sym_name, E_PAL, 
			rec_size(&gr->_pal_rec));

	// bitmap/tileset comments
	if(gr->img_flags &	GRIT_INCL)
		grit_xp_decl(fout, GRIT_CHUNK(gr,img), gr->sym_name, 
			(GRIT_IS_BMP(gr) ? E_BM : E_TILE), 
			rec_size(&gr->_img_rec));

	// (meta-)map comments
	if(gr->map_flags & GRIT_INCL)
	{
		int chunk= GRIT_CHUNK(gr,map);
		grit_xp_decl(fout, chunk, gr->sym_name, E_MAP, 
			rec_size(&gr->_map_rec));

		if(gr->meta_width*gr->meta_height != 1)
			grit_xp_decl(fout, chunk, gr->sym_name, E_META, rec_size(&gr->_meta_rec));
	}

	// include guards again
	strcpy(str, gr->sym_name);
	strupr(str);
	fprintf(fout, "#endif // __%s__\n\n", str);

	sprintf(tag, "//}}BLOCK(%s)",gr->sym_name);
	fprintf(fout, "%s\n", tag);

	// --- End grit-block ---

	if(bAppend)
	{
		// Skip till end tag and copy rest
		file_find_tag(NULL, fin, tag);
		file_copy(fout, fin, -1);

		// close files and rename
		fclose(fout);
		fclose(fin);
		remove(fpath);
		rename(tmppath, fpath);
	}
	else
	{
		// close files
		fclose(fout);
	}

	return TRUE;
}

//! Creates data preface, containing a data description
/*!
	The preface describres a little bit about the preferred 
	interpolation of the data: bitmap size, bitdepth, compression, 
	size, etc. As data itself is rather open to interpretation, 
	this can come in very handy after a while.
	\param gr	Git record.to describe
	\param fp	FILE pointer
	\param cmt	Comment symbol string (like &ldquo;//&rdquo; for C)
*/
BOOL grit_preface(GRIT_REC *gr, FILE *fp, const char *cmt)
{
	int tmp, size=0;
	u32 flags;
	int aw, ah, mw, mh;
	char hline[80], str2[16]="", str[96]="";

	time_t aclock;
	struct tm *newtime;

	// ASSERT(fp)

	memset(hline, '=', 72);
	hline[72]= '\n';
	hline[73]= '\0';
	memcpy(hline, cmt, strlen(cmt));

	aw= gr->area_right-gr->area_left;
	ah= gr->area_bottom-gr->area_top;
	mw= gr->meta_width;
	mh= gr->meta_height;

	fprintf(fp, "%s%s\n", hline, cmt);
	fprintf(fp, "%s\t%s, %dx%d@%d, \n", cmt, gr->sym_name, 
		aw, ah, gr->img_bpp);

	// Transparency options
	if(gr->img_flags & GRIT_IMG_TRANS)
	{
		RGBQUAD *rgb= &gr->img_trans;
		fprintf(fp, "%s\tTransparent color : %02X,%02X,%02X\n", cmt, 
			rgb->rgbRed, rgb->rgbBlue, rgb->rgbBlue);
	}
	else if(gr->img_flags & GRIT_IMG_BMP_A)
	{
		fprintf(fp, "%s\tAlphabit on.\n", cmt);
	}
	else if(gr->pal_flags & GRIT_PAL_TRANS)
	{
		fprintf(fp, "%s\tTransperent palette entry: %d.\n", cmt, gr->pal_trans);
	}


	// Palette comments
	flags= gr->pal_flags;
	if(flags & GRIT_INCL)
	{
		tmp= rec_size(&gr->_pal_rec);
		fprintf(fp, "%s\t+ palette %d entries, ", cmt, tmp/2);

		fprintf(fp, "%s compressed\n", cCprs[BF_GET(flags,GRIT_CPRS)]);
		
		sprintf(str2, "%d + ", tmp);
		strcat(str, str2);
		size += tmp;
	}

	// bitmap/tileset comments
	flags= gr->img_flags;
	if(flags &	GRIT_INCL)
	{
		fprintf(fp, "%s\t+ ", cmt);
		if(flags & GRIT_IMG_BMP)
		{
			fputs("bitmap ", fp);
		}
		else
		{
			tmp= dib_get_width(gr->_dib)*
				dib_get_height(gr->_dib)/64;
			fprintf(fp, "%d tiles ", tmp);
			u32 mflags= gr->map_flags;
			if( (mflags&(GRIT_RDX_ON|GRIT_INCL)) == (GRIT_RDX_ON|GRIT_INCL) )
			{
				// TODO: blank flag
				fputs("(t", fp);
				if(mflags & GRIT_RDX_FLIP)
					fputs("|f", fp);
				if(mflags & GRIT_RDX_PAL)
					fputs("|p", fp);
				fputs(" reduced) ", fp);
			}
			// if map is printed as well, put meta cmt there
			if(mw*mh != 1 && (~mflags&GRIT_INCL))
				fprintf(fp, "%dx%d metatiles ", mw, mh);
		}
		fprintf(fp, "%s compressed\n", 
			cCprs[BF_GET(flags, GRIT_CPRS)]);

		tmp= rec_size(&gr->_img_rec);
		sprintf(str2, "%d + ", tmp);
		strcat(str, str2);
		size += tmp;
	}

	// (meta-)map comments
	flags= gr->map_flags;
	if(flags & GRIT_INCL)
	{
		fprintf(fp, "%s\t+ ", cmt);
		switch(flags & GRIT_MAP_LAY_MASK)
		{
		case GRIT_MAP_FLAT:
			fputs("regular map (flat), ", fp);		break;
		case GRIT_MAP_REG:
			fputs("regular map (in SBBs), ", fp);	break;
		case GRIT_MAP_AFF:
			fputs("affine map, ", fp);				break;
		}

		fprintf(fp, "%s compressed, ", 
			cCprs[BF_GET(flags,GRIT_CPRS)]);
		
		tmp= rec_size(&gr->_map_rec);
		sprintf(str2, "%d + ", tmp);
		strcat(str, str2);
		size += tmp;


		fprintf(fp, "%dx%d \n", aw/mw/8, ah/mh/8);
		if(gr->meta_width*gr->meta_height != 1)
		{
			fprintf(fp, "%s\t%dx%d metatiles\n", cmt, mw, mh);
			tmp= rec_size(&gr->_meta_rec);
			sprintf(str2, "%d + ", tmp);
			strcat(str, str2);
			size += tmp;
		}
		if(gr->img_flags & GRIT_IMG_SHARED)
			fprintf(fp, "%s\tExternal tile file: %s.\n", cmt, 
				gr->shared->path);
	}
	
	tmp= strlen(str);
	if(tmp>2)
	{
		str[tmp-2]= '=';
		fprintf(fp, "%s\tTotal size: %s%d\n", cmt, str, size);
	}

	time( &aclock );
	newtime = localtime( &aclock );
	strftime(str, 96, "%Y-%m-%d, %H:%M:%S", newtime);

	fprintf(fp, "%s\n%s\tTime-stamp: %s\n", cmt, cmt, str);

	const char *curr= grit_app_string, *prev= curr;
	while( (curr= strchr(prev, '\n')) != NULL )
	{
		fputs(cmt, fp);
		fwrite(prev, curr+1-prev, 1, fp);
		prev= curr+1;
	}
	fprintf(fp, "%s%s\n", cmt, prev);
	//fprintf(fp, "%s\tExported by Cearn's GBA Image Transmogrifier\n", cmt);
	//fprintf(fp, "%s\t( http://www.coranac.com )\n", cmt);
	fprintf(fp, "%s\n%s\n", cmt, hline);

	return TRUE;
}

// EOF
