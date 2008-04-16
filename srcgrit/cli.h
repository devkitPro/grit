

#ifndef _CLI_PARSER_H_
#define _CLI_PARSER_H_


#include <vector>

typedef std::vector<char*> strvec;

#define CLI_BOOL(_x) cli_bool(_x, opts)
#define CLI_INT(_x, _y) cli_int(_x, opts, _y)
#define CLI_STR(_x, _y) cli_str(_x, opts, _y)

int cli_find_key(const char *key, const strvec &opts);
int cli_int(const char *key, const strvec &opts, int dflt);
bool cli_bool(const char *key, const strvec &opts);
char *cli_str(const char *key, const strvec &opts, char *dflt);


/*
int cli_find_key(const char *key, int argc, char **argv);
int cli_int(const char *key, int argc, char **argv, int dflt);
bool cli_bool(const char *key, int argc, char **argv);
char *cli_str(const char *key, int argc, char **argv, char *dflt);
*/

#endif

// EOF
