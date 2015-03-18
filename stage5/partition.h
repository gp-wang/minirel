#ifndef PARTITION_H
#define PARTITION_H

#include "heapfile.h"


// define if debug output wanted
//#define DEBUGPART


class Partition {
 public:
  Partition(HeapFileScan *rel,              // name of heap file to partition
	    const string & fileName,             // (base) name of heap file
	    const int P,                      // number of partitions
	    const int (*hashfcn)(const Record & rec,
				 const int P),  
	                               // hash function to use in partitioning
	    string* &partName,           // names of partitioned heap files
	    Status &status);            // create partitions of file
  ~Partition();                         // destroy partitions

 private:

  int P;                                // number of partitions
  string *partName;                      // partition names
};

#endif
