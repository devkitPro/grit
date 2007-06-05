//
//! \file cldib_tools.h
//!  Tools header file
//! \date 20050930 - 20060725
//! \author cearn

#ifndef __CLDIB_TOOLS_H__
#define __CLDIB_TOOLS_H__

#include "cldib_core.h"

// --------------------------------------------------------------------
// CONSTANTS
// --------------------------------------------------------------------

/*!	\addtogroup grpDibConv
*	\{
*/

//! bit(un)pack flags
/*!	\ingroup grpDataConv
*/
typedef enum eBUP
{
	BUP_BEBIT= (1<<30),	//!< Bit-chunks inside bytes are grouped big-endian.
	BUP_BASE0= (1<<31)	//!< Offset applies to 0 chunks too.
} eBUP;

/*!	\}	*/

// --------------------------------------------------------------------
// CLASSES
// --------------------------------------------------------------------



// --------------------------------------------------------------------
// PROTOTYPES
// --------------------------------------------------------------------

// --- MISC (cldib_tools.cpp) ------------------------------------

/*!	\addtogroup grpColor
*	\{
*/
DWORD rgb_dist(const RGBQUAD *a, const RGBQUAD *b);
RGBQUAD str2rgb(const char *str);

/*!	\}	*/


/*!	\addtogroup grpDibTools
*	\{
*/

CLDIB *dib_redim(CLDIB *src, int tileW, int tileH, int tileN);
BOOL data_redim(const RECORD *src, RECORD *dst, int tileH, int tileN);

//! \name Redox functions
//\{
int dib_pal_reduce(CLDIB *dib, RECORD *extPal);
// dib_tile_reduce(...)
// dib_tile_oxidize(...)
//\}

/*!	\}	*/

// --- CONVERSION (cldib_conv.cpp) ------------------------------------

/*!	\addtogroup grpDibConv
*	\{
*/

//! \name DIB converters
// \{
CLDIB *dib_convert(CLDIB *src, int dstB, DWORD base);
CLDIB *dib_bit_unpack(CLDIB *src, int dstB, DWORD base);
CLDIB *dib_8_to_true(CLDIB *src, int dstB);
CLDIB *dib_true_to_true(CLDIB *src, int dstB);
CLDIB *dib_true_to_8(CLDIB *src, int nclrs);
// \}


//!	\name Data converters
// \{

INLINE WORD swap_word(WORD nn);
INLINE DWORD swap_dword(DWORD nn);

BOOL data_bit_unpack(void *dstv, const void *srcv, 
	int srcS, int srcB, int dstB, DWORD base);
BOOL data_bit_pack(void *dstv, const void *srcv, 
	int srcS, int srcB, int dstB, DWORD base);
BOOL data_bit_rev(void *dstv, const void *srcv, int len, int bpp);
BOOL data_byte_rev(void *dstv, const void *srcv, int len, int chunk);

BOOL data_8_to_true(void *dstv, const void *srcv, int srcS, 
	int dstB, RGBQUAD *pal);
BOOL data_true_to_true(void *dstv, const void *srcv, int srcS, 
	int srcB, int dstB);

// \}

/*!	\}	*/


// --- COLOR ADJUSTMENT (cldib_adjust.cpp) ----------------------------

/*!	\addtogroup grpColor	*/
/*!	\{	*/

BOOL dib_invert(CLDIB *dib);

//! \name Main adjustment routines
// \{

BOOL dib_adjust(CLDIB *dib, BYTE lut[], enum eClrChannel cce);
BOOL data_adjust(BYTE *data, BYTE lut[], enum eClrChannel cce, 
	int nn, int ofs);

// \}


//! \name Lut initialisers
// \{

BOOL dib_lut_brightness(BYTE lut[], double perc);
BOOL dib_lut_contrast(BYTE lut[], double perc);
BOOL dib_lut_gamma(BYTE lut[], double gamma);
BOOL dib_lut_invert(BYTE lut[]);
BOOL dib_lut_intensity(BYTE lut[], double perc);
BOOL dib_lut_linear(BYTE lut[], double a, double b);

// \}

//!	\name Color replacers
// \{
BOOL dib_pal_replace(CLDIB *dib, DWORD dst[], DWORD src[], int nn);
BOOL dib_pixel_replace(CLDIB *dib, DWORD dst[], DWORD src[], int nn);
// \}

/*	\}	/addtogroup grpColor	*/


// --------------------------------------------------------------------
// FILE READ/WRITE INTERFACE
// --------------------------------------------------------------------


/*!	\addtogroup grpMisc
*	The file routines don't actually provide any functionality of 
*	their own, just an interface. Write your own routines, then attach 
*	them with <code>dib_set_<i>foo</i>_proc</code>.
*	\{
*/

//!	\name	File read/write interface
// \{

//! General file-reader type
typedef CLDIB *(*fnDibLoad)(const char *fpath, void *extra);
 
//! General file-writer type
typedef BOOL (*fnDibSave)(const CLDIB *dib, const char *fpath, void *extra);


extern fnDibLoad dib_load;
extern fnDibSave dib_save;

fnDibLoad dib_set_load_proc(fnDibLoad proc);
fnDibSave dib_set_save_proc(fnDibSave proc);

CLDIB *dib_load_dflt(const char *fpath, void *extra);
BOOL dib_save_dflt(const CLDIB *dib, const char *fpath, void *extra);

//\}

/*!	\}	*/


// --------------------------------------------------------------------
// INLINES
// --------------------------------------------------------------------


//! Swap the bytes in a WORD
INLINE WORD swap_word(WORD xx)
{
	return (xx>>8) | ((xx&255)<<8);
}

//! Swap the bytes in a DWORD
INLINE DWORD swap_dword(DWORD xx)
{
	return ((xx&0xFF)<<24) | ((xx&0xFF00)<<8) | 
		((xx>>8)&0xFF00) | ((xx>>24)&0xFF);
}

#endif // __CLDIB_TOOLS_H__

// EOF
