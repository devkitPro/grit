//
// Main compression routine file.
//
//! \file cprs.cpp
//! \author J Vijn
//! \date 20080208 - 20080208
//
/* === NOTES ===
  * There are only two things you'd need to make this completely 
	separate: RECORD and the GBA types.
  * PONDER: since not all of these things are actually compression, 
	a namecange may be in order at some point :| 
*/

#include "cprs.h"


// --------------------------------------------------------------------
// FUNCTIONS 
// --------------------------------------------------------------------

//! Create the compression header word (little endian)
u32	cprs_create_header(uint size, u8 tag)
{
	u8 data[4];

	data[0]= tag;
	data[1]= (size    ) & 255;
	data[2]= (size>> 8) & 255;
	data[3]= (size>>16) & 255;
	return *(u32*)data;
}

//! compression dispatcher.
bool cprs_compress(RECORD *dst, const RECORD *src, ECprsTag tag)
{
	if(dst==NULL || src==NULL)
		return false;

	bool bOK= false;

	switch(tag)
	{
	case CPRS_NONE_TAG:
		bOK= cprs_none(dst, src) != 0;			break;

	case CPRS_LZ77_TAG:
		bOK= cprs_gba_lz77(dst, src) != 0;		break;

	//CPRS_HUF4_TAG
	case CPRS_HUFF8_TAG:
		bOK= cprs_gba_huff(dst, src, 8) != 0;	break;

	case CPRS_RLE_TAG:
		bOK= cprs_gba_rle8(dst, src) != 0;		break;


	//CPRS_DIFF8_TAG
	//CPRS_DIFF16_TAG
	default:
		return false;
	}

	return bOK;
}

//! Don't compress, but still add a compression-like header.
uint cprs_none(RECORD *dst, const RECORD *src)
{
	uint srcS= src->width * src->height;
	uint dstS= ALIGN4(srcS)+4;

	u8 *dstD= (u8*)malloc(dstS);
	if(dstD == NULL)
		return 0;

	*(u32*)dstD= cprs_create_header(srcS, CPRS_NONE_TAG);
	memcpy(dstD+4, src->data, srcS);

	free(dst->data);	
	dst->width= 1;
	dst->height= dstS;
	dst->data= dstD;

	return dstS;
}

// EOF
