#ifndef SORT_H
#define SORT_H

#include "heapfile.h"

// define if debug output wanted
//#define DEBUGSORT


// SORTREC is an in-memory sort record that qsort(3) sorts.
// The sort attribute as well as the associated RID are
// stored in the record. The RID is used for fetching the
// full record when it is needed.

typedef struct {
  RID rid;                              // record id of current record
  char* field;                          // pointer to field
  int length;                           // length of field
} SORTREC;


class SortedFile {
 public:
  SortedFile(const string & fileName, 
	     int offset,// sort source file on the given
	     int length, Datatype type, // attribute
	     int maxItems, Status& status);

  Status next(Record & rec);            // fetch next record in sort order
  Status setMark();                     // record a position in sort sequence
  Status gotoMark();                    // go to last recorded spot
  ~SortedFile();                        // destroy temporary structures / files

 private:
  Status sortFile();                    // split source file into sub-runs
  Status generateRun(int numItems);     // generate one sub-run of file
  Status startScans();                  // start a scan on each sorted run

  typedef struct {
    string name;                        // name of run file
    HeapFileScan* inFile;               // ptr to input file
    InsertFileScan* outFile;		// ptr to output file
    int valid;                          // TRUE if recPtr has a record
    Record rec;
    RID rid;                            // RID of current record of run
    RID mark;
  } RUN;

  vector<RUN> runs;                   // holds info about each sub-run

  HeapFile* hfile;                   // source file to sort
  HeapFileScan* hfs;                   // source file to sort
  string fileName;                      // name of source file to sort
  Datatype type;                        // type of sort attribute
  int offset;                           // offset of sort attribute
  int length;                           // length of sort attribute

  SORTREC* buffer;                      // in-memory sort buffer
  int maxItems;                         // max. # of items/tuples in buffer
  int numItems;                         // current # of items in buffer
};

#endif
