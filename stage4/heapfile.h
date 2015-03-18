#ifndef HEAPFILE_H
#define HEAPFILE_H

#include <sys/types.h>
#include <functional>
#include <iostream>
#include <vector>
#include <string.h>
using namespace std;

#include "page.h"
#include "buf.h"

extern DB db;

// define if debug output wanted
//#define DEBUGREL

// Some constant definitions
const unsigned MAXNAMESIZE = 50;

enum Datatype { STRING, INTEGER, FLOAT };    // attribute data types
enum Operator { LT, LTE, EQ, GTE, GT, NE };  // scan operators

struct FileHdrPage
{
  char		fileName[MAXNAMESIZE];   // name of file
  int		firstPage;	// pageNo of first data page in file
  int		lastPage;	// pageNo of last data page in file
  int		pageCnt;	// number of pages
  int		recCnt;		// record count
};


// class definition of heapFile
class HeapFile {
protected:
   File* 	filePtr;        // underlying DB File object
   FileHdrPage*  headerPage;	// pinned file header page in buffer pool
   int		headerPageNo;	// page number of header page
   bool		hdrDirtyFlag;   // true if header page has been updated

   Page* 	curPage;	// data page currently pinned in buffer pool
   int   	curPageNo;	// page number of pinned page
   bool  	curDirtyFlag;   // true if page has been updated
   RID   	curRec;         // rid of last record returned

public:

  // initialize
  HeapFile(const string & name, Status& returnStatus);

  // destructor
  ~HeapFile();

  // return number of records in file
  const int getRecCnt() const;

  // given a RID, read record from file, returning pointer and length
  const Status getRecord(const RID &rid, Record & rec);
};


class HeapFileScan : public HeapFile
{
public:

    HeapFileScan(const string & name, Status & status);

    // end filtered scan
    ~HeapFileScan();

    const Status startScan(const int offset, 
                           const int length,  
                           const Datatype type, 
                           const char* filter, 
                           const Operator op);

    const Status endScan(); // terminate the scan
    const Status markScan(); // save current position of scan
    const Status resetScan(); // reset scan to last marked location

    // return RID of next record that satisfies the scan 
    const Status scanNext(RID& outRid);

    // read current record, returning pointer and length
    const Status getRecord(Record & rec);

    // delete current record 
    const Status deleteRecord();

    // marks current page of scan dirty
    const Status markDirty();

private:
    int   offset;            // byte offset of filter attribute
    int   length;            // length of filter attribute
    Datatype type;           // datatype of filter attribute
    const char* filter;      // comparison value of filter
    Operator op;             // comparison operator of filter

     // The following variables are used to preserve the state
    // of the scan when the method markScan() is invoked.
    // A subsequent invocation of resetScan() will cause the
    // scan to be rolled back to the following
    int   markedPageNo;	// page number of pinned page
    RID   markedRec;         // rid of last record returned

    const bool matchRec(const Record & rec) const;
};


class InsertFileScan : public HeapFile
{
public:

    InsertFileScan(const string & name, Status & status);

    // end filtered scan
    ~InsertFileScan();

    // insert record into file, returning its RID
    const Status insertRecord(const Record & rec, RID& outRid); 
};

#endif
