//
//! \file grit_misc.cpp
//!   Various routines
//! \date 20050814 - 20050903
//! \author  cearn
//
// === NOTES === 

#include "grit.h"


// --------------------------------------------------------------------
// PROTOTYPES
// --------------------------------------------------------------------


int cprs_gba_lz77(RECORD *dst, const RECORD *src);
int cprs_gba_huff(RECORD *dst, const RECORD *src, int srcB);
int cprs_gba_rle8(RECORD *dst, const RECORD *src);


// --------------------------------------------------------------------
// FUNCTIONS
// --------------------------------------------------------------------


//! Compression hub
/*!
	\param dst. Record to compress too
	\param src. Record to compress
	\param flags. Compression type
	\note Aliasing \a dst and \a src is safe.
*/
BOOL grit_compress(RECORD *dst, const RECORD *src, u32 flags)
{
	// ASSERT(dst);
	// ASSERT(src);

	RECORD cprsRec= { 0, 0, NULL };

	switch(flags & GRIT_CPRS_MASK)
	{
	case GRIT_CPRS_LZ77:
		cprs_gba_lz77(&cprsRec, src);
		break;
	case GRIT_CPRS_HUFF:
		// PONDER: what if it fails??
		cprs_gba_huff(&cprsRec, src, 8);
		break;
	case GRIT_CPRS_RLE:
		cprs_gba_rle8(&cprsRec, src);
		break;
	default:
		return FALSE;
	}

	if(cprsRec.data)
		rec_alias(dst, &cprsRec);

	return TRUE;
}

// EOF
