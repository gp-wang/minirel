#include <sys/types.h>
#include <functional>
#include <string.h>
#include <iostream>
#include <sstream>
#include <vector>
using namespace std;
#include "sort.h"
#include "stdlib.h"

#define MIN(a,b)   ((a) < (b) ? (a) : (b))


// These comparison functions are visible only within this
// source file. reccmp is the comparison routine (much like
// strcmp or memcmp) that accepts integers, floats, and strings.
// It returns -1 if p1 is less than p2, +1 if p1 is greater
// than p2, or zero otherwise.

static int reccmp(char* p1, char* p2, int p1Len, int p2Len, Datatype type)
{
  float diff = 0.0;

  switch(type) {
  case INTEGER:
    int iattr, ifltr;                   // word-alignment problem possible
    memcpy(&iattr, p1, sizeof(int));
    memcpy(&ifltr, p2, sizeof(int));
    diff = iattr - ifltr;
    break;

  case FLOAT:
    float fattr, ffltr;                 // word-alignment problem possible
    memcpy(&fattr, p1, sizeof(float));
    memcpy(&ffltr, p2, sizeof(float));
    diff = fattr - ffltr;
    break;

  case STRING:
    diff = memcmp(p1, p2, MIN(p1Len, p2Len));
    break;
  }

  if (diff < 0)
    diff = -1;
  else if (diff > 0)
    diff = 1;

  return (int)diff;
}


// These three comparison routines are jacketed versions of
// reccmp. This is because qsort(3) takes only a function pointer
// but no additional parameters. The objects pointed to by p1
// and p2 are of type SORTREC which has a pointer to the field
// to be compared as well as its length (used for strings).

#define SR(p)  ((SORTREC*)p)

static int intcmp(const void* p1, const void* p2)
{
  return reccmp(SR(p1)->field, SR(p2)->field,
		SR(p1)->length, SR(p2)->length,
		INTEGER);
}


static int floatcmp(const void* p1, const void* p2)
{
  return reccmp(SR(p1)->field, SR(p2)->field,
		SR(p1)->length, SR(p2)->length,
		FLOAT);
}


static int stringcmp(const void* p1, const void* p2)
{
  return reccmp(SR(p1)->field, SR(p2)->field,
		SR(p1)->length, SR(p2)->length,
		STRING);
}


// Create a sorted temporary file of the source file (fileName).
// Sorting is based on attribute that is defined by offset, len,
// and type. maxItems is the maximum number of items that a sorted
// sub-run can hold (usually derived from amount of memory available).
// Status code is returned in variable status.

SortedFile::SortedFile(const string & fileName, 
		       int offset, int len, Datatype type,
		       int maxItems, Status& status)
      : fileName(fileName), type(type), offset(offset), 
	length(len), maxItems(maxItems)
{
  // Check incoming parameters.

  status = OK;

  if (offset < 0 || len < 1)
    status = BADSORTPARM;
  else if (type != STRING && type != INTEGER && type != FLOAT)
    status = BADSORTPARM;
  else if (type == INTEGER && len != sizeof(int)
	   || type == FLOAT && len != sizeof(float))
    status = BADSORTPARM;

  if (status != OK)
    return;

  // Must have space for at least 2 items (records) because otherwise
  // items cannot be swapped and sorted!

  if (maxItems < 2 || !(buffer = new SORTREC [maxItems])) {
    status = INSUFMEM;
    return;
  }
    
  status = sortFile();
}


// Sort file into sub-runs. The source file is split into runs
// which have at most maxItems records each. That many records
// are read into memory, sorted using qsort(3), and then written
// to a temporary file.

Status SortedFile::sortFile()
{
  Status status;
  Record rec;

  // Open source file.

  // Start an unfiltered sequential scan.
  hfs = new HeapFileScan(fileName, status);
  if (status != OK) return status;

  status = hfs->startScan(0, 0, STRING, NULL, EQ);
  if (status != OK) return status;

  // As long as the source file has more records, collect up to
  // maxItems records into buffer and then dump records into
  // temporary file.

  do {
    for(numItems = 0; numItems < maxItems; numItems++) {

      // Fetch next record from source file, check if end of file.

      if ((status = hfs->scanNext(buffer[numItems].rid)) == FILEEOF) break;
      else if (status != OK) return status;
      if ((status = hfs->getRecord(rec)) != OK) return status;

      // Create space for holding a copy of the sorting attribute
      // only (rest of record is read when temporary file is
      // written). Copy sorting attribute from source record and
      // store the length of the attribute (reccmp is general-
      // purpose and can be shared by multiple instances of
      // SortedFile!).

      if (!(buffer[numItems].field = new char [length])) return INSUFMEM;
      memcpy(buffer[numItems].field, (char *)rec.data + offset, length);
      buffer[numItems].length = length;
    }
    
    // If at least 1 record in sub-run, sort records and write out
    // to temporary file.

    if (numItems > 0) {
      if ((status = generateRun(numItems)) != OK) return status;
      for(int i = 0; i < numItems; i++) delete [] buffer[i].field;
    }
  } while (numItems > 0);

  // Terminate sequential scan on source file and close file.

  delete hfs;

  // Prepare a sequential scan on each sub-run so that next()
  // can fetch next record from each run.

  if ((status = startScans()) != OK) return status;

  return OK;
}


// Sort the records in buffer[] (actually, the sorting attribute
// plus the associated RID) and then dump records into temporary
// file.

Status SortedFile::generateRun(int items)
{
  Status status;

  // Sort buffer using library function qsort (quick sort). Use
  // the appropriate comparison function for integers, floats,
  // or strings (qsort can't take type as a parameter).

  if (type == INTEGER)
    qsort(buffer, items, sizeof(SORTREC), intcmp);
  else if (type == FLOAT)
    qsort(buffer, items, sizeof(SORTREC), floatcmp);
  else
    qsort(buffer, items, sizeof(SORTREC), stringcmp);

  // If this is the first sub-run, malloc space for a RUN object,
  // otherwise realloc more space. Note that on most systems
  // realloc(NULL) could be used even when runs == NULL, but
  // this doesn't work on all systems.

  RUN newRun;
  runs.push_back(newRun);

  // If failed to create space for an additional run.

   RUN & run = runs.back();

  // Generate file name for temporary file.

  stringstream  outputString;
  outputString << fileName << ".sort." << runs.size() << ends;
  run.name = outputString.str();

#ifdef DEBUGSORT
  cout << "%%  Writing " << items << " tuples to file " << run.name
       << endl;
#endif

  // Make sure temporary file does not exist already. We don't
  // want to corrupt somebody else's sorted files (on another
  // attribute, for example).

  if ((status = db.createFile(run.name)) != OK)
    return status;                      // file must not exist already
  if ((status = db.destroyFile(run.name)) != OK)
    return status;                      // delete if successful

  // Open a heap file. This will also create the temporary file.
  if (!(run.outFile = new InsertFileScan(run.name, status))) return INSUFMEM;
  if (status != OK) return status;

  // Open input file
  hfile = new HeapFile (fileName, status);
  if (status != OK) return status;

  // For each sort record (attribute plus RID) in the buffer, fetch
  // the whole record from the source file and then insert it into
  // the temporary file.

  // cout << "%%  Writing " << items << " tuples to file " << run.name << endl;
  for(int i = 0; i < items; i++) {
    SORTREC* rec = &buffer[i];
    RID rid;
    Record record;

    if ((status = hfile->getRecord(rec->rid, record)) != OK) return status;
    if ((status = run.outFile->insertRecord(record, rid)) != OK) return status;
  }

  delete run.outFile;
  delete hfile;
  return OK;
}


// Prepare a sequential scan on each sub-run so that next()
// can fetch the next record from each run. The valid bit of
// each run is marked false to indicate that the (first)
// record has not been fetched yet. next() must therefore
// fetch it.

Status SortedFile::startScans()
{
  Status status;
  vector<RUN>::iterator run;

  for(run = runs.begin(); run != runs.end(); run++)
    {
      run->inFile = new HeapFileScan(run->name, status);
      if (status != OK) return status;
      status = (run->inFile)->startScan(0, 0, STRING, NULL, EQ);
      if (status != OK) return status;

      run->valid = false;
      run->rid.pageNo = -1;
      run->rid.slotNo = -1;
    }
  return OK;
}


// Retrieve the next smallest record from the set of sorted sub-runs.
// The next record of each sub-run is peeked to find out the
// smallest of all. The pointer in the chosen sub-run is then
// advanced.

Status SortedFile::next(Record & rec)
{
  Status status;
  int i=0;

  // Empty source file has zero sub-runs and causes
  // end of file to be returned.

  if (runs.size() <= 0) return FILEEOF;

  // Find the run which has the smallest next record. If a run
  // has false valid bit, it doesn't have the next record in memory
  // yet.

  RUN* smallest = NULL;
  vector<RUN>::iterator run;

  for(run = runs.begin(); run != runs.end(); run++, i++)
    {
      if (run->valid == false) {          // no record fetched yet for this run?
	status = run->inFile->scanNext(run->rid);
	if (status == FILEEOF)            // reached end of this run file?
	  run->rid.pageNo = -1;           // mark end of file
	else if (status != OK)
	  return status;
	else {                            // if next record exists, fetch it
	  if ((status = run->inFile->getRecord(run->rec)) != OK)
	    return status;
	}
	run->valid = true;                // a record is now in memory
      }

      if (run->rid.pageNo < 0)            // end of run already?
	continue;

      if (!smallest)                      // select first one as smallest
	smallest = &(*run);
      else if (reccmp((char *)smallest->rec.data + offset,
		      (char *)run->rec.data + offset,
		      length, length, type) > 0)
	smallest = &(*run);
    }
  
  if (!smallest)                        // no next record found?
    return FILEEOF;

#ifdef DEBUGSORT
  cout << "%%  Retrieved smallest from " << smallest->name << endl;
#endif

  rec = smallest->rec;               // give record pointers to caller

  smallest->valid = false;              // must fetch new record next time

  return OK;
}


// Remember a position in the sorted output so that the caller
// can later return to this spot. 

Status SortedFile::setMark()
{
#ifdef DEBUGSORT
  cout << "%%  Setting mark in file" << endl;
#endif

  vector<RUN>::iterator run;

  for(run = runs.begin(); run != runs.end(); run++)
  {
      (run->inFile)->markScan();
      run->mark.pageNo = run->rid.pageNo;
      run->mark.slotNo = run->rid.slotNo;
  }
  return OK;
}


// Restore sort position by fetching the last marked record
// This allows the caller to back up in the sorted sequence 
// (used in sort-merge join in case of duplicates).

Status SortedFile::gotoMark()
{
#ifdef DEBUGSORT
  cout << "%%  Going to a mark in file" << endl;
#endif

  Status status;
  vector<RUN>::iterator run;

  for(run = runs.begin(); run != runs.end(); run++)
    {
      status = (run->inFile)->resetScan();
      if (status != OK) return status;
      // restore rid info in the run
      run->rid.pageNo = run->mark.pageNo;
      run->rid.slotNo = run->mark.slotNo;

      // Restore file position only if last marked position is
      // something else than end of file.
      if (run->rid.pageNo >= 0) {
	if ((status = run->inFile->getRecord(run->rec)) != OK) return status;
      }

      // Current record is already in memory so next() must not
      // advance in the temporary file.
      run->valid = true;
    }

  return OK;
}

// Deallocate all space allocated for this sorted file and
// delete temporary files.

SortedFile::~SortedFile()
{
  for(unsigned int i = 0; i < runs.size(); i++) {
    delete runs[i].inFile;
    (void)db.destroyFile(runs[i].name);
  }   

  delete [] buffer;
}
