#include "heapfile.h"
#include "error.h"

// routine to create a heapfile
/* This function creates an empty (well, almost empty) heap file.
 @Param fileName: a string to be used as the file's name to be created
 @return: the status of execution result
*/
const Status createHeapFile(const string fileName)
{
  File* 		file;
  Status 		status;
  FileHdrPage*	hdrPage;
  int			hdrPageNo;
  int			newPageNo;
  Page*		newPage;

  // try to open the file. This should return an error
  status = db.openFile(fileName, file);
  if (status != OK)
    {
      // file doesn't exist. First create it and allocate
      // an empty header page and data page.
      Status cr_result = db.createFile(fileName);
      if( cr_result != OK) {return cr_result;}

      Status op_result = db.openFile(fileName, file);
      if( op_result != OK) {return op_result;}

      
      /* gw: construct header page */
      Status hdr_alloc_result = bufMgr->allocPage(file, hdrPageNo, newPage);
      if( hdr_alloc_result != OK) {return hdr_alloc_result;}

      hdrPage = reinterpret_cast<FileHdrPage*>(newPage);

      int len_name = fileName.size();
      for(int i = 0; i != len_name; ++i) {
	hdrPage->fileName[i] = fileName[i];
      }
      hdrPage->fileName[len_name] = '\0';
      hdrPage->firstPage = 0;
      hdrPage->lastPage = 0;
      hdrPage->pageCnt = 0;
      hdrPage->recCnt = 0;


      /* gw: construct first data page */
      Status data_alloc_result = bufMgr->allocPage(file, newPageNo, newPage);
      if( data_alloc_result != OK) {return data_alloc_result;}
      newPage->init(newPageNo);
      /* gw: end of file marker */
      newPage->setNextPage(-1);	

      /* gw: update header page stat */
      hdrPage->firstPage = newPageNo;
      hdrPage->lastPage = newPageNo;
      hdrPage->pageCnt = 1;
      hdrPage->recCnt = 0;

      /* gw: unpin pages */
      bufMgr->unPinPage(file, hdrPageNo, true);
      bufMgr->unPinPage(file, newPageNo, true);

      /* gw: ensure initial content on disk */
      bufMgr->flushFile(file);

      db.closeFile(file);
  
      return OK;
		
    }
  return (FILEEXISTS);
}

// routine to destroy a heapfile
const Status destroyHeapFile(const string fileName)
{
	return (db.destroyFile (fileName));
}


/* This method first opens the appropriate file.
   @Param fileName: input of string type to supply the name of the
   file to be opened.
   @Param returnStatus: output the status of method execution.
   @Return: (No return because of constructor) 
 */
HeapFile::HeapFile(const string & fileName, Status& returnStatus)
{
    Status 	status;
    //    Page*	pagePtr;

    cout << "opening file " << fileName << endl;

    // open the file and read in the header page and the first data page
    if ((status = db.openFile(fileName, filePtr)) == OK)
    {
      File* file = filePtr;

      int pageNo = -1;
      /* gw: get header page's page no */
      status = file->getFirstPage(pageNo);
      if(status != OK) {
	cerr << "getFirstPage failed\n";
	returnStatus = status;
      }

      Page* page_ptr;
      status = bufMgr->readPage(file, pageNo, page_ptr);
      if(status != OK) {
	cerr << "readPage failed\n";
	returnStatus = status;
      }

      headerPage = reinterpret_cast<FileHdrPage*>(page_ptr);
      headerPageNo = pageNo;
      hdrDirtyFlag = false;
      
      int firstPageNo = headerPage->firstPage;
      status = bufMgr->readPage(file, firstPageNo, page_ptr);
      if(status != OK) {
	cerr << "readPage failed\n";
	returnStatus = status;
	return;
      }
      
      /* gw: set curPage */
      curPage = page_ptr;
      curPageNo = firstPageNo;
      curDirtyFlag = false;

      curRec = NULLRID;
      returnStatus = OK;
      return;

    }
    else
    {
    	cerr << "open of heap file failed\n";
		returnStatus = status;
		return;
    }
}

// the destructor closes the file
HeapFile::~HeapFile()
{
    Status status;
    cout << "invoking heapfile destructor on file " << headerPage->fileName << endl;

    // see if there is a pinned data page. If so, unpin it 
    if (curPage != NULL)
    {
    	status = bufMgr->unPinPage(filePtr, curPageNo, curDirtyFlag);
		curPage = NULL;
		curPageNo = 0;
		curDirtyFlag = false;
		if (status != OK) cerr << "error in unpin of date page\n";
    }
	
	 // unpin the header page
    status = bufMgr->unPinPage(filePtr, headerPageNo, hdrDirtyFlag);
    if (status != OK) cerr << "error in unpin of header page\n";
	
	// status = bufMgr->flushFile(filePtr);  // make sure all pages of the file are flushed to disk
	// if (status != OK) cerr << "error in flushFile call\n";
	// before close the file
	status = db.closeFile(filePtr);
    if (status != OK)
    {
		cerr << "error in closefile call\n";
		Error e;
		e.print (status);
    }
}

// Return number of records in heap file

const int HeapFile::getRecCnt() const
{
  return headerPage->recCnt;
}

// retrieve an arbitrary record from a file.
// if record is not on the currently pinned page, the current page
// is unpinned and the required page is read into the buffer pool
// and pinned.  returns a pointer to the record via the rec parameter
/*
 @Param rid: supply the rid of record to be fetched
 @Param rec: output the record
 @Return: status of method execution
*/
const Status HeapFile::getRecord(const RID &  rid, Record & rec)
{
  Status status;

  // cout<< "getRecord. record (" << rid.pageNo << "." << rid.slotNo << ")" << endl;
  if( curPage && curPageNo == rid.pageNo ) {
    /* gw: current page happens to be the desired one */
    curRec = rid;
    status = curPage->getRecord(curRec, rec);
  }
  else {
    /* gw: current page is not the desired one, replace it  */
    status = bufMgr->unPinPage(filePtr, curPageNo, curDirtyFlag);
    if( status != OK) {return status;}
    
    curPageNo = rid.pageNo, curRec = rid, curDirtyFlag = false;

    status = bufMgr->readPage(filePtr, curPageNo, curPage);
    if( status != OK) return status;
      
    status = curPage->getRecord(curRec, rec);
  }
  return status;
   
}

HeapFileScan::HeapFileScan(const string & name,
			   Status & status) : HeapFile(name, status)
{
    filter = NULL;
}

const Status HeapFileScan::startScan(const int offset_,
				     const int length_,
				     const Datatype type_, 
				     const char* filter_,
				     const Operator op_)
{
    if (!filter_) {                        // no filtering requested
        filter = NULL;
        return OK;
    }
    
    if ((offset_ < 0 || length_ < 1) ||
        (type_ != STRING && type_ != INTEGER && type_ != FLOAT) ||
        ((type_ == INTEGER && length_ != sizeof(int))
         || (type_ == FLOAT && length_ != sizeof(float))) ||
        (op_ != LT && op_ != LTE && op_ != EQ && op_ != GTE && op_ != GT && op_ != NE))
    {
        return BADSCANPARM;
    }

    offset = offset_;
    length = length_;
    type = type_;
    filter = filter_;
    op = op_;

    return OK;
}


const Status HeapFileScan::endScan()
{
    Status status;
    // generally must unpin last page of the scan
    if (curPage != NULL)
    {
        status = bufMgr->unPinPage(filePtr, curPageNo, curDirtyFlag);
        curPage = NULL;
        curPageNo = 0;
		curDirtyFlag = false;
        return status;
    }
    return OK;
}

HeapFileScan::~HeapFileScan()
{
    endScan();
}

const Status HeapFileScan::markScan()
{
    // make a snapshot of the state of the scan
    markedPageNo = curPageNo;
    markedRec = curRec;
    return OK;
}

const Status HeapFileScan::resetScan()
{
    Status status;
    if (markedPageNo != curPageNo) 
    {
		if (curPage != NULL)
		{
			status = bufMgr->unPinPage(filePtr, curPageNo, curDirtyFlag);
			if (status != OK) return status;
		}
		// restore curPageNo and curRec values
		curPageNo = markedPageNo;
		curRec = markedRec;
		// then read the page
		status = bufMgr->readPage(filePtr, curPageNo, curPage);
		if (status != OK) return status;
		curDirtyFlag = false; // it will be clean
    }
    else curRec = markedRec;
    return OK;
}

/*Returns (via the outRid parameter) the RID of the next record that
  satisfies the scan predicate.
  @Param outRid: output the RID of next record that satisfies the scan
  predicate.
  @Return: status of method execution
 */
const Status HeapFileScan::scanNext(RID& outRid)
{
  
  Status 	status = OK;
  RID		nextRid;
  RID		tmpRid;
  int 	nextPageNo;
  Record      rec;

  /* gw: some dedicated status for control flow */
  Status initial_status = OK, rec_status = OK, first_status = OK, next_status = OK;

  /* gw: if the curPage is not initialized yet, do it now */
  if(!curPage) {

    curPageNo = headerPage->firstPage, curDirtyFlag = false;
    status = bufMgr->readPage(filePtr, curPageNo, curPage);
    if( status != OK) return status;

    initial_status = curPage->firstRecord(tmpRid);
    if( status != OK) return status;
      
    curRec = tmpRid;
      
  }

  /* gw: loop thru all rec, page is incremented when hitting ENDOFPAGE */
  while (initial_status == OK || rec_status == OK) {
    /* gw: if the status of reading first record is good, then
       proceed to read next record */
    if(first_status == OK) {
      next_status  = curPage->nextRecord(curRec, nextRid);
      if (next_status == OK) curRec = nextRid;
    }

    /* gw: if the next record (after the first one) is bad, we need
       to increment page */
    if(!(next_status == OK))
      {
	/* gw: get next page's num */
	status = curPage->getNextPage(nextPageNo);
	if( status != OK) {return status;}
	/* gw: check whether last page */
	if (nextPageNo == -1) {
	  return FILEEOF;
	}

	/* gw: next page is valid, unpin curPage to make room for
	   reading that page in */
	status = bufMgr->unPinPage(filePtr, curPageNo, curDirtyFlag);
	if( status != OK) {return status;}
    
	curPageNo = nextPageNo, curDirtyFlag = false;

	status = bufMgr->readPage(filePtr, curPageNo, curPage);
	if( status != OK) return status;

	first_status = curPage->firstRecord(curRec);
	/* gw: if the first record is bad, the page is empty. Hence
	   we need to move on to next page */
	if(!(first_status == OK)) {
	  continue;
	}

      }

    /* gw: read in the current record and check for predicate */
    rec_status = curPage->getRecord(curRec, rec);
    if (rec_status != OK) return rec_status;

    if (matchRec(rec)) {
      outRid = curRec;
      return OK;
    }
  }

  /* gw: should not come here */
  return FILEEOF;    
}


// returns pointer to the current record.  page is left pinned
// and the scan logic is required to unpin the page 

const Status HeapFileScan::getRecord(Record & rec)
{
    return curPage->getRecord(curRec, rec);
}

// delete record from file. 
const Status HeapFileScan::deleteRecord()
{
    Status status;

    // delete the "current" record from the page
    status = curPage->deleteRecord(curRec);
    curDirtyFlag = true;

    // reduce count of number of records in the file
    headerPage->recCnt--;
    hdrDirtyFlag = true; 
    return status;
}


// mark current page of scan dirty
const Status HeapFileScan::markDirty()
{
    curDirtyFlag = true;
    return OK;
}

const bool HeapFileScan::matchRec(const Record & rec) const
{
    // no filtering requested
    if (!filter) return true;

    // see if offset + length is beyond end of record
    // maybe this should be an error???
    if ((offset + length -1 ) >= rec.length)
	return false;

    float diff = 0;                       // < 0 if attr < fltr
    switch(type) {

    case INTEGER:
        int iattr, ifltr;                 // word-alignment problem possible
        memcpy(&iattr,
               (char *)rec.data + offset,
               length);
        memcpy(&ifltr,
               filter,
               length);
        diff = iattr - ifltr;
        break;

    case FLOAT:
        float fattr, ffltr;               // word-alignment problem possible
        memcpy(&fattr,
               (char *)rec.data + offset,
               length);
        memcpy(&ffltr,
               filter,
               length);
        diff = fattr - ffltr;
        break;

    case STRING:
        diff = strncmp((char *)rec.data + offset,
                       filter,
                       length);
        break;
    }

    switch(op) {
    case LT:  if (diff < 0.0) return true; break;
    case LTE: if (diff <= 0.0) return true; break;
    case EQ:  if (diff == 0.0) return true; break;
    case GTE: if (diff >= 0.0) return true; break;
    case GT:  if (diff > 0.0) return true; break;
    case NE:  if (diff != 0.0) return true; break;
    }

    return false;
}

InsertFileScan::InsertFileScan(const string & name,
                               Status & status) : HeapFile(name, status)
{
  //Do nothing. Heapfile constructor will read the header page and the first
  // data page of the file into the buffer pool
}

InsertFileScan::~InsertFileScan()
{
    Status status;
    // unpin last page of the scan
    if (curPage != NULL)
    {
        status = bufMgr->unPinPage(filePtr, curPageNo, true);
        curPage = NULL;
        curPageNo = 0;
        if (status != OK) cerr << "error in unpin of data page\n";
    }
}

// Insert a record into the file
/*
  @Param rec: input of the record that is going to be inserted
  @Param outRid: output the RID of the record that is successfully
  inserted
  @Return: status of method execution
 */
const Status InsertFileScan::insertRecord(const Record & rec, RID& outRid)
{
  Page*	newPage;
  int		newPageNo;
  Status	status;//, unpinstatus;
  RID		rid;

  // check for very large records
  if ((unsigned int) rec.length > PAGESIZE-DPFIXED)
    {
      // will never fit on a page, so don't even bother looking
      return INVALIDRECLEN;
    }
  
  /* gw: if curPage is not the last page, replace it */
  if(!curPage || curPageNo != headerPage->lastPage) {
      
    status = bufMgr->unPinPage(filePtr, curPageNo, curDirtyFlag);
    if( status != OK) {return status;}

    curPageNo = headerPage->lastPage;
    status = bufMgr->readPage(filePtr, curPageNo, curPage);
    if( status != OK) return status;
    curDirtyFlag = false;
      
    status  = curPage->firstRecord(curRec);
    if( status != OK) return status;
      
  }

  /* gw: insert while check whether current page is full */
  status = curPage->insertRecord(rec, rid);
  if(status == OK) {
    /* gw: bookkeeping */
    headerPage->recCnt++, curRec = rid, curDirtyFlag = true;
    /* gw: output */
    outRid = rid;
    return OK;
  }
  else if (status == NOSPACE) {
    /* gw: if NOSPACE, need to allocate a new page */
    status = bufMgr->allocPage(filePtr, newPageNo, newPage);
    if( status != OK) {return status;}
      
    newPage->init(newPageNo);
    headerPage->lastPage = newPageNo, headerPage->pageCnt++;
      
    curPage->setNextPage(newPageNo);
    curDirtyFlag = true;
    status = bufMgr->unPinPage(filePtr, curPageNo, curDirtyFlag);
    if( status != OK) {return status;}

    /* gw: read in the newly allocated page for insertion */
    curPageNo = headerPage->lastPage;
    status = bufMgr->readPage(filePtr, curPageNo, curPage);
    if( status != OK) return status;

    status = curPage->insertRecord(rec, rid);
    if( status != OK) return status;
      
    headerPage->recCnt++, curRec = rid, curDirtyFlag = true;
    /* gw: set marker for end of file */
    curPage->setNextPage(-1);
      
    status = bufMgr->unPinPage(filePtr, curPageNo, curDirtyFlag);
    if( status != OK) return status;

    outRid = rid;
    return OK;
  }
  else {
    /* gw: should not have reached here */
    return status;
  }
   
}


