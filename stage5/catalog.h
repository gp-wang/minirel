#ifndef CATALOG_H
#define CATALOG_H

#include "heapfile.h"


// define if debug output wanted
//#define DEBUGCAT


#define RELCATNAME   "relcat"           // name of relation catalog
#define ATTRCATNAME  "attrcat"          // name of attribute catalog
#define MAXNAME      32                 // length of relName, attrName
#define MAXSTRINGLEN 255                // max. length of string attribute


// schema of relation catalog:
//   relation name : char(32)           <-- lookup key
//   attribute count : integer(4)


typedef struct {
  char relName[MAXNAME];                // relation name
  int attrCnt;                          // number of attributes
} RelDesc;


typedef struct {
  char relName[MAXNAME];                // relation name
  char attrName[MAXNAME];               // attribute name
  int  attrType;                        // INTEGER, FLOAT, or STRING
  int  attrLen;                         // length of attribute in bytes
  void *attrValue;                      // ptr to binary value
} attrInfo; 


class RelCatalog : public HeapFile {
 public:
  // open relation catalog
  RelCatalog(Status &status);

  // get relation descriptor for a relation
  const Status getInfo(const string & relation, RelDesc& record);

  // add information to catalog
  const Status addInfo(RelDesc & record);

  // remove tuple from catalog
  const Status removeInfo(const string & relation);

  // create a new relation
  const Status createRel(const string & relation, 
		   const int attrCnt, 
		   const attrInfo attrList[]);

  // destroy a relation
  const Status destroyRel(const string & relation);

  // print catalog information
  const Status help(const string & relation);          // relation may be NULL

  // get rid of catalog
  ~RelCatalog();
};


// schema of attribute catalog:
//   relation name : char(32)           <-- lookup keys
//   attribute name : char(32)          <--
//   attribute number : integer(4)
//   attribute type : integer(4)  (type is Datatype actually)
//   attribute size : integer(4)


typedef struct {
  char relName[MAXNAME];                // relation name
  char attrName[MAXNAME];               // attribute name
  int attrOffset;                       // attribute offset
  int attrType;                         // attribute type
  int attrLen;                          // attribute length
} AttrDesc;


class AttrCatalog : public HeapFile {
 friend class RelCatalog;

 public:
  // open attribute catalog
  AttrCatalog(Status &status);

  // get attribute catalog tuple
  const Status getInfo(const string & relation, 
		       const string & attrName, 
		       AttrDesc &record);

  // add information to catalog
  const Status addInfo(AttrDesc & record);

  // remove tuple from catalog
  const Status removeInfo(const string & relation, const string & attrName);

  // get all attributes of a relation
  const Status getRelInfo(const string & relation, 
			  int &attrCnt, 
			  AttrDesc *&attrs);

  // delete all information about a relation
  const Status dropRelation(const string & relation);

  // close attribute catalog
  ~AttrCatalog();
};


extern RelCatalog  *relCat;
extern AttrCatalog *attrCat;
extern Error error;
extern Status createHeapFile(const string filename);
extern Status destroyHeapFile(const string filename);

#endif
