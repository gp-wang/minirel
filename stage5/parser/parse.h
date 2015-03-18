#ifndef PARSE_H
#define PARSE_H



//
// ATTR_DESCR: attribute descriptor
//

typedef struct {
  char *attrName;                       // relation name
  int attrType;                         // type of attribute
  int attrLen;                          // length of attribute
} ATTR_DESCR;


//
// REL_ATTR: describes a qualified attribute (relName.attrName)
//

typedef struct {
  char *relName;                        // relation name
  char *attrName;                       // attribute name
} REL_ATTR;

//
// ATTR_VAL: <attribute, value> pair
//

typedef struct {
  char *attrName;                       // attribute name
  int valType;                          // type of value
  int valLength;                        // length if type = STRING_TYPE
  void *value;                          // value for attribute
} ATTR_VAL;


#define INTCHAR   'i'
#define FLOATCHAR 'f'
#define STRCHAR   's'
#define PROMPT	  "\n>>> "


//
// all the available kinds of nodes
//

typedef enum {
    N_QUERY,
    N_INSERT,
    N_DELETE,
    N_CREATE,
    N_DESTROY,
    N_BUILD,
    N_REBUILD,
    N_DROP,
    N_LOAD,
    N_PRINT,
    N_HELP,
    N_SELECT,
    N_JOIN,
    N_PRIMATTR,
    N_QUALATTR,
    N_ATTRVAL,
    N_ATTRTYPE,
    N_VALUE,
    N_LIST,
    N_ALIAS
} NODEKIND;


//
// structure of parse tree nodes
//

typedef struct node {
    NODEKIND kind;

    union {
	// query node */
	struct {
	    char *relname;
	    struct node *attrlist;
	    struct node *qual;
	} QUERY;

	// insert node */
	struct {
	    char *relname;
	    struct node *attrlist;
	} INSERT;

	// delete node */
	struct {
	    char *relname;
	    struct node *qual;
	} DELETE;

	// create node */
	struct {
	    char *relname;
	    struct node *attrlist;
	    struct node *primattr;
	} CREATE;

	// destroy node */
	struct {
	    char *relname;
	} DESTROY;

	// re/build node */
	struct {
	    char *relname;
	    char *attrname;
	    int nbuckets;
	} BUILD;

	// drop node */
	struct {
	    char *relname;
	    char *attrname;
	} DROP;

	// load node */
	struct {
	    char *relname;
	    char *filename;
	} LOAD;

	// pprint node */
	struct {
	    char *relname;
	} PRINT;

	// help node */
	struct {
	    char *relname;
	} HELP;

	// select node */
	struct {
	    struct node *selattr;
	    int op;
	    struct node *value;
	} SELECT;

	// join node */
	struct {
	    struct node *joinattr1;
	    int op;
	    struct node *joinattr2;
	} JOIN;

	// qualified attribute node */
	struct {
	    char *relname;
	    char *attrname;
	} QUALATTR;

	// primary attribute node */
	struct {
	    char *attrname;
	    int nbuckets;
	} PRIMATTR;

	// <attribute, value> pair */
	struct {
	    char *attrname;
	    struct node *value;
	} ATTRVAL;

	// <attribute, type> pair */
	struct {
	    char *attrname;
	    int  type;
	    // type of the attribute
	    // 'i'-128 means integer
	    // 'f'-128 means real
	    // otherwise means length of string

	   // char *type;
	} ATTRTYPE;

	// <value, type> pair */
	struct {
	    int type;
	    int len;
	    union {
		int ival;
		float rval;
		char *sval;
	    } u;
	} VALUE;

	// list node */
	struct {
	  struct node *self;
	  struct node *next;
	} LIST;
	
	// Alias node */
	struct {
	  char *relname;
	  char *alias;
	} ALIAS;
    } u;
} NODE;


//
// function prototypes
//

NODE *newnode(int kind);
NODE *query_node(char *relname, NODE *attrlist, NODE *n);
NODE *insert_node(char *relname, NODE *attrlist);
NODE *delete_node(char *relname, NODE *qual);
NODE *create_node(char *relname, NODE *attrlist, NODE *primattr);
NODE *destroy_node(char *relname);
NODE *build_node(char *relname, char *attrname, int nbuckets);
NODE *rebuild_node(char *relname, char *attrname, int nbuckets);
NODE *drop_node(char *relname, char *attrname);
NODE *load_node(char *relname, char *filename);
NODE *print_node(char *relname);
NODE *help_node(char *relname);
NODE *select_node(NODE *selattr, int op, NODE *value);
NODE *join_node(NODE *joinattr1, int op, NODE *joinattr2);
NODE *qualattr_node(char *relname, char *attrname);
NODE *primattr_node(char *attrname, int nbuckets);
NODE *attrval_node(char *attrname, NODE *value);
//attrtype_node need to change due to change of NODE.ATTRTYPE
NODE *attrtype_node(char *attrname, int type /*char *type*/);
NODE *int_node(int ival);
NODE *float_node(float rval);
NODE *string_node(char *s);
NODE *list_node(NODE *n);
NODE *prepend(NODE *n, NODE *list);
NODE *merge_attr_value_list(NODE *attr_list, NODE *value_list);
NODE *alias_node(char *relname, char *alias);
NODE *replace_alias_in_qualattr_list(NODE *alias, NODE *qualattr_list);
NODE *replace_alias_in_condition(NODE *alias, NODE *where);
#endif
