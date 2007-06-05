//
//! \file grit_shared.cpp
//!   GRIT_SHARED routines
//! \date 20050814 - 20070227
//! \author cearn
//
// === NOTES === 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//#include <sys/param.h>

#include <cldib.h>
#include "grit.h"



// --------------------------------------------------------------------
// GRIT_SHARED functions
// --------------------------------------------------------------------

//! GRIT_SHARED constructor
GRIT_SHARED *grs_alloc()
{
	GRIT_SHARED *grs= (GRIT_SHARED*)malloc(sizeof(GRIT_SHARED));
	if(grs == NULL)
		return NULL;

	memset(grs, 0, sizeof(GRIT_SHARED));

	return grs;
}

//! GRIT_SHARED constructor
void grs_free(GRIT_SHARED *grs)
{
	if(grs == NULL)
		return;

	grs_clear(grs);
	free(grs);
}

//! Clear GRIT_SHARED struct
void grs_clear(GRIT_SHARED *grs)
{
	free(grs->sym_name);	
	free(grs->path);
	dib_free(grs->dib);
	free(grs->pal_rec.data);
	
	memset(&grs, 0, sizeof(GRIT_SHARED));
}


/*
//! Run exporter with shared data
void grs_run(GRIT_REC *grs, GRIT_REC *gr_base)
{
	//# Make copy of gr_base for flags, etc

	//# Attach shared data

	//# Run for shared gr

	//# Detach shared data

	//# delete gr
}
*/
