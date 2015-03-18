#include <string.h>

#define MAXCHAR 5000                    // size of buffer of strings

static char charpool[MAXCHAR];          // buffer for string allocation
static int charptr = 0;

static int lower(char *dst, char *src, int max);

//
// string_alloc: returns a pointer to a string of length len if possible
//

static char *string_alloc(int len)
{
  char *s;

  if (charptr + len > MAXCHAR) {
    fprintf(stderr, "out of memory\n");
    exit(1);
  }

  s = charpool + charptr;
  charptr += len;
  
  return s;
}


//
// reset_charptr: releases all memory allocated in preparation for the
// next query.
//
// No return value.
//

void reset_charptr(void)
{
  charptr = 0;
}


//
// reset_scanner: resets the scanner after a syntax error
//
// No return value.
//

void reset_scanner(void)
{
  charptr = 0;
  yyrestart(yyin);
}


//
// get_id: determines whether s is a reserved word, and returns the
// appropriate token value if it is.  Otherwise, it returns the token
// value corresponding to a string.  If s is longer than the maximum token
// length (MAXSTRINGLEN) then it returns NOTOKEN, so that the parser will
// flag an error (this is a stupid kludge).
//

#define MAXSTRINGLEN 50

static int get_id(char *s)
{
  static char string[MAXSTRINGLEN];
  int len;

  if ((len = lower(string, s, MAXSTRINGLEN)) == MAXSTRINGLEN)
    return NOTOKEN;
  if (!strcmp(string, "select"))
    return yylval.ival = RW_SELECT;
  if (!strcmp(string, "insert"))
    return yylval.ival = RW_INSERT;
  if (!strcmp(string, "delete"))
    return yylval.ival = RW_DELETE;
  if (!strcmp(string, "create"))
    return yylval.ival = RW_CREATE;
  if (!strcmp(string, "destroy"))
    return yylval.ival = RW_DESTROY;
  if (!strcmp(string, "buildindex"))
    return yylval.ival = RW_BUILD;
  if (!strcmp(string, "rebuildindex"))
    return yylval.ival = RW_REBUILD;
  if (!strcmp(string, "dropindex"))
    return yylval.ival = RW_DROP;
  if (!strcmp(string, "load"))
    return yylval.ival = RW_LOAD;
  if (!strcmp(string, "print"))
    return yylval.ival = RW_PRINT;
  if (!strcmp(string, "help"))
    return yylval.ival = RW_HELP;
  if (!strcmp(string, "quit"))
    return yylval.ival = RW_QUIT;
  if (!strcmp(string, "into"))
    return yylval.ival = RW_INTO;
  if (!strcmp(string, "where"))
    return yylval.ival = RW_WHERE;
  if (!strcmp(string, "primary"))
    return yylval.ival = RW_PRIMARY;
  if (!strcmp(string, "numbuckets"))
    return yylval.ival = RW_NUMBUCKETS;
  if (!strcmp(string, "all"))
    return yylval.ival = RW_ALL;
  if (!strcmp(string, "from"))
    return yylval.ival = RW_FROM;
  if (!strcmp(string, "as"))
    return yylval.ival = RW_AS;
  if (!strcmp(string, "table"))
    return yylval.ival = RW_TABLE;
  if (!strcmp(string, "and"))
    return yylval.ival = RW_AND;
  if (!strcmp(string, "or"))
    return yylval.ival = RW_OR;
  if (!strcmp(string, "not"))
    return yylval.ival = RW_NOT;
  if (!strcmp(string, "values"))
    return yylval.ival = RW_VALUES;
  if (!strcmp(string, "int"))
    return yylval.ival = INT_TYPE;
  if (!strcmp(string, "real"))
    return yylval.ival = REAL_TYPE;
  if (!strcmp(string, "char"))
    return yylval.ival = CHAR_TYPE;
  yylval.sval = mk_string(s, len);
  return T_STRING;
}


//
// lower: copies src to dst, converting it to lowercase, stopping at the
// end of src or after max characters.
//
// Returns:
// 	the length of dst (which may be less than the length of src, if
// 	    src is too long).
//

static int lower(char *dst, char *src, int max)
{
  int len;
    
  for(len = 0; len < max && src[len] != '\0'; ++len) {
    dst[len] = src[len];
    if (src[len] >= 'A' && src[len] <= 'Z')
      dst[len] += 'a' - 'A';
  }
  dst[len] = '\0';

  return len;
}


//
// get_qstring: removes the quotes from a quoted string, allocates
// space for the resulting string.
//
// Returns:
// 	a pointer to the new string
//

static char *get_qstring(char *qstring, int len)
{
  // replace ending quote with \0
  qstring[len - 1] = '\0';

  // copy everything following beginning quote
  return mk_string(qstring + 1, len - 2);
}


//
// mk_string: allocates space for a string of length len and copies s into
// it.
//
// Returns:
// 	a pointer to the new string
//

static char *mk_string(char *s, int len)
{
  char *copy;

  // allocate space for new string
  if ((copy = string_alloc(len + 1)) == NULL) {
    printf("out of string space\n");
    exit(1);
  }
  
  // copy the string
  strncpy(copy, s, len + 1);
  return copy;
}
