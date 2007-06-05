//
//! \file winglue.h
//!  Required windows stuff for portability's sake
//!  Mostly ripped from FreeImage.h
//! \date 20060725 - 20070403
//! \author cearn
//=== NOTES ===
// * The switch-define is either _MSC_VER or _WIN32, I'm not sure which. 

#ifndef __WINGLUE_H__
#define __WINGLUE_H__


#ifdef _MSC_VER

// --------------------------------------------------------------------
// WINDOWS/Visual C++ platform
// --------------------------------------------------------------------

#include <windows.h>

#ifndef PATH_MAX
#define PATH_MAX _MAX_PATH
#endif

// <sys/param.h> functionality
#ifndef _SYS_PARAM_H
#define _SYS_PARAM_H

#include <limits.h>

/* These are useful for cross-compiling */ 
#define BIG_ENDIAN      4321
#define LITTLE_ENDIAN   1234
#define BYTE_ORDER      LITTLE_ENDIAN

#define MAXPATHLEN      PATH_MAX

#endif	// _SYS_PARAM_H

#define strcasecmp stricmp		// 


#else	// _MSC_VER


// --------------------------------------------------------------------
// NON-WINDOWS platform
// --------------------------------------------------------------------


// This defines BYTE_ORDER etc. If it doesn't exist, just C&P from 
// above
#include <sys/param.h>	

#include <stdlib.h>
#include <string.h>

#include <FreeImage.h>

// --- BASE TYPES -----------------------------------------------------

typedef unsigned char UCHAR;
typedef unsigned short USHORT;
typedef unsigned long UINT;
// rest is defined in FreeImage.h

typedef DWORD COLORREF;

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

typedef struct _RECT
{
	LONG left;
	LONG top;
	LONG right;
	LONG bottom;
} RECT;

// --- BITMAP STUFF ---------------------------------------------------
 

// --- INLINES --------------------------------------------------------

static inline COLORREF RGB( BYTE red, BYTE green, BYTE blue )
{
	return ( (blue << 16) | ( green << 8 ) | red );
}

static inline BYTE GetRValue( COLORREF color )
{
	return (BYTE)(color & 0xFF);
}

static inline BYTE GetGValue( COLORREF color )
{
	return (BYTE)(( color >> 8 )& 0xFF);
}

static inline BYTE GetBValue( COLORREF color )
{
	return (BYTE)(( color >> 16 ) & 0xFF);
}


#endif	// _MSC_VER


#endif	//__WINGLUE_H__

// EOF
