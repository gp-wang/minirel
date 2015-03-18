#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>
#include "catalog.h"
#include "stdlib.h"

DB db;
BufMgr *bufMgr;
Error error;

RelCatalog *relCat;
AttrCatalog *attrCat;
#define CALL(c)    {Status s;if((s=c)!=OK){error.print(s);exit(1);}}


int main(int argc, char *argv[])
{
  if (argc < 2) {
    cerr << "Usage: " << argv[0] << " dbname" << endl;
    return 1;
  }

  // create database subdirectory and chdir there

  if (mkdir(argv[1], S_IRUSR | S_IWUSR | S_IXUSR
	             | S_IRGRP | S_IWGRP | S_IXGRP) < 0) {
    perror("mkdir");
    exit(1);
  }


  if (chdir(argv[1]) < 0) {
    perror("chdir");
    exit(1);
  }

  // create buffer manager
  
  bufMgr = new BufMgr(100);
  

  Status status;
  // create heapfiles to hold the relcat and attribute catalogs
  status = createHeapFile("relcat");
  if (status != OK) {
    error.print(status);
    exit(1);
  }
  status = createHeapFile("attrcat");
  if (status != OK) {
    error.print(status);
    exit(1);
  }

  // open relation and attribute catalogs
  relCat = new RelCatalog(status);
  if (status == OK)
    attrCat = new AttrCatalog(status);
  if (status != OK) {
    error.print(status);
    exit(1);
  }

  // add tuples describing relcat and attrcat to relation catalog
  // and attribute catalog

  RelDesc rd;
  AttrDesc ad;

  strcpy(rd.relName, RELCATNAME);
  rd.attrCnt = 2;
  CALL(relCat->addInfo(rd));

  strcpy(ad.relName, RELCATNAME);
  strcpy(ad.attrName, "relName");
  ad.attrOffset = 0;
  ad.attrType = (int)STRING;
  ad.attrLen = sizeof rd.relName;
  CALL(attrCat->addInfo(ad));

  strcpy(ad.attrName, "attrCnt");
  ad.attrOffset += sizeof rd.relName;
  ad.attrType = (int)INTEGER;
  ad.attrLen = sizeof rd.attrCnt;
  CALL(attrCat->addInfo(ad));

  strcpy(rd.relName, ATTRCATNAME);
  rd.attrCnt = 5;
  CALL(relCat->addInfo(rd))

  strcpy(ad.relName, ATTRCATNAME);
  strcpy(ad.attrName, "relName");
  ad.attrOffset = 0;
  ad.attrType = (int)STRING;
  ad.attrLen = sizeof ad.relName;
  CALL(attrCat->addInfo(ad));

  strcpy(ad.attrName, "attrName");
  ad.attrOffset += sizeof ad.relName;
  ad.attrType = (int)STRING;
  ad.attrLen = sizeof ad.attrName;
  CALL(attrCat->addInfo(ad));

  strcpy(ad.attrName, "attrOffset");
  ad.attrOffset += sizeof ad.attrName;
  ad.attrType = (int)INTEGER;
  ad.attrLen = sizeof ad.attrOffset;
  CALL(attrCat->addInfo(ad));

  strcpy(ad.attrName, "attrType");
  ad.attrOffset += sizeof ad.attrOffset;
  ad.attrType = (int)INTEGER;
  ad.attrLen = sizeof ad.attrType;
  CALL(attrCat->addInfo(ad));

  strcpy(ad.attrName, "attrLen");
  ad.attrOffset += sizeof ad.attrType;
  ad.attrType = (int)INTEGER;
  ad.attrLen = sizeof ad.attrLen;
  CALL(attrCat->addInfo(ad));

  delete relCat;
  delete attrCat;

  delete bufMgr;

  cout << "Database " << argv[1] << " created" << endl;

  return 0;
}
