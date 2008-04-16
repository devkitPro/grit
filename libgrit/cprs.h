//
// Header file for grit compression routines.
//
//! \file grit_cprs.h
//! \author J Vijn
//! \date 20080208 - 20080208
//
/* === NOTES ===
  * There are only two things you'd need to make this completely 
	separate: RECORD and the GBA types.
  * PONDER: since not all of these things are actually compression, 
	a namecange may be in order at some point :| 
*/

#ifndef __GRIT_COMPRESSION__
#define __GRIT_COMPRESSION__

#include "grit_core.h"

// --------------------------------------------------------------------
// CONSTANTS
// --------------------------------------------------------------------


//! Compression type tags.
enum ECprsTag
{
	CPRS_NONE_TAG	= 0x00,		//<! No compression.
	CPRS_LZ77_TAG	= 0x10,		//<! GBA LZ77 compression.
	CPRS_HUFF_TAG	= 0x20, 
//	CPRS_HUFF4_TAG	= 0x24,		//<! GBA Huffman, 4bit.
	CPRS_HUFF8_TAG	= 0x28,		//<! GBA Huffman, 8bit.
	CPRS_RLE_TAG	= 0x30,		//<! GBA RLE compression.
//	CPRS_DIFF8_TAG	= 0x81,		//<! GBA Diff-filter, 8bit.
//	CPRS_DIFF16_TAG	= 0x82,		//<! GBA Diff-filter, 16bit.
};


typedef void (*cprs_proc_t)(RECORD *dst, const RECORD *src);


// --------------------------------------------------------------------
// PROTOTYPES 
// --------------------------------------------------------------------

u32	cprs_create_header(uint size, u8 tag); 

bool cprs_compress(RECORD *dst, const RECORD *src, ECprsTag tag);
//bool cprs_decompress(RECORD *dst, const RECORD *src, ECprsTag tag);


uint cprs_none(RECORD *dst, const RECORD *src);

uint cprs_gba_lz77(RECORD *dst, const RECORD *src);
//uint cprs_gba_huf4(RECORD *dst, const RECORD *src);
uint cprs_gba_huf8(RECORD *dst, const RECORD *src);
uint cprs_gba_rle8(RECORD *dst, const RECORD *src);

uint cprs_gba_huff(RECORD *dst, const RECORD *src, int srcB);


// --------------------------------------------------------------------
// INLINES 
// --------------------------------------------------------------------


#endif // __GRIT_COMPRESSION__

// EOF
