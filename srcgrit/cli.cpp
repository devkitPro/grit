//
//! \file cli.cpp
//!  Command line interface
//! \date 20050823 - 20080130
//! \author gauauu, cearn
/* === NOTES ===
*/

#include "cli.h"
#include <string.h>

//! Find a key in a string array.
/*!	\return key index or length of array if not found.
*/
int cli_find_key(const char *key, const strvec &opts)
{
	int ii;
	for(ii=1; ii<opts.size(); ii++)
		if(strncmp(key, opts[ii], strlen(key)) == 0)
			break;

	return ii;
}

//! Return whether \a key is present in \a opts.
bool cli_bool(const char *key, const strvec &opts)
{
	return cli_find_key(key, opts) < opts.size();
}

//! Return the integer following \a key, or \a dflt if \a key not found.
int cli_int(const char *key, const strvec &opts, int dflt)
{
	int pos = cli_find_key(key, opts);
	if(pos >= opts.size())
		return dflt;

	char *str= &opts[pos][strlen(key)];

	if(*str != '\0')					// attached field
		return strtoul(str, NULL, 0);

	if(pos == opts.size()-1)			// separate field, but OOB
		return dflt;

	return strtoul(opts[pos+1], NULL, 0);
}

//! Return the string following \a key, or \a dflt if \a key not found.
char *cli_str(const char *key, const strvec &opts, char *dflt)
{
	int pos = cli_find_key(key, opts);
	if(pos >= opts.size())
		return dflt;

	char *str= &opts[pos][strlen(key)];
	if(*str != '\0')					// attached field
		return str;

	if(pos == opts.size()-1)			// separate field, but OOB
		return dflt;

	return opts[pos+1];
}

// TODO cli_range ... somehow :P

// EOF
