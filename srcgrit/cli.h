

#ifndef _CLI_PARSER_H_
#define _CLI_PARSER_H_

#define CLI_BOOL(_x) cli_bool(_x, argc, argv)
#define CLI_INT(_x, _y) cli_int(_x, argc, argv, _y)
#define CLI_STR(_x, _y) cli_str(_x, argc, argv, _y)

int cli_find_key(const char *key, int argc, char **argv);
int cli_bool(const char *key, int argc, char **argv);
int cli_int(const char *key, int argc, char **argv, int deflt);
char *cli_str(const char *key, int argc, char **argv, char *deflt);

#endif

// EOF
