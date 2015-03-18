#include "heapfile.h"
#include "parse.h"
#include "y.tab.h"
#include <string.h>
#include  <stdio.h>

//
// total number of nodes available for a given parse-tree
//

#define MAXNODE	100

static NODE nodepool[MAXNODE];
static int nodeptr = 0;

static char *find_match_in_alias(NODE* alias, char *rel_alias);

//
// reset_parser: resets the scanner and parser when a syntax error occurs
//
// No return value
//

void reset_parser(void)
{
  extern void reset_scanner();
  reset_scanner();
  nodeptr = 0;
}


static void (*cleanup_func)() = NULL;


//
// new_query: prepares for a new query by releasing all resources used
// by previous queries.
//
// No return value.
//

void new_query(void)
{
  extern void reset_charptr();
  nodeptr = 0;
  reset_charptr();
  if(cleanup_func)
    (*cleanup_func)();
}


void register_cleanup_function(void (*func)())
{
  cleanup_func = func;
}


//
// newnode: allocates a new node of the specified kind and returns a pointer
// to it on success.  Returns NULL on error.
//

NODE *newnode(int kind)
{
  NODE *n;

  // if we've used up all of the nodes then error
  if(nodeptr == MAXNODE){
    cerr << "Out of Memory !" << endl;
    exit(1);
  }

  // get the next node
  n = nodepool + nodeptr;
  ++nodeptr;
  
  // initialize the `kind' field
  n->kind = (NODEKIND)kind;
  return n;
}


//
// query_node: allocates, initializes, and returns a pointer to a new
// query node having the indicated values.
//

NODE *query_node(char *relname, NODE *attrlist, NODE *qual)
{
  NODE *n = newnode(N_QUERY);

  n->u.QUERY.relname = relname;
  n->u.QUERY.attrlist = attrlist;
  n->u.QUERY.qual = qual;
  return n;
}


//
// insert_node: allocates, initializes, and returns a pointer to a new
// insert node having the indicated values.
//

NODE *insert_node(char *relname, NODE *attrlist)
{
  NODE *n = newnode(N_INSERT);

  n->u.INSERT.relname = relname;
  n->u.INSERT.attrlist = attrlist;
  return n;
}


//
// delete_node: allocates, initializes, and returns a pointer to a new
// delete node having the indicated values.
//

NODE *delete_node(char *relname, NODE *qual)
{
  NODE *n = newnode(N_DELETE);
  
  n->u.DELETE.relname = relname;
  n->u.DELETE.qual = qual;
  return n;
}


//
// create_node: allocates, initializes, and returns a pointer to a new
// create node having the indicated values.
//

NODE *create_node(char *relname, NODE *attrlist, NODE *primattr)
{
  NODE *n = newnode(N_CREATE);
    
  n->u.CREATE.relname = relname;
  n->u.CREATE.attrlist = attrlist;
  n->u.CREATE.primattr = primattr;
  return n;
}


//
// destroy_node: allocates, initializes, and returns a pointer to a new
// destroy node having the indicated values.
//

NODE *destroy_node(char *relname)
{
  NODE *n = newnode(N_DESTROY);
  
  n->u.DESTROY.relname = relname;
  return n;
}


//
// build_node: allocates, initializes, and returns a pointer to a new
// build node having the indicated values.
//

NODE *build_node(char *relname, char *attrname, int nbuckets)
{
  NODE *n = newnode(N_BUILD);

  n->u.BUILD.relname = relname;
  n->u.BUILD.attrname = attrname;
  n->u.BUILD.nbuckets = nbuckets;
  return n;
}


//
// rebuild_node: allocates, initializes, and returns a pointer to a new
// build node having the indicated values.
//

NODE *rebuild_node(char *relname, char *attrname, int nbuckets)
{
  NODE *n = newnode(N_REBUILD);

  n->u.BUILD.relname = relname;
  n->u.BUILD.attrname = attrname;
  n->u.BUILD.nbuckets = nbuckets;
  return n;
}


//
// drop_node: allocates, initializes, and returns a pointer to a new
// drop node having the indicated values.
//

NODE *drop_node(char *relname, char *attrname)
{
  NODE *n = newnode(N_DROP);

  n->u.DROP.relname = relname;
  n->u.DROP.attrname = attrname;
  return n;
}


//
// load_node: allocates, initializes, and returns a pointer to a new
// load node having the indicated values.
//

NODE *load_node(char *relname, char *filename)
{
  NODE *n = newnode(N_LOAD);
  
  n->u.LOAD.relname = relname;
  n->u.LOAD.filename = filename;
  return n;
}


//
// print_node: allocates, initializes, and returns a pointer to a new
// print node having the indicated values.
//

NODE *print_node(char *relname)
{
  NODE *n = newnode(N_PRINT);

  n->u.PRINT.relname = relname;
  return n;
}


//
// help_node: allocates, initializes, and returns a pointer to a new
// help node having the indicated values.
//

NODE *help_node(char *relname)
{
  NODE *n = newnode(N_HELP);
    
  n->u.HELP.relname = relname;
  return n;
}


//
// select_node: allocates, initializes, and returns a pointer to a new
// select node having the indicated values.
//

NODE *select_node(NODE *selattr, int op, NODE *value)
{
  NODE *n = newnode(N_SELECT);

  n->u.SELECT.selattr = selattr;
  n->u.SELECT.op = op;
  n->u.SELECT.value = value;
  return n;
}


//
// join_node: allocates, initializes, and returns a pointer to a new
// join node having the indicated values.
//

NODE *join_node(NODE *joinattr1, int op, NODE *joinattr2)
{
  NODE *n = newnode(N_JOIN);

  n->u.JOIN.joinattr1 = joinattr1;
  n->u.JOIN.op = op;
  n->u.JOIN.joinattr2 = joinattr2;
  return n;
}


//
// primattr_node: allocates, initializes, and returns a pointer to a new
// join node having the indicated values.
//

NODE *primattr_node(char *attrname, int nbuckets)
{
  NODE *n = newnode(N_PRIMATTR);

  n->u.PRIMATTR.attrname = attrname;
  n->u.PRIMATTR.nbuckets = nbuckets;
  return n;
}


//
// qualattr_node: allocates, initializes, and returns a pointer to a new
// qualattr node having the indicated values.
//

NODE *qualattr_node(char *relname, char *attrname)
{
  NODE *n = newnode(N_QUALATTR);

  n->u.QUALATTR.relname = relname;
  n->u.QUALATTR.attrname = attrname;
  return n;
}


//
// attrval_node: allocates, initializes, and returns a pointer to a new
// attrval node having the indicated values.
//

NODE *attrval_node(char *attrname, NODE *value)
{
  NODE *n = newnode(N_ATTRVAL);

  n->u.ATTRVAL.attrname = attrname;
  n->u.ATTRVAL.value = value;
  return n;
}


//
// attrtype_node: allocates, initializes, and returns a pointer to a new
// attrtype node having the indicated values.
//

NODE *attrtype_node(char *attrname, int type /* char *type */)
{
  NODE *n = newnode(N_ATTRTYPE);

  n->u.ATTRTYPE.attrname = attrname;
  n->u.ATTRTYPE.type = type;
  return n;
}


//
// int_node: allocates, initializes, and returns a pointer to a new
// int node having the indicated values.
//

NODE *int_node(int ival)
{
  NODE *n = newnode(N_VALUE);

  n->u.VALUE.type = INTEGER;
  n->u.VALUE.u.ival = ival;
  n->u.VALUE.len = 0;
  return n;
}


//
// float_node: allocates, initializes, and returns a pointer to a new
// float node having the indicated values.
//

NODE *float_node(float rval)
{
  NODE *n = newnode(N_VALUE);

  n->u.VALUE.type = FLOAT;
  n->u.VALUE.u.rval = (float)rval;
  n->u.VALUE.len = 0;
  return n;
}


//
// string_node: allocates, initializes, and returns a pointer to a new
// string node having the indicated values.
//

NODE *string_node(char *s)
{
  NODE *n = newnode(N_VALUE);

  n->u.VALUE.type = STRING;
  n->u.VALUE.u.sval = s;
  n->u.VALUE.len = strlen(s);
  return n;
}


//
// list_node: allocates, initializes, and returns a pointer to a new
// list node having the indicated values.
//

NODE *list_node(NODE *n)
{
  NODE *list = newnode(N_LIST);

  list->u.LIST.self = n;
  list->u.LIST.next = NULL;
  return list;
}


//
// prepends node n onto the front of list.
//
// Returns the resulting list.
//

NODE *prepend(NODE *n, NODE *list)
{
  NODE *newlist = newnode(N_LIST);

  newlist->u.LIST.self = n;
  newlist->u.LIST.next = list;
  return newlist;
}

//
// alias node 
// store the alias of a relation in a query
//

NODE *alias_node(char *relname, char *alias)
{
  NODE *n = newnode(N_ALIAS);
  
  n->u.ALIAS.relname = relname;
  n->u.ALIAS.alias = alias;
  return n;
}

//
// merge attr_list and value_list to a attrval_list
//
// return the result list
//

NODE *merge_attr_value_list(NODE *attr_list, NODE *value_list)
{
  NODE* attr_ptr= attr_list;
  NODE* value_ptr = value_list;

  while (attr_ptr){
    if (value_ptr == NULL) {
      fprintf(stderr,"Error: Value list is shorter than attr list!\n");
      return NULL;
    }
    attr_ptr->u.LIST.self->u.ATTRVAL.value =  value_ptr->u.LIST.self;
    attr_ptr = attr_ptr->u.LIST.next;
    value_ptr = value_ptr->u.LIST.next;
  }

  if (value_ptr != NULL) {
    fprintf(stderr, "Error: Value list is longer than attr list!\n");
    return NULL;
  }
  return attr_list;
}


//
// find out if the give string matches any relname or alias
//
// return the matched relname or NULL if no match
//
char *find_match_in_alias(NODE *alias, char *rel_alias)
{ 
  NODE *n = alias;
  
  if (rel_alias == NULL) return NULL;
  
  while(n) {
    if (!strcmp(n->u.LIST.self->u.ALIAS.relname, rel_alias)) 
      return rel_alias;
    if (n->u.LIST.self->u.ALIAS.alias) {
      if (!strcmp(n->u.LIST.self->u.ALIAS.alias, rel_alias)) 
        return n->u.LIST.self->u.ALIAS.relname;
    }
    n = n->u.LIST.next;
  }
  
  return NULL;
}

//
// replace the relation alias in a qualification attribute list
// with the relation name
//
// returns the result list
NODE *replace_alias_in_qualattr_list(NODE *alias, NODE *qualattr_list)
{ 
  NODE *n = qualattr_list;
  char *s;
  
  while(n) {
    s = n->u.LIST.self->u.QUALATTR.relname;
    if ((s == NULL)&&(alias->u.LIST.next)) {
      fprintf(stderr, "Error: must have relation qualifier before");
      fprintf(stderr, "attributes if multi-table invovle in the query\n");
      return NULL;
    }
    if (s == NULL) { //one table in query
      n->u.LIST.self->u.QUALATTR.relname = alias->u.LIST.self->u.ALIAS.relname;
    }
    else {
      s = find_match_in_alias(alias, s);
      if (s == NULL) {
      	fprintf(stderr, "Error: relation qualifier %s not found\n", 
      	        n->u.LIST.self->u.QUALATTR.relname);
      	return NULL;
      }
      n->u.LIST.self->u.QUALATTR.relname = s;
    }
    n = n->u.LIST.next;
  }
  
  return qualattr_list;
}

//
// replace the relation alias in a where condition
// with the relation name
//
// returns the result list

NODE *replace_alias_in_condition(NODE *alias, NODE *where)
{
  NODE *n = where;
  char *s;

  if (where==NULL) return NULL;
  
  if (n->kind == N_SELECT) {
    s = n->u.SELECT.selattr->u.QUALATTR.relname;
    if ((s == NULL)&&(alias->u.LIST.next)) {
      fprintf(stderr, "Error: must have relation qualifier before");
      fprintf(stderr, "attributes if multi-table invovle in the query\n");
      return NULL;
    }
    if (s == NULL) { //one table in query
      n->u.SELECT.selattr->u.QUALATTR.relname = 
         alias->u.LIST.self->u.ALIAS.relname;
    }
    else {
      s = find_match_in_alias(alias, s);
      if (s == NULL) {
        fprintf(stderr, "Error: relation qualifier %s not found\n", 
                n->u.SELECT.selattr->u.QUALATTR.relname);
        return NULL;
      }
      n->u.SELECT.selattr->u.QUALATTR.relname = s;
    }
  }
  else { // N_JOIN
    s = n->u.JOIN.joinattr1->u.QUALATTR.relname; //left node
    if ((s == NULL)&&(alias->u.LIST.next)) {
      fprintf(stderr, "Error: must have relation qualifier before");
      fprintf(stderr, "attributes if multi-table invovle in the query\n");
      return NULL;
    }
    if (s == NULL) { //one table in query
      n->u.JOIN.joinattr1->u.QUALATTR.relname = 
         alias->u.LIST.self->u.ALIAS.relname;
    }
    else {
      s = find_match_in_alias(alias, s);
      if (s == NULL) {
        fprintf(stderr, "Error: relation qualifier %s not found\n", 
                n->u.JOIN.joinattr1->u.QUALATTR.relname);
        return NULL;
      }
      n->u.JOIN.joinattr1->u.QUALATTR.relname = s;
    }

    s = n->u.JOIN.joinattr2->u.QUALATTR.relname; //right node
    if ((s == NULL)&&(alias->u.LIST.next)) {
      fprintf(stderr, "Error: must have relation qualifier before");
      fprintf(stderr, "attributes if multi-table invovle in the query\n");
      return NULL;
    }
    if (s == NULL) { //one table in query
      n->u.JOIN.joinattr2->u.QUALATTR.relname = 
         alias->u.LIST.self->u.ALIAS.relname;
    }
    else {
      s = find_match_in_alias(alias, s);
      if (s == NULL) {
        fprintf(stderr, "Error: relation qualifier %s not found\n", 
                n->u.JOIN.joinattr2->u.QUALATTR.relname);
        return NULL;
      }
      n->u.JOIN.joinattr2->u.QUALATTR.relname = s;
    }     
  }
  
  return where;
}
