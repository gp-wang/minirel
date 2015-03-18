%{
//
// parser.y: yacc specification for simple SQL-like query language
//

#include <stdlib.h>
#include <stdio.h>
#include "heapfile.h"
#include "parse.h"

extern "C" int isatty(int);
extern int yylex();
extern int yywrap();
extern void reset_scanner();
extern void quit();

void yyerror(char *);

extern char *yytext;                    // tokens in string format
static NODE *parse_tree;                // root of parse tree
%}

%union{
  int ival;
  float rval;
  char *sval;
  NODE *n;
}

%token  	RW_CREATE
		RW_BUILD
		RW_REBUILD
		RW_DROP
		RW_DESTROY
		RW_PRINT
		RW_LOAD
		RW_HELP
		RW_QUIT
		RW_SELECT
		RW_INTO
		RW_WHERE
		RW_INSERT
		RW_DELETE
		RW_PRIMARY
		RW_NUMBUCKETS
		RW_ALL
		RW_FROM
		RW_AS
		RW_TABLE
		RW_AND
		RW_OR
		RW_NOT
		RW_VALUES	
		INT_TYPE
		REAL_TYPE
		CHAR_TYPE	
		T_EQ
		T_LT
		T_LE
		T_GT
		T_GE
		T_EQ
		T_NE
		T_EOF
    		NOTOKEN

%token	<ival>	T_INT

%token	<rval>	T_REAL

%token	<sval>	T_STRING
		T_QSTRING
		T_SHELL_CMD

%type	<ival>	op

%type	<sval>	opt_into_relname
		opt_relname
		string

%type	<n>	command
		query
		insert
		delete
		create
		destroy
		build
/*
		rebuild
*/
		drop
		load
		print
		help
		quit
		opt_primary_attr
		opt_where
		qual
		selection
		join
		non_mt_qualattr_list
		qualattr
/*
		non_mt_attrval_list
		attrval
*/
		non_mt_attrtype_list
		attrtype
		value
		attrib
		attrib_list
		value_list
		val
		table_list
		table
%%

start
	: command ';'
	{
		parse_tree = $1;
		YYACCEPT;
	}
	| T_SHELL_CMD
	{
	        if(!isatty(0))
		    puts($1);
		(void)system($1);
		parse_tree = NULL;
		YYACCEPT;
	}
	| error
	{
		reset_scanner();
		parse_tree = NULL;
		YYACCEPT;
	}
	| T_EOF
	{
		quit();
	}
	;

command
	: query
	| insert
	| delete
	| create
	| destroy
	| build
/*
	| rebuild
*/
	| drop
	| load
	| print
	| help
	| quit
	| nothing
	{
		$$ = NULL;
	}
	;

query
	: RW_SELECT non_mt_qualattr_list opt_into_relname RW_FROM table_list opt_where
/*	RW_SELECT opt_into_relname '(' non_mt_qualattr_list ')' opt_where */
	{
		NODE *where;
		NODE *qualattr_list = replace_alias_in_qualattr_list($5, $2);
		if (qualattr_list == NULL) { // something wrong in qualattr_list
		  $$ = NULL;
		}
		else {
		  where = replace_alias_in_condition($5, $6);
		  if ((where == NULL) && ($6 != NULL)) {
		     $$ = NULL; //something wrong in where condition
		  }
		  else {
		    $$ = query_node($3, qualattr_list, where);
		  }
		}
	}
	;

table_list
	: '(' table_list ')'
	{
		$$ = $2;
	}
	| table ',' table_list
	{
		$$ = prepend($1, $3);
	}
	| table
	{
		$$ = list_node($1);
	}
	
table
	: string  string  /* relation alias */
	{
		$$ = alias_node($1, $2);
	}
	| string /* no alias */
	{
		$$ = alias_node($1, NULL);
	}

insert
	: RW_INSERT RW_INTO string '(' attrib_list ')' RW_VALUES '(' value_list ')' 
	{
		NODE* tmp = merge_attr_value_list($5, $9);
		if (tmp == NULL) $$=NULL;
		else $$ = insert_node($3, tmp);
	}
	;

attrib_list
	: attrib ',' attrib_list
	{
		$$ = prepend($1, $3);
	}
	| attrib
	{
		$$ = list_node($1);
	}

attrib
	: string
	{
		$$ = attrval_node($1, NULL);
	}

value_list
	: val ',' value_list
	{
		$$ = prepend($1, $3);
	}
	| val
	{
		$$ = list_node($1);
	}

val
	: value 
	{
		$$ = $1;
	}

delete
	: RW_DELETE RW_FROM string opt_where
	{ 
		$$ = delete_node($3, $4);
	}
	;

create
	: RW_CREATE RW_TABLE string '(' non_mt_attrtype_list ')' opt_primary_attr
	{
		$$ = create_node($3, $5, $7);
	}
	;

destroy
	: RW_DESTROY RW_TABLE string
	{
		$$ = destroy_node($3);
	}
	;

build
	: RW_BUILD string '(' string ')'
	{
		$$ = build_node($2, $4, 0);
	}
	;

/*
rebuild
	: RW_REBUILD string '(' string ')' RW_NUMBUCKETS T_EQ T_INT
	{
		$$ = rebuild_node($2, $4, $8);
	}
	;
*/

drop
	: RW_DROP string '(' string ')'
	{
		$$ = drop_node($2, $4);
	}
	| RW_DROP string
	{
		$$ = drop_node($2, NULL);
	}
	;

load
	: RW_LOAD RW_TABLE string RW_FROM '(' T_QSTRING ')'
	{
		$$ = load_node($3, $6);
	}
	;
print
	: RW_PRINT RW_TABLE string
	{
		$$ = print_node($3);
	}
	;

help
	: RW_HELP opt_relname
	{
		$$ = help_node($2);
	}
	;

quit
	: RW_QUIT ';'
	{
		quit();
	}
	;

opt_primary_attr
	: RW_PRIMARY string RW_NUMBUCKETS T_EQ T_INT
	{
		$$ = primattr_node($2, $5);
	}
	| nothing
	{
		$$ = NULL;
	}
	;

opt_into_relname
	: RW_INTO string
	{
		$$ = $2;
	}
	| nothing
	{
		$$ = NULL;
	}
	;

opt_relname
	: RW_TABLE string
	{
		$$ = $2;
	}
	| nothing
	{
		$$ = NULL;
	}
	;
	
opt_where
	: RW_WHERE qual
	{
		$$ = $2;
	}
	| nothing
	{
		$$ = NULL;
	}
	;

qual
	: selection
	| join
	;

selection
	: qualattr op value
	{
		$$ = select_node($1, $2, $3);
	}
	;

join
	: qualattr op qualattr
	{
		$$ = join_node($1, $2, $3);
	}
	;

non_mt_qualattr_list
	: '(' non_mt_qualattr_list ')'
	{
		$$ = $2;
	}
	| qualattr ',' non_mt_qualattr_list
	{
		$$ = prepend($1, $3);
	}
	| qualattr
	{
		$$ = list_node($1);
	}
	;

qualattr
	: string '.' string
	{
		$$ = qualattr_node($1, $3);
	}
	| string
	{
		$$ = qualattr_node(NULL, $1);
	}
	;
/*
non_mt_attrval_list
	: attrval ',' non_mt_attrval_list
	{
		$$ = prepend($1, $3);
	}
	| attrval
	{
		$$ = list_node($1);
	}
	;

attrval
	: string T_EQ value
 	{
		$$ = attrval_node($1, $3);
	}
	;
*/
non_mt_attrtype_list
	: attrtype ',' non_mt_attrtype_list
	{
		$$ = prepend($1, $3);
	}
	| attrtype
	{
		$$ = list_node($1);
	}
	;

attrtype
	: string INT_TYPE
 	{
		$$ = attrtype_node($1, 'i'-128);
	}
	| string REAL_TYPE
	{
		$$ = attrtype_node($1, 'f'-128);
	}
	| string CHAR_TYPE '(' value ')'
	{
	 	$$ = attrtype_node($1, $4->u.VALUE.u.ival);
	}
	| string CHAR_TYPE
	{
		$$ = attrtype_node($1, 2);
	}
	;

op
	: T_LT
	{
		$$ = LT;
	}
	| T_LE
	{
		$$ = LTE;
	}
	| T_GT
	{
		$$ = GT;
	}
	| T_GE
	{
		$$ = GTE;
	}
	| T_EQ
	{
		$$ = EQ;
	}
	| T_NE
	{
		$$ = NE;
	}
	;

value
	: T_INT
	{
		$$ = int_node($1);
	}
	| T_REAL
	{
		$$ = float_node($1);
	}
	| T_QSTRING
	{
		$$ = string_node($1);
	}
	;

string
	: T_STRING
	{
		$$ = $1;
	}
	;

nothing
	: /* epsilon */
	;

%%

void parse(void)
{
  extern void new_query();
  extern void interp(NODE *);

  for(;;){

    // reset parser and scanner for a new query
    new_query();

    // print a prompt
    printf("%s", PROMPT);
    fflush(stdout);

    // if a query was successfully read, interpret it
    if(yyparse() == 0 && parse_tree != NULL)
      interp(parse_tree);
  }
}


void yyerror(char *s)
{
  puts(s);
}

