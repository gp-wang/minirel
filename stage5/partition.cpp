#include <sys/types.h>
#include <functional>
#include <string.h>
#include <iostream>
#include <sstream>
#include <vector>
using namespace std;
#include "partition.h"


// The Partition class splits a heap file into P partitions, using
// a hash function provided by the caller. The hash function must
// return an integer in the range 0 to P-1.
//
// Variable rel is a heap file that has already been opened by the
// caller. fileName is the (base) name of the heap file, and will be
// used as the base part of the partition file names which are of the
// form fileName.p where p is in the range 0 to P-1.
//
// Returns OK if heap file was split successfully, otherwise an error
// code is returned. If OK is returned, variable partName will return
// the names of the partition files. The caller can open the partition
// files as HeapFiles. The partition files are destroyed by the destructor
// of the Partition class.

Partition::Partition(HeapFileScan *rel, 
		     const string &fileName, 
		     const int P,
		     const int (*hashfcn)(const Record & record,
					  const int P),
		     string* &partName, 
		     Status &status) :
  P(P), partName(NULL)
{
  InsertFileScan **part;
  int p;

#ifdef DEBUGPART
  cerr << "%%  Partitioning " << fileName << "..." << endl;
#endif

  // create list of partition heap files and file names

  if (!(part = new InsertFileScan * [P]) || !(partName = new string[P])) {
    status = INSUFMEM;
    return;
  }

  // construct names of partition files (fileName.p where p = 0 to P-1)
  // and create heap files on disk

  for(p = 0; p < P; p++) {

    stringstream  s;
    s << "/tmp/" << fileName << '.' << p << ends;
    partName[p] = s.str();

    if (!(part[p] = new InsertFileScan(partName[p], status))) {
      status = INSUFMEM;
      return;
    }
    if (status != OK)
      return;
  }

  this->partName = partName;

  // perform a sequential scan on the file to be partitioned, and
  // for each record read, get its hash value (using hash function
  // provided by the caller) and then insert the record into the
  // corresponding partition file

  if ((status = rel->startScan(0, sizeof(int), INTEGER, NULL,
			       EQ)) != OK)
    return;

  while(1) {
    Record rec;
    RID rid;

    status = rel->scanNext(rid);
    if (status != OK)
      break;
    if ((status = rel->getRecord(rec)) != OK)
      return;
    p = hashfcn(rec, P);
    if ((status = part[p]->insertRecord(rec, rid)) != OK)
      return;
  }
  if (status != OK && status != FILEEOF)
    return;

  // close partition files and deallocate memory

  for(p = 0; p < P; p++)
    delete part[p];
  delete part;

  if ((status = rel->endScan()) != OK)
    return;

  status = OK;
  return;
}


// The destructor will destroy the heap files where partitions were stored.

Partition::~Partition()
{
  if (!partName)
    return;

  for(int p = 0; p < P; p++) {
    if (db.destroyFile(partName[p]) != OK)
      cerr << "error destroying " << partName[p] << endl;
  }

  delete partName;
}
