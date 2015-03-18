#include "catalog.h"


RelCatalog::RelCatalog(Status &status) :
	 HeapFile(RELCATNAME, status)
{
}


const Status RelCatalog::getInfo(const string & relation, RelDesc &record)
{
  if (relation.empty())
    return BADCATPARM;

  Status status;
  Record rec;
  RID rid;

  HeapFileScan*  hfs;
  hfs = new HeapFileScan(RELCATNAME, status);
  if (status != OK) return status;

  if ((status = hfs->startScan(0, relation.length() + 1, STRING,
			  relation.c_str(), EQ)) != OK) 
  {
	delete hfs;
	return status;
  }

  status = hfs->scanNext(rid);
  if (status == FILEEOF) status = RELNOTFOUND;
  else if (status == OK) 
  {
    if ((status = hfs->getRecord(rec)) != OK) return status;
    assert(sizeof(RelDesc) == rec.length);
    memcpy(&record, rec.data, rec.length);
  }

  Status nextStatus = hfs->endScan();
  if (status == OK) status = nextStatus;

  delete hfs;
  return status;
}


const Status RelCatalog::addInfo(RelDesc & record)
{
  RID rid;
  InsertFileScan*  ifs;
  Status status;

  ifs = new InsertFileScan(RELCATNAME, status);
  if (status != OK) return status;

  int len = strlen(record.relName);
  memset(&record.relName[len], 0, sizeof record.relName - len);
  Record rec;
  rec.data = &record;
  rec.length = sizeof(RelDesc);

  status = ifs->insertRecord(rec, rid);
  delete ifs;
  return status;
}

const Status RelCatalog::removeInfo(const string & relation)
{
  Status status;
  RID rid;
  HeapFileScan*  hfs;

  if (relation.empty()) return BADCATPARM;

  hfs = new HeapFileScan(RELCATNAME, status);
  if (status != OK) return status;

  if ((status = hfs->startScan(0, relation.length() + 1, STRING,
			  relation.c_str(), EQ)) != OK)
  {
	delete hfs;
        return status;
  }
  status = hfs->scanNext(rid);
  if (status == FILEEOF) status = RELNOTFOUND;
  if (status == OK) status = hfs->deleteRecord();

  delete hfs;
  hfs->endScan();
  if (status == NORECORDS) return OK;
  else return status;
}


RelCatalog::~RelCatalog()
{
}


AttrCatalog::AttrCatalog(Status &status) :
	 HeapFile(ATTRCATNAME, status)
{
}


const Status AttrCatalog::getInfo(const string & relation, 
				  const string & attrName,
				  AttrDesc &record)
{

  Status status;
  RID rid;
  Record rec;
  HeapFileScan*  hfs;

  if (relation.empty() || attrName.empty()) return BADCATPARM;
  hfs = new HeapFileScan(ATTRCATNAME, status);
  if (status != OK) return status;

  if ((status = hfs->startScan(0, relation.length() + 1, STRING,
			  relation.c_str(), EQ)) != OK)
  {
	delete hfs;
        return status;
  }

  while((status = hfs->scanNext(rid)) == OK) 
  {
    if ((status = hfs->getRecord(rec)) != OK) return status;
    assert(sizeof(AttrDesc) == rec.length);
    memcpy(&record, rec.data, rec.length);
    if (string(record.attrName) == attrName)
      break;
  }
  if (status == FILEEOF)
    status = ATTRNOTFOUND;

  Status nextStatus = hfs->endScan();
  if (status == OK) status = nextStatus;
  delete hfs;
  return status;
}


const Status AttrCatalog::addInfo(AttrDesc & record)
{
  RID rid;
  InsertFileScan*  ifs;
  Status status;

  ifs = new InsertFileScan(ATTRCATNAME, status);
  if (status != OK) return status;

  int len = strlen(record.relName);
  memset(&record.relName[len], 0, sizeof record.relName - len);
  len = strlen(record.attrName);
  memset(&record.attrName[len], 0, sizeof record.attrName - len);

  Record rec;
  rec.data = &record;
  rec.length = sizeof(AttrDesc);
  //cout << "insert record into attCat of size " << rec.length << endl;
  status = ifs->insertRecord(rec, rid);
  if (status != OK) cout << "got error return from insertrecord" << endl;
  delete ifs;
  return status;
}


const Status AttrCatalog::removeInfo(const string & relation, 
			       const string & attrName)
{
  Status status;
  Record rec;
  RID rid;
  AttrDesc record;
  HeapFileScan*  hfs;

  if (relation.empty() || attrName.empty()) return BADCATPARM;

  hfs = new HeapFileScan(ATTRCATNAME, status);
  if (status != OK) return status;

  if ((status = hfs->startScan(0, relation.length() + 1, STRING,
			  relation.c_str(), EQ)) != OK)
  {
	delete hfs;
        return status;
  }

  while((status = hfs->scanNext(rid)) == OK) 
  {
    if ((status = hfs->getRecord(rec)) != OK) return status;

    assert(sizeof(AttrDesc) == rec.length);
    memcpy(&record, rec.data, rec.length);
#ifdef DEBUGCAT
    cerr << "%%  Read attrcat entry " << record.relName
         << "." << record.attrName << endl;
#endif
    if (string(record.attrName) ==  attrName) break;
  }
  if (status == FILEEOF) status = RELNOTFOUND;
  if (status == OK) {
#ifdef DEBUGCAT
    cout << "%%  Deleting attrcat entry " << record.relName
         << "." << record.attrName << endl;
#endif
    status = hfs->deleteRecord();
  }
  hfs->endScan();
  delete hfs;
  if (status == NORECORDS) return OK;
  else return status;
}


const Status AttrCatalog::getRelInfo(const string & relation, 
				     int &attrCnt,
				     AttrDesc *&attrs)
{
  Status status;
  RID rid;
  Record rec;
  HeapFileScan*  hfs;

  if (relation.empty()) return BADCATPARM;

  hfs = new HeapFileScan(ATTRCATNAME, status);
  if (status != OK) return status;

  if ((status = hfs->startScan(0, relation.length() + 1, STRING,
			  relation.c_str(), EQ)) != OK)
  {
	delete hfs;
        return status;
  }

  attrCnt = 0;
  while((status = hfs->scanNext(rid)) == OK) {
    if ((status = hfs->getRecord(rec)) != OK) return status;

    assert(sizeof(AttrDesc) == rec.length);
    ++attrCnt;
    if (attrCnt == 1) {
         if (!(attrs = (AttrDesc*)malloc(sizeof(AttrDesc))))
	return INSUFMEM;
    } else {
      if (!(attrs = (AttrDesc*)realloc(attrs, attrCnt * sizeof(AttrDesc))))
	return INSUFMEM;
    }
    memcpy(&attrs[attrCnt - 1], rec.data, rec.length);
  }

  if (status == FILEEOF) {
    if (attrCnt == 0) status = RELNOTFOUND;
    else status = OK;
  }

  Status nextStatus = hfs->endScan();
  if (status == OK) status = nextStatus;

  delete hfs;
  return status;
}


AttrCatalog::~AttrCatalog()
{
}
