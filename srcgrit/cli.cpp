//
//! \file cli.cpp
//!  Command line interface
//! \date 20050823 - 20070725
//! \author gauauu, cearn
// === NOTES ===

#include "cli.h"
#include <string.h>
#include <stdlib.h>


int cli_find_key(const char *key, int argc, char **argv)
{
	int ii;
	for(ii=1; ii<argc; ii++)
		if(strncmp(key, argv[ii], strlen(key)) == 0)
			return ii;
	return argc;
}

int cli_bool(const char *key, int argc, char **argv)
{
	int pos = cli_find_key(key, argc, argv);
	if(pos >= argc)
		return 0;
	return 1;
}

int cli_int(const char *key, int argc, char **argv, int deflt)
{
	int pos = cli_find_key(key, argc, argv);
	if(pos >= argc)
		return deflt;

	char *str= &argv[pos][strlen(key)];

	if(*str != '\0')			// attached field
		return strtoul(str, NULL, 0);
	if(pos == argc-1)			// separate field, but OOB
		return deflt;
	return atoi(argv[pos+1]);
}

char *cli_str(const char *key, int argc, char **argv, char *deflt)
{
	int pos = cli_find_key(key, argc, argv);
	if(pos >= argc)
		return deflt;

	char *str= &argv[pos][strlen(key)];

	if(*str != '\0')			// attached field
		return str;
	if(pos == argc-1)			// separate field, but OOB
		return deflt;
	return argv[pos+1];
}

// TODO cli_range ... somehow :P

// EOF
