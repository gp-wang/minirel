/*
 * Project: CS564 p3
 * Student Name: Gaopeng Wang
 * Student (Wisc) Id: gwang63
 *         (CS login) Id: gaopeng
 *         (UW) Id: 907 000 0444
 * File purpose: This file contains the definition of BufMgr's member
 * functions.
 */

#include <cassert>
#include <memory.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include <iostream>
#include <stdio.h>
#include "page.h"
#include "buf.h"

#define ASSERT(c)  { if (!(c)) {                                \
      cerr << "At line " << __LINE__ << ":" << endl << "  ";    \
      cerr << "This condition should hold: " #c << endl;        \
	exit(1);						\
    }                                                           \
  }

//----------------------------------------
// Constructor of the class BufMgr
//----------------------------------------

BufMgr::BufMgr(const int bufs)
{
  numBufs = bufs;

  bufTable = new BufDesc[bufs];
  memset(bufTable, 0, bufs * sizeof(BufDesc));
  for (int i = 0; i < bufs; i++) 
    {
      bufTable[i].frameNo = i;
      bufTable[i].valid = false;
    }

  bufPool = new Page[bufs];
  memset(bufPool, 0, bufs * sizeof(Page));

  int htsize = ((((int) (bufs * 1.2))*2)/2)+1;
  hashTable = new BufHashTbl (htsize);  // allocate the buffer hash table

  clockHand = bufs - 1;
}


/* This is the destructor for BufMgr class. Flushes out all dirty
   pages and deallocates the buffer pool and the BufDesc table.
   
 *@param  (None)
 *@return (None)
 */
BufMgr::~BufMgr() {
  /* gw: scan through frames, write these valid and dirty ones to disk */
  for (int i = 0; i < numBufs; i++)
    {
    
      BufDesc* bdptr = &bufTable[i];
      Page* pgptr = &bufPool[i];

      if(!bdptr->valid || !bdptr->dirty) {
	continue;
      }
        
      bdptr->file->writePage(bdptr->pageNo, pgptr);
    }

  delete [] bufTable;
  delete [] bufPool;
}



/* Allocates a free frame using the clock algorithm; if necessary,
   writing a dirty page back to disk. 

 *@param  frame: reference to the frame number that are going to be
 *allocated by this function.
 
 *@return status of this function's execution result. Returns
   BUFFEREXCEEDED if all buffer frames are pinned, UNIXERR if the call
   to the I/O layer returned an error when a dirty page was being
   written to disk and OK otherwise.
   
 */
const Status BufMgr::allocBuf(int & frame) {
  /* gw: total number of pages that are found pinned while going
     through the list */
  int cnt_pinned_pg = 0;
  
  /* gw: using quotient operation, to check whether all frames are
     pinned. If so, the cnt_pinned_pg will reach numBufs. Becaues
     cnt_pinned_pg intially is zero, it will eventually grow to
     numBufs and the loop will terminate. */
  
  while( cnt_pinned_pg / numBufs != 1 ) {
    /* gw: clock algorithm starts */
  
    advanceClock();

    /* gw: BufDesc for that frame */
    BufDesc* bdptr = &bufTable[clockHand];
    /* gw: Actual Page for that frame */    
    Page* pgptr = &bufPool[clockHand];
    
    // gw: test for validness
    if(!bdptr->valid) {
      frame = bdptr->frameNo;
      bdptr->Clear();
      return OK;
    }

    /* gw: test for refbit */
    if(bdptr->refbit) {
      bdptr->refbit = false;
      continue;
    }

    /* gw: test for pinned page */
    if(bdptr->pinCnt > 0) {
      ++cnt_pinned_pg;
      continue;
    }

    /* gw: test dirty */
    if(bdptr->dirty) {
      Status wt_result = bdptr->file->writePage(bdptr->pageNo, pgptr);
      if( wt_result != OK) {
	return UNIXERR;
      }
      bdptr->dirty = false;
    }

    /* gw: when reaching here, the control flow has cleared special
       cases and is ready for future set and uses. */
    Status remove_result = hashTable->remove(bdptr->file, bdptr->pageNo);
    if(remove_result != OK) {
      return remove_result;
    }

    bdptr->Clear();
    frame = bdptr->frameNo;
    return OK;

  }
  /* gw: if reached here, the while loop is breached and it indicates
     all pages are pinned */
  return BUFFEREXCEEDED;

}



/* Locates the pointer to a page in memory that contains the page
 *specified by the file parameter and PageNo parameter. Pass the
 *result out via the parameter page.
 
 *@param  file: a pointer to the file of the page that are being
 *looked up.
 *@param  PageNo: the page number of the page that are being looked up

 *@param  page: a reference of the pointer to Page to store the lookup
 *result of readpage();
 
 *@return status of this function's execution result. Returns OK if no
 *errors occurred, UNIXERR if a Unix error occurred, BUFFEREXCEEDED if
 *all buffer frames are pinned, HASHTBLERROR if a hash table error
 *occurred.
 
 */
const Status BufMgr::readPage(File* file, const int PageNo, Page*& page) {
  int frame_result = -1;
  Status lk_result = hashTable->lookup(file, PageNo, frame_result);
  if(lk_result == HASHTBLERROR ) {
    return HASHTBLERROR;
  }
  else if (lk_result == HASHNOTFOUND){
    
    Status alloc_result = allocBuf(frame_result);
    if (alloc_result != OK ) {
      return alloc_result;
    }
    
    Page* fmptr = &bufPool[frame_result];
    

    Status insert_result = hashTable->insert(file, PageNo, frame_result) ;
    if ( insert_result != OK) {
      return insert_result;
    }

    BufDesc* bdptr = &bufTable[frame_result];
    bdptr->Set(file, PageNo);
    bdptr->frameNo = frame_result;

    Status read_result = file->readPage(PageNo, fmptr);
    if ( read_result != OK) {
      disposePage(file, PageNo);
      return read_result;
    }

    page = fmptr;
  }
  else {
    Page* fmptr = &bufPool[frame_result];
    BufDesc* bdptr = &bufTable[frame_result];

    bdptr->refbit = true;
    bdptr->pinCnt++;

    page = fmptr;
  }
  return OK;
}

/* Unpin the page specified by the combination of (file, PageNo).
 *@param file: a pointer to the file that contains the page to be unpinned

 *@param PageNo: the page number that is going to be unpinned.

 *@param dirty: a boolean value that specifies whether the page being
 *unpinned is dirty or not.

 @return: It returns the status of function execution result. Returns
 OK if no errors occurred, HASHNOTFOUND if the page is not in the
 buffer pool hash table, PAGENOTPINNED if the pin count is already 0.
 

 */
const Status BufMgr::unPinPage(File* file, const int PageNo, const bool dirty) {
  int frame_result = -1;
  Status lk_result = hashTable->lookup(file, PageNo, frame_result);
  if(lk_result != OK) {
    return lk_result;
  }

  BufDesc* bdptr = &bufTable[frame_result];
  int& pin_count = bdptr->pinCnt;

  if(pin_count == 0) {
    return PAGENOTPINNED;
  }
  --pin_count;

  if(dirty) {
    bdptr->dirty = true;
  }
    
  return OK;
}


/* Allocate a page for the combination specified by (file, PageNo)

   *@param file: a pointer to the file that contains the page to be allocated

   *@param PageNo: the page number that is going to be allocated.

   
   *@param  page: a reference of the pointer to Page to store the lookup
   *result of allocPage();

   *@return: It returns the status of function execution
   *result. Returns OK if no errors occurred, UNIXERR if a Unix error
   *occurred, BUFFEREXCEEDED if all buffer frames are pinned and
   *HASHTBLERROR if a hash table error occurred.
   
*/
const Status BufMgr::allocPage(File* file, int& pageNo, Page*& page) {
  int page_result = -1;
  Status falloc_result = file->allocatePage(page_result);
  if(falloc_result != OK){
    return falloc_result;
  }

  
  int frame_result = -1;
  Status alloc_result = allocBuf(frame_result);
  if(alloc_result != OK) {
    return alloc_result;
  }

  Status insert_result = hashTable->insert(file, page_result, frame_result);
  if(insert_result != OK) {
    return insert_result;
  }

  BufDesc* bdptr = &bufTable[frame_result];
  bdptr->Set(file, page_result);
  
  pageNo = page_result;

  Page* pgptr = &bufPool[frame_result];
  page = pgptr;
  
  return OK;
}



/*
  This function disposes the page specified by (file, PageNo)
   *@param file: a pointer to the file that contains the page to be disposed

   *@param PageNo: the page number that is going to be disposed.

   *@return: It returns the status of function execution
   *result. 
   
*/
const Status BufMgr::disposePage(File* file, const int pageNo) {
  int frame_result = -1;
  Status lk_result = hashTable->lookup(file, pageNo, frame_result);
  if(lk_result == OK) {
    BufDesc* bdptr = &bufTable[frame_result];
    bdptr->Clear();

    Status hs_remove_result = hashTable->remove(file, pageNo);
    if(hs_remove_result != OK){
      return hs_remove_result;
    }

    Status fl_dispose_result = file->disposePage(pageNo);
    if(fl_dispose_result != OK) {
      return fl_dispose_result;
    }
    
  }

  return lk_result;

}

/* This function flushes all the pages in current bufferpool for the
   file specified by the parameter of file.

   *@param file: a pointer to the file that are going to be flushed.

   
   *@return: It returns the status of function execution
   *result. Returns OK if no errors occurred and PAGEPINNED if some page of the file is pinned.

 */
const Status BufMgr::flushFile(const File* file) {
  
  for(int i = 0; i < numBufs; ++i) {
    /* gw: need to chk for matching file's page */

    BufDesc* bdptr = &bufTable[i];
    int pageNo = bdptr->pageNo;
    Page* pgptr = &bufPool[i];

    if(!(bdptr->valid && bdptr->file == file)) {
      continue;
    }

    /* gw: pin page chk */
    if(bdptr->pinCnt > 0) {
      return PAGEPINNED;
    }
    
    if(bdptr->dirty) {
      /* gw: TODO, notable */
      Status wt_result = bdptr->file->writePage(pageNo, pgptr); 
      if(wt_result != OK) {
	return wt_result;
      }
      bdptr->dirty = false;
    }

    Status hs_remove_result = hashTable->remove(file, pageNo);
    if(hs_remove_result != OK){
      return hs_remove_result;
    }

    bdptr->Clear();

  }

  return OK;
}





void BufMgr::printSelf(void) 
{
  BufDesc* tmpbuf;
  
  cout << endl << "Print buffer...\n";
  for (int i=0; i<numBufs; i++) {
    tmpbuf = &(bufTable[i]);
    cout << i << "\t" << (char*)(&bufPool[i]) 
         << "\tpinCnt: " << tmpbuf->pinCnt;
    
    if (tmpbuf->valid == true)
      cout << "\tvalid\n";
    cout << endl;
  };
}


