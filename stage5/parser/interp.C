#include <stdio.h>

#include "catalog.h"
#include "query.h"
#include "utility.h"
#include "parse.h"
#include "y.tab.h"


#define E_OK			0
#define E_INCOMPATIBLE		-1
#define E_TOOMANYATTRS		-2
#define E_NOLENGTH		-3
#define E_INVINTSIZE		-4
#define E_INVFLOATSIZE		-5
#define E_INVFORMATSTRING	-6
#define E_INVSTRLEN		-7
#define E_DUPLICATEATTR		-8
#define E_TOOLONG		-9
#define E_STRINGTOOLONG		-10


#define ERRFP			stderr  // error message go here
#define MAXATTRS		40      // max. number of attrs in a relation


//
// prefab arrays of useful types
//

static REL_ATTR qual_attrs[MAXATTRS + 1];
static ATTR_DESCR attr_descrs[MAXATTRS + 1];
static ATTR_VAL ins_attrs[MAXATTRS + 1];
static char *names[MAXATTRS + 1];

static int mk_attrnames(NODE *list, char *attrnames[], char *relname);
static int mk_qual_attrs(NODE *list, REL_ATTR qual_attrs[],
			 char *relname1, char *relname2);
static int mk_attr_descrs(NODE *list, ATTR_DESCR attr_descrs[]);
static int mk_ins_attrs(NODE *list, ATTR_VAL ins_attrs[]);
//static int parse_format_string(char *format_string, int *type, int *len);
static int parse_format_string(int format, int *type, int *len);
static void *value_of(NODE *n);
static int  type_of(NODE *n);
static int  length_of(NODE *n);
static void print_error(char *errmsg, int errval);
static void echo_query(NODE *n);
static void print_qual(NODE *n);
static void print_attrnames(NODE *n);
static void print_attrdescrs(NODE *n);
static void print_attrvals(NODE *n);
static void print_primattr(NODE *n);
static void print_qualattr(NODE *n);
static void print_op(int op);
static void print_val(NODE *n);


static attrInfo attrList[MAXATTRS];
static attrInfo attr1;
static attrInfo attr2;


extern "C" int isatty(int fd);          // returns 1 if fd is a tty device


//
// interp: interprets parse trees
//
// No return value.
//

void interp(NODE *n)
{
  int nattrs;				// number of attributes 
  int type;				// attribute type
  int len;				// attribute length
  int op;				// comparison operator
  NODE *temp, *temp1, *temp2;		// temporary node pointers
  char *attrname;			// temp attribute names
  void *value;			        // temp value	
  int nbuckets;			        // temp number of buckets
  int errval;				// returned error value
  RelDesc relDesc;
  Status status;
  int attrCnt, i, j;
  AttrDesc *attrs;
  string resultName;
  static int counter = 0;

  // if input not coming from a terminal, then echo the query

  if (!isatty(0))
    echo_query(n);

  switch(n->kind) {
  case N_QUERY:

    // First check if the result relation is specified

    if (n->u.QUERY.relname)
      {
	resultName = n->u.QUERY.relname;

	// Check if the result relation exists.
	status = attrCat->getRelInfo(resultName, attrCnt, attrs);
	if (status != OK && status != RELNOTFOUND)
	  {
	    error.print(status);
	    return;
	  }
      }
    else
      {
	resultName = "Tmp_Minirel_Result";

	status = relCat->getInfo(resultName, relDesc);
	if (status != OK && status != RELNOTFOUND)
	  {
	    error.print(status);
	    return;
	  }

	if (status == OK)
	  {
	    error.print(TMP_RES_EXISTS);
	    return;
	  }
      }


    // if no qualification then this is a simple select
    temp = n->u.QUERY.qual;
    if (temp == NULL) {

      // make a list of attribute names suitable for passing to select
      nattrs = mk_attrnames(temp1 = n->u.QUERY.attrlist, names, NULL);
      if (nattrs < 0) {
	print_error("select", nattrs);
	break;
      }

      for(int acnt = 0; acnt < nattrs; acnt++) {
	strcpy(attrList[acnt].relName, names[nattrs]);
	strcpy(attrList[acnt].attrName, names[acnt]);
	attrList[acnt].attrType = -1;
	attrList[acnt].attrLen = -1;
	attrList[acnt].attrValue = NULL;
      }
      
      if (status == RELNOTFOUND)
	{
	  // Create the result relation
	  attrInfo *createAttrInfo = new attrInfo[nattrs];
	  for (i = 0; i < nattrs; i++)
	    {
	      AttrDesc attrDesc;

	      strcpy(createAttrInfo[i].relName, resultName.c_str());
	      strcpy(createAttrInfo[i].attrName, attrList[i].attrName);
	      
	      status = attrCat->getInfo(attrList[i].relName,
					attrList[i].attrName,
					attrDesc);
	      if (status != OK)
		{
		  error.print(status);
		  return;
		}
	      createAttrInfo[i].attrType = attrDesc.attrType;
	      createAttrInfo[i].attrLen = attrDesc.attrLen;
	    }

	  status = relCat->createRel(resultName, nattrs, createAttrInfo);
	  delete []createAttrInfo;

	  if (status != OK)
	    {
	      error.print(status);
	      return;
	    }
	}
      else
	{
	  // Check to see that the attribute types match
	  if (nattrs != attrCnt)
	    {
	      error.print(ATTRTYPEMISMATCH);
	      return;
	    }

	  for (i = 0; i < nattrs; i++)
	    {
	      AttrDesc attrDesc;

	      status = attrCat->getInfo(attrList[i].relName,
					attrList[i].attrName,
					attrDesc);
	      if (status != OK)
		{
		  error.print(status);
		  return;
		}

	      if (attrDesc.attrType != attrs[i].attrType || 
		  attrDesc.attrLen != attrs[i].attrLen)
		{
		  error.print(ATTRTYPEMISMATCH);
		  return;
		}
	    }
	  free(attrs);
	}

      // make the call to QU_Select

      errval = QU_Select(resultName,
			 nattrs,
			 attrList,
			 NULL,
			 (Operator)0,
			 NULL);

      if (errval != OK)
	error.print((Status)errval);
    }

    // if qual is `attr op value' then this is a regular select
    else if (temp->kind == N_SELECT) {
	  
      temp1 = temp->u.SELECT.selattr;

      // make a list of attribute names suitable for passing to select
      nattrs = mk_attrnames(n->u.QUERY.attrlist, names,
			    temp1->u.QUALATTR.relname);
      if (nattrs < 0) {
	print_error("select", nattrs);
	break;
      }

      for(int acnt = 0; acnt < nattrs; acnt++) {
	strcpy(attrList[acnt].relName, names[nattrs]);
	strcpy(attrList[acnt].attrName, names[acnt]);
	attrList[acnt].attrType = -1;
	attrList[acnt].attrLen = -1;
	attrList[acnt].attrValue = NULL;
      }
      
      strcpy(attr1.relName, names[nattrs]);
      strcpy(attr1.attrName, temp1->u.QUALATTR.attrname);
      attr1.attrType = type_of(temp->u.SELECT.value);
      attr1.attrLen = -1;
      attr1.attrValue = (char *)value_of(temp->u.SELECT.value);

      if (status == RELNOTFOUND)
	{
	  // Create the result relation
	  attrInfo *createAttrInfo = new attrInfo[nattrs];
	  for (i = 0; i < nattrs; i++)
	    {
	      AttrDesc attrDesc;

	      strcpy(createAttrInfo[i].relName, resultName.c_str());
	      strcpy(createAttrInfo[i].attrName, attrList[i].attrName);
	      
	      status = attrCat->getInfo(attrList[i].relName,
					attrList[i].attrName,
					attrDesc);
	      if (status != OK)
		{
		  error.print(status);
		  return;
		}
	      createAttrInfo[i].attrType = attrDesc.attrType;
	      createAttrInfo[i].attrLen = attrDesc.attrLen;
	    }

	  status = relCat->createRel(resultName, nattrs, createAttrInfo);
	  delete []createAttrInfo;

	  if (status != OK)
	    {
	      error.print(status);
	      return;
	    }
	}
      else
	{
	  // Check to see that the attribute types match
	  if (nattrs != attrCnt)
	    {
	      error.print(ATTRTYPEMISMATCH);
	      return;
	    }

	  for (i = 0; i < nattrs; i++)
	    {
	      AttrDesc attrDesc;

	      status = attrCat->getInfo(attrList[i].relName,
					attrList[i].attrName,
					attrDesc);
	      if (status != OK)
		{
		  error.print(status);
		  return;
		}

	      if (attrDesc.attrType != attrs[i].attrType || 
		  attrDesc.attrLen != attrs[i].attrLen)
		{
		  error.print(ATTRTYPEMISMATCH);
		  return;
		}
	    }
	  free(attrs);
	}

      // make the call to QU_Select
      char * tmpValue = (char *)value_of(temp->u.SELECT.value);

      errval = QU_Select(resultName,
			 nattrs,
			 attrList,
			 &attr1,
			 (Operator)temp->u.SELECT.op,
			 tmpValue);

      delete [] tmpValue;
      delete [] attr1.attrValue;

      if (errval != OK)
	error.print((Status)errval);
    }

    // if qual is `attr1 op attr2' then this is a join
    else {

      temp1 = temp->u.JOIN.joinattr1;
      temp2 = temp->u.JOIN.joinattr2;

      // make an attribute list suitable for passing to join
      nattrs = mk_qual_attrs(n->u.QUERY.attrlist,
			     qual_attrs,
			     temp1->u.QUALATTR.relname,
			     temp2->u.QUALATTR.relname);
      if (nattrs < 0) {
	print_error("select", nattrs);
	break;
      }

      // set up the joined attributes to be passed to Join
      qual_attrs[nattrs].relName = temp1->u.QUALATTR.relname;
      qual_attrs[nattrs].attrName = temp1->u.QUALATTR.attrname;
      qual_attrs[nattrs + 1].relName = temp2->u.QUALATTR.relname;
      qual_attrs[nattrs + 1].attrName = temp2->u.QUALATTR.attrname;
      
      for(int acnt = 0; acnt < nattrs; acnt++) {
	strcpy(attrList[acnt].relName, qual_attrs[acnt].relName);
	strcpy(attrList[acnt].attrName, qual_attrs[acnt].attrName);
	attrList[acnt].attrType = -1;
	attrList[acnt].attrLen = -1;
	attrList[acnt].attrValue = NULL;
      }
      
      strcpy(attr1.relName, qual_attrs[nattrs].relName);
      strcpy(attr1.attrName, qual_attrs[nattrs].attrName);
      attr1.attrType = -1;
      attr1.attrLen = -1;
      attr1.attrValue = NULL;

      strcpy(attr2.relName, qual_attrs[nattrs+1].relName);
      strcpy(attr2.attrName, qual_attrs[nattrs+1].attrName);
      attr2.attrType = -1;
      attr2.attrLen = -1;
      attr2.attrValue = NULL;

      if (status == RELNOTFOUND)
	{
	  // Create the result relation
	  attrInfo *createAttrInfo = new attrInfo[nattrs];
	  for (i = 0; i < nattrs; i++)
	    {
	      AttrDesc attrDesc;

	      strcpy(createAttrInfo[i].relName, resultName.c_str());

	      // Check if there is another attribute with same name
	      for (j = 0; j < i; j++)
		if (!strcmp(createAttrInfo[j].attrName, attrList[i].attrName))
		  break;

	      strcpy(createAttrInfo[i].attrName, attrList[i].attrName);

	      if (j != i)
		sprintf(createAttrInfo[i].attrName, "%s_%d", 
			createAttrInfo[i].attrName, counter++);
	      
	      status = attrCat->getInfo(attrList[i].relName,
					attrList[i].attrName,
					attrDesc);
	      if (status != OK)
		{
		  error.print(status);
		  return;
		}
	      createAttrInfo[i].attrType = attrDesc.attrType;
	      createAttrInfo[i].attrLen = attrDesc.attrLen;
	    }

	  status = relCat->createRel(resultName, nattrs, createAttrInfo);
	  delete []createAttrInfo;

	  if (status != OK)
	    {
	      error.print(status);
	      return;
	    }
	}
      else
	{
	  // Check to see that the attribute types match
	  if (nattrs != attrCnt)
	    {
	      error.print(ATTRTYPEMISMATCH);
	      return;
	    }

	  for (i = 0; i < nattrs; i++)
	    {
	      AttrDesc attrDesc;

	      status = attrCat->getInfo(attrList[i].relName,
					attrList[i].attrName,
					attrDesc);
	      if (status != OK)
		{
		  error.print(status);
		  return;
		}

	      if (attrDesc.attrType != attrs[i].attrType || 
		  attrDesc.attrLen != attrs[i].attrLen)
		{
		  error.print(ATTRTYPEMISMATCH);
		  return;
		}
	    }
	  free(attrs);
	}

      // make the call to QU_Join

      errval = QU_Join(resultName,
		       nattrs,
		       attrList,
		       &attr1,
		       (Operator)temp->u.JOIN.op,
		       &attr2);

      if (errval != OK)
	error.print((Status)errval);
    }

    if (resultName == string( "Tmp_Minirel_Result"))
      {
	// Print the contents of the result relation and destroy it
	status = UT_Print(resultName);
	if (status != OK)
	  error.print(status);

	status = relCat->destroyRel(resultName);
	if (status != OK)
	  error.print(status);
      }

    break;

  case N_INSERT:

    // make attribute and value list to be passed to QU_Insert
    nattrs = mk_ins_attrs(n->u.INSERT.attrlist, ins_attrs);
    if (nattrs < 0) {
      print_error("insert", nattrs);
      break;
    }
    
    // make the call to QU_Insert
    int acnt;
    for(acnt = 0; acnt < nattrs; acnt++) {
      strcpy(attrList[acnt].relName, n->u.INSERT.relname);
      strcpy(attrList[acnt].attrName, ins_attrs[acnt].attrName);
      attrList[acnt].attrType = (Datatype)ins_attrs[acnt].valType;
      attrList[acnt].attrLen = -1;
      attrList[acnt].attrValue = ins_attrs[acnt].value;
    }
      
    errval = QU_Insert(n->u.INSERT.relname,
		       nattrs,
		       attrList);

    for (acnt = 0; acnt < nattrs; acnt++)
      delete [] attrList[acnt].attrValue;

    if (errval != OK)
      error.print((Status)errval);
    
    break;

  case N_DELETE:

    // set up the name of deletion relation
    qual_attrs[0].relName = n->u.DELETE.relname;
    
    // if qualification given...
    if ((temp1 = n->u.DELETE.qual) != NULL) {
      // qualification must be a select, not a join
      if (temp1->kind != N_SELECT) {
	cerr << "Syntax Error" << endl;
	break;
      }
	    
      temp2 = temp1->u.SELECT.selattr;
/*      
      // make sure attribute in qualification is from deletion rel
      if (strcmp(n->u.DELETE.relname, temp2->u.QUALATTR.relname)) {
	print_error("delete", E_INCOMPATIBLE);
	break;
      }
*/      
      // set up qualification
      attrname = temp2->u.QUALATTR.attrname;
      op = temp1->u.SELECT.op;
      type = type_of(temp1->u.SELECT.value);
      len = length_of(temp1->u.SELECT.value);
      value = value_of(temp1->u.SELECT.value);
    }
    
    // otherwise, set up for no qualification
    else {
      attrname = NULL;
      op = (Operator)0;
      type = 0;
      len = 0;
      value = NULL;
    }

    // make the call to QU_Delete

    if (attrname)
      errval = QU_Delete(n -> u.DELETE.relname,
			 attrname,
			 (Operator)op,
			 (Datatype)type,
			 (char *)value);
    else
      errval = QU_Delete(n -> u.DELETE.relname,
			 "",
			 (Operator)op,
			 (Datatype)type,
			 (char *)value);

    delete [] value;

    if (errval != OK)
      error.print((Status)errval);

    break;

  case N_CREATE:

    // make a list of ATTR_DESCRS suitable for sending to UT_Create
    nattrs = mk_attr_descrs(n->u.CREATE.attrlist, attr_descrs);
    if (nattrs < 0) {
      print_error("create", nattrs);
      break;
    }

    // get info about primary attribute, if there is one
    if ((temp = n->u.CREATE.primattr) == NULL) {
      attrname = NULL;
      nbuckets = 1;
    } else {
      attrname = temp->u.PRIMATTR.attrname;
      nbuckets = temp->u.PRIMATTR.nbuckets;
    }

    for(acnt = 0; acnt < nattrs; acnt++) {
      strcpy(attrList[acnt].relName, n -> u.CREATE.relname);
      strcpy(attrList[acnt].attrName, attr_descrs[acnt].attrName);
      attrList[acnt].attrType = attr_descrs[acnt].attrType;
      attrList[acnt].attrLen = attr_descrs[acnt].attrLen;
      attrList[acnt].attrValue = NULL;
    }
      
    // make the call to UT_Create
    errval = relCat->createRel(n -> u.CREATE.relname,
			       nattrs,
			       attrList);

    if (errval != OK)
      error.print((Status)errval);


    break;

  case N_DESTROY:

    errval = relCat->destroyRel(n -> u.DESTROY.relname);
    if (errval != OK)
      error.print((Status)errval);

    break;

  case N_LOAD:

    errval = UT_Load(n -> u.LOAD.relname, n -> u.LOAD.filename);

    if (errval != OK)
      error.print((Status)errval);

    break;

  case N_PRINT:

    errval = UT_Print(n -> u.PRINT.relname);

    if (errval != OK)
      error.print((Status)errval);

    break;
    
  case N_HELP:

    if (n -> u.HELP.relname)
      errval = relCat->help(n -> u.HELP.relname);
    else
      errval = relCat->help("");

    if (errval != OK)
      error.print((Status)errval);

    break;

  default:                              // so that compiler won't complain
    assert(0);
  }
}


//
// mk_attrnames: converts a list of qualified attributes (<relation,
// attribute> pairs) into an array of char pointers so it can be
// sent to the appropriate UT or QU function.
//
// All of the attributes should come from relation relname.  If relname
// is NULL, then it checks that all attributes come from the same
// relation.
//
// The first element of the array after the last attribute name is
// set to the name of the relation.
// 
// Returns:
// 	the length of the list on success ( >= 0 )
// 	error code otherwise ( < 0 )
//
// (Thus, the return code is both the number of attributes in the array,
// and the index of the relatin name in the array).
//

static int mk_attrnames(NODE *list, char *attrnames[], char *relname)
{
  int i;
  NODE *temp;

  // for each qualified attribute in the list...
  for(i = 0; list != NULL && i < MAXATTRS; ++i, list = list->u.LIST.next) {
    temp = list->u.LIST.self;

    // if relname is NULL, then remember this relname
    if (relname == NULL)
      relname = temp->u.QUALATTR.relname;

    // otherwise, see if the relname matches the remembered relname
    else if (strcmp(relname, temp->u.QUALATTR.relname))
      return E_INCOMPATIBLE;

    // add attribute name to the list
    attrnames[i] = list->u.LIST.self->u.QUALATTR.attrname;
  }

  // if the list is too long then error
  if (i == MAXATTRS)
    return E_TOOMANYATTRS;

  // put the relation name in the last position in the array
  attrnames[i] = relname;

  return i;
}


//
// mk_qual_attrs: converts a list of qualified attributes (<relation,
// attribute> pairs) into an array of REL_ATTRS so it can be sent to
// QU_Join.
//
// All of the attributes must come from either relname1 or relname2.
//
// Returns:
// 	the lengh of the list on success ( >= 0 )
// 	error code otherwise
//

static int mk_qual_attrs(NODE *list, REL_ATTR qual_attrs[],
			 char *relname1, char *relname2)
{
  int i;
  NODE *attr;

  // for each element of the list...
  for(i = 0; list != NULL && i < MAXATTRS; ++i, list = list->u.LIST.next) {
    attr = list->u.LIST.self;

    // if relname != relname 1...
    if (strcmp(attr->u.QUALATTR.relname, relname1)) {

      // and relname != relname 2, then error
      if (strcmp(attr->u.QUALATTR.relname, relname2))
	return E_INCOMPATIBLE;
    }

    // add it to the list
    qual_attrs[i].relName = attr->u.QUALATTR.relname;
    qual_attrs[i].attrName = attr->u.QUALATTR.attrname;
  }

  // If the list is too long then error
  if (i == MAXATTRS)
    return E_TOOMANYATTRS;
  
  return i;
}


//
// mk_attr_descrs: converts a list of attribute descriptors (attribute names,
// types, and lengths) to an array of ATTR_DESCR's so it can be sent to
// UT_Create.
//
// Returns:
// 	length of the list on success ( >= 0 )
// 	error code otherwise
//

static int mk_attr_descrs(NODE *list, ATTR_DESCR attr_descrs[])
{
  int i;
  int type, len;
  NODE *attr;
  int errval;

  // for each element of the list...
  for(i = 0; list != NULL && i < MAXATTRS; ++i, list = list->u.LIST.next) {
    attr = list->u.LIST.self;
    
    // interpret the format string
    errval = parse_format_string(attr->u.ATTRTYPE.type, &type, &len);
    if (errval != E_OK)
      return errval;

    // add it to the list
    attr_descrs[i].attrName = attr->u.ATTRTYPE.attrname;
    attr_descrs[i].attrType = type;
    attr_descrs[i].attrLen = len;
  }
  
  // if the list is too long, then error
  if (i == MAXATTRS)
    return E_TOOMANYATTRS;
  
  return i;
}


//
// mk_ins_attrs: converts a list of <attribute, value> pairs to an array
// of ATTR_VAL's so it can be sent to QU_Insert.
//
// Returns:
// 	length of the list on success ( >= 0 )
// 	error code otherwise ( < 0 )
//

static int mk_ins_attrs(NODE *list, ATTR_VAL ins_attrs[])
{
  int i, type, len;
  NODE *attr;
  
  // add the attributes to the list
  for(i = 0; list != NULL && i < MAXATTRS; ++i, list = list->u.LIST.next) {
    attr = list->u.LIST.self;
    
    // make sure string attributes aren't too long
    type = type_of(attr->u.ATTRVAL.value);
    len = length_of(attr->u.ATTRVAL.value);
    if (type == STRING && len > MAXSTRINGLEN)
      return E_STRINGTOOLONG;
    
    ins_attrs[i].attrName = attr->u.ATTRVAL.attrname;
    ins_attrs[i].valType = type;
    ins_attrs[i].valLength = len;
    ins_attrs[i].value = value_of(attr->u.ATTRVAL.value);
  }
  
  // if list is too long then error
  if (i == MAXATTRS)
    return E_TOOMANYATTRS;
  
  return i;
}

/*
  Re write parse_format_string due to change of NODE.ATTRTYPE
*/
static int parse_format_string(int format, int *type, int *len)
{

  if (format == ('i'-128)) {
    *type = INTEGER;
    *len = sizeof(int);
    return E_OK;
  }
  else if (format == ('f'-128)) {
    *type = FLOAT;
    *len = sizeof(float);
    return E_OK;
  }
  else if ((format<=255)&&(format>=1)) {
    *type = STRING;
    *len = format;
    return E_OK;
  }

  return E_INVFORMATSTRING;
}


/*
//
// parse_format_string: deciphers a format string of the form: x
// where x is a type specification (one of `i' INTEGER, `f' FLOAT,
// or `s' STRING, and stores the type in *type.
//
// Returns
// 	E_OK on success
// 	error code otherwise
//

static int parse_format_string(char *format_string, int *type, int *len)
{
  int n;
  char c;
  
  // extract the components of the format string
  n = sscanf(format_string, "%c%d", &c, len);
  
  // if no length given...
  if (n == 1) {
    
    switch(c) {
    case INTCHAR:
      *type = INTEGER;
      *len = sizeof(int);
      break;
    case FLOATCHAR:
      *type = FLOAT;
      *len = sizeof(float);
      break;
    case STRCHAR:
      *type = STRING;
      *len = 1;
      break;
    default:
      return E_INVFORMATSTRING;
    }
  }

  // if both are given, make sure the length is valid
  else if (n == 2) {

    switch(c) {
    case INTCHAR:
      *type = INTEGER;
      if (*len != sizeof(int))
	return E_INVINTSIZE;
      break;
    case FLOATCHAR:
      *type = FLOAT;
      if (*len != sizeof(float))
	return E_INVFLOATSIZE;
      break;
    case STRCHAR:
      *type = STRING;
      break;
    default:
      return E_INVFORMATSTRING;
    }
  }

  // otherwise it's not a valid format string
  else
    return E_INVFORMATSTRING;
  
  return E_OK;
}
*/

//
// type_of: returns the type of a value node
//

static int type_of(NODE *n)
{
  return n->u.VALUE.type;
}


//
// length_of: returns the length of the value in a value node
//

static int length_of(NODE *n)
{
  return n->u.VALUE.len;
}


//
// value_of: returns the value of a value node
// The caller will get a fresh copy of the value, in string form.
// We assume the caller will free() the memory pointed to by the
// return value of this function.
//

static void *value_of(NODE *n)
{
  char *newvalue;
  char value[255];
  
  switch(type_of(n)) {
  case INTEGER:
    sprintf(value, "%d", n->u.VALUE.u.ival);
    break;
  case FLOAT:
    sprintf(value, "%f", n->u.VALUE.u.rval);
    break;
  case STRING:
    sprintf(value, "%s", n->u.VALUE.u.sval);
  }
  if (!(newvalue = new char [strlen(value)+1])) {
    fprintf(stderr, "could not allocate memory\n");
    exit(1);
  }
  strcpy(newvalue, value);
  return (void *)newvalue;
}


//
// print_error: prints an error message corresponding to errval
//

static void print_error(char *errmsg, int errval)
{
  if (errmsg != NULL)
    fprintf(stderr, "%s: ", errmsg);
  switch(errval) {
  case E_OK:
    fprintf(ERRFP, "no error\n");
    break;
  case E_INCOMPATIBLE:
    fprintf(ERRFP, "attributes must be from selected relation(s)\n");
    break;
  case E_TOOMANYATTRS:
    fprintf(ERRFP, "too many attributes\n");
    break;
  case E_NOLENGTH:
    fprintf(ERRFP, "length must be specified for STRING attribute\n");
    break;
  case E_INVINTSIZE:
    fprintf(ERRFP, "invalid size for INTEGER attribute (should be %d)\n",
	    (int) sizeof(int));
    break;
  case E_INVFLOATSIZE:
    fprintf(ERRFP, "invalid size for FLOAT attribute (should be %d)\n",
	    (int) sizeof(float));
    break;
  case E_INVFORMATSTRING:
    fprintf(ERRFP, "invalid format string\n");
    break;
  case E_INVSTRLEN:
    fprintf(ERRFP, "invalid length for string attribute\n");
    break;
  case E_DUPLICATEATTR:
    fprintf(ERRFP, "duplicated attribute name\n");
    break;
  case E_TOOLONG:
    fprintf(stderr, "relation name or attribute name too long\n");
    break;
  case E_STRINGTOOLONG:
    fprintf(stderr, "string attribute too long\n");
    break;
  default:
    fprintf(ERRFP, "unrecognized errval: %d\n", errval);
  }
}


//
// quit procedure (makes sure that we exit, even it UT_Quit doesn't)
//

void quit(void)
{
  UT_Quit();

  // if UT_Quit didn't exit, then print a warning and quit
  fprintf(stderr, "*** ERROR:  UT_quit failed to exit. ***\n");
  
  exit(1);
}


static void echo_query(NODE *n)
{
  switch(n->kind) {
  case N_QUERY:
    printf("select");
    if (n->u.QUERY.relname != NULL)
      printf(" into %s", n->u.QUERY.relname);
    printf(" (");
    print_attrnames(n->u.QUERY.attrlist);
    printf(")");
    print_qual(n->u.QUERY.qual);
    printf(";\n");
    break;
  case N_INSERT:
    printf("insert %s (", n->u.INSERT.relname);
    print_attrvals(n->u.INSERT.attrlist);
    printf(");\n");
    break;
  case N_DELETE:
    printf("delete %s", n->u.DELETE.relname);
    print_qual(n->u.DELETE.qual);
    printf(";\n");
    break;
  case N_CREATE:
    printf("create %s (", n->u.CREATE.relname);
    print_attrdescrs(n->u.CREATE.attrlist);
    printf(")");
    print_primattr(n->u.CREATE.primattr);
    printf(";\n");
    break;
  case N_DESTROY:
    printf("destroy %s;\n", n->u.DESTROY.relname);
    break;
  case N_BUILD:
    printf("buildindex %s(%s);\n", n->u.BUILD.relname, n->u.BUILD.attrname);
#if 0
    printf("buildindex %s(%s) numbuckets = %d;\n", n->u.BUILD.relname,
	   n->u.BUILD.attrname, n->u.BUILD.nbuckets);
#endif
    break;
  case N_REBUILD:
    printf("rebuildindex %s(%s) numbuckets = %d;\n", n->u.BUILD.relname,
	   n->u.BUILD.attrname, n->u.BUILD.nbuckets);
    break;
  case N_DROP:
    printf("dropindex %s", n->u.DROP.relname);
    if (n->u.DROP.attrname != NULL)
      printf("(%s)", n->u.DROP.attrname);
    printf(";\n");
    break;
  case N_LOAD:
    printf("load %s(\"%s\");\n",
	   n->u.LOAD.relname, n->u.LOAD.filename);
    break;
  case N_PRINT:
    printf("print %s;\n", n->u.PRINT.relname);
    break;
  case N_HELP:
    printf("help");
    if (n->u.HELP.relname != NULL)
      printf(" %s", n->u.HELP.relname);
    printf(";\n");
    break;
  default:                              // so that compiler won't complain
    assert(0);
  }
}


static void print_attrnames(NODE *n)
{
  for(; n != NULL; n = n->u.LIST.next) {
    print_qualattr(n->u.LIST.self);
    if (n->u.LIST.next != NULL)
      printf(", ");
  }
}


static void print_attrvals(NODE *n)
{
  NODE *attr;
  
  for(; n != NULL; n = n->u.LIST.next) {
    attr = n->u.LIST.self;
    printf("%s =", attr->u.ATTRVAL.attrname);
    print_val(attr->u.ATTRVAL.value);
    if (n->u.LIST.next != NULL)
      printf(", ");
  }
}

static void print_attrdescrs(NODE *n)
{
  NODE *attr;

  for(; n != NULL; n = n->u.LIST.next) {
    attr = n->u.LIST.self;
    printf("%s = ", attr->u.ATTRTYPE.attrname);
    int format = attr->u.ATTRTYPE.type;
    if (format == ('i'-128)) {
      printf("int");
    }
    else if (format == ('f'-128)) {
      printf("real");
    }
    else if ((format<=255)&&(format>=1)) {
      printf("char(%d)", attr->u.ATTRTYPE.type);
    }
    if (n->u.LIST.next != NULL)
      printf(", ");
  }
}


static void print_primattr(NODE *n)
{
  if (n == NULL)
    return;
  
  printf(" primary %s numbuckets = %d",
	 n->u.PRIMATTR.attrname, n->u.PRIMATTR.nbuckets);
}

static void print_qual(NODE *n)
{
  if (n == NULL)
    return;
  printf(" where ");
  if (n->kind == N_SELECT) {
    print_qualattr(n->u.SELECT.selattr);
    print_op(n->u.SELECT.op);
    print_val(n->u.SELECT.value);
  } else {
    print_qualattr(n->u.JOIN.joinattr1);
    print_op(n->u.JOIN.op);
    printf(" ");
    print_qualattr(n->u.JOIN.joinattr2);
  }
}


static void print_qualattr(NODE *n)
{
  printf("%s.%s", n->u.QUALATTR.relname, n->u.QUALATTR.attrname);
}


static void print_op(int op)
{
  switch(op) {
  case LT:
    printf(" <");
    break;
  case LTE:
    printf(" <=");
    break;
  case EQ:
    printf(" =");
    break;
  case GT:
    printf(" >");
    break;
  case GTE:
    printf(" >=");
    break;
  case NE:
    printf(" <>");
    break;
  }
}


static void print_val(NODE *n)
{
  switch(n->u.VALUE.type) {
  case INTEGER:
    printf(" %d", n->u.VALUE.u.ival);
    break;
  case FLOAT:
    printf(" %f", n->u.VALUE.u.rval);
    break;
  case STRING:
    printf(" \"%s\"", n->u.VALUE.u.sval);
    break;
  }
}
