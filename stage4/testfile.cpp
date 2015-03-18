#include <stdio.h>
#include "heapfile.h"
#include <string.h>
#include "stdlib.h"

extern Status createHeapFile(string FileName);
extern Status destroyHeapFile(string FileName);

// globals
DB db;
BufMgr* bufMgr;

int main(int argc, char **argv)
{
    cout << "Testing the relation interface" << endl << endl;

    // DB db;
    Error error;
    HeapFile* file1;
    HeapFileScan *scan1, *scan2;

    InsertFileScan *iScan;
    Status status;
    RID newRid;
	int deleted;
  
    typedef struct {
        int i;
        float f;
        char s[64];
    } RECORD;

    RECORD 	rec1;
    RECORD rec2;              // retrieved recs are unaligned!!
    int           rec1Len;
    Record        dbrec2;
    RID		  rec2Rid;

    bufMgr = new BufMgr(101);

    int i,j;
    int num = 10120;
    Record dbrec1;
    RID*  ridArray;


    ridArray = new RID[num];

    // destroy any old copies of "dummy.02"
    status = destroyHeapFile("dummy.02");
    // ignore the error return

    status = createHeapFile("dummy.02");
    if (status != OK) 
    {
		cerr << "got err0r status return from  createHeapFile" << endl;
    	error.print(status);
    }

    // initialize all of rec1.s to keep purify happy
    memset(rec1.s, ' ', sizeof(rec1.s));

    cout << endl;
    cout << "insert " << num << " records into dummy.02" << endl;
    cout << endl;

    cout << "Start an insert scan which will have the side effect of opening dummy.02 " << endl;
    iScan = new InsertFileScan("dummy.02", status);
    for(i = 0; i < num; i++) {
        sprintf(rec1.s, "This is record %05d", i);
        rec1.i = i;
        rec1.f = i;

        dbrec1.data = &rec1;
        dbrec1.length = sizeof(RECORD);
        status = iScan->insertRecord(dbrec1, newRid);

		// stash away rid and key of the record
		ridArray[i] = newRid;
		//printf("next rid (%d.%d)\n",ridArray[i].pageNo, ridArray[i].slotNo);

        if (status != OK) 
        {
            cout << "got err0r status return from insertrecord" << endl;
            cout << "inserted " << i << "records into file dummy1 before error " 
                 << endl;
            error.print(status);
            exit(1);
        }
    }

    delete iScan; // delete the scan, closing the file at the same time
    file1 = NULL;

    cout << "inserted " << num << " records into file dummy.02 successfully" << endl;
    cout << endl;

    cout << endl;
    cout << "pull 11th record from file dummy.02 using file->getRecord() " << endl;
    file1 = new HeapFile("dummy.02", status); // open the file
    if (status != OK) error.print(status);
    else 
    {
		// get every record
		for (i=0;i<num;i=i+11)
		{
			// reconstruct record for comparison purposes
    	    sprintf(rec1.s, "This is record %05d", i);
    	    rec1.i = i;
    	    rec1.f = i;

			// now read the record
			//printf("getting record (%d.%d)\n",ridArray[i].pageNo, ridArray[i].slotNo);
			status = file1->getRecord(ridArray[i], dbrec2);
    	    if (status != OK) error.print(status);

			// compare with what we should get back
			if (memcmp(&rec1, dbrec2.data, sizeof(RECORD)) != 0)
			cout << "err0r reading record " << i << " back" << endl;
		}
		cout << "getRecord() tests passed successfully" << endl;
    }
    delete file1;

    // scan the file sequentially checking that each record was stored properly
    cout << "scan file dummy.02 " << endl;
    scan1 = new HeapFileScan("dummy.02", status);
    if (status != OK) error.print(status);
    else 
    {
		scan1->startScan(0, 0, STRING, NULL, EQ);
		i = 0; 
		while ((status = scan1->scanNext(rec2Rid)) != FILEEOF)
		{
			// reconstruct record i
			sprintf(rec1.s, "This is record %05d", i);
    	    rec1.i = i;
    	    rec1.f = i;
    	    status = scan1->getRecord(dbrec2);
    	    if (status != OK) break;
			if (memcmp(&rec1, dbrec2.data, sizeof(RECORD)) != 0)
			cout << "err0r reading record " << i << " back" << endl;
    	    i++;
		}
		if (status != FILEEOF) error.print(status);
		cout << "scan file1 saw " << i << " records " << endl;
		if (i != num)
            cout << "Err0r.   scan should have returned " << num << " records!"
                 << endl;
    }
    // delete scan object
    scan1->endScan();
    delete scan1;
    scan1 = NULL;


	// scan the file sequentially checking that each record was stored properly
    cout << endl << "scan file dummy.02 " << endl;
    scan1 = new HeapFileScan("dummy.02", status);
    if (status != OK) error.print(status);
    else 
    {
		scan1->startScan(0, 0, STRING, NULL, EQ);
		i = 0; 
		while ((status = scan1->scanNext(rec2Rid)) != FILEEOF)
		{
			// reconstruct record i
			sprintf(rec1.s, "This is record %05d", i);
    	    rec1.i = i;
    	    rec1.f = i;
    	    status = scan1->getRecord(dbrec2);
    	    if (status != OK) break;
			if (memcmp(&rec1, dbrec2.data, sizeof(RECORD)) != 0)
			cout << "err0r reading record " << i << " back" << endl;
    	    i++;
		}
		if (status != FILEEOF) error.print(status);
		cout << "scan file1 saw " << i << " records " << endl;
		if (i != num)
            cout << "Err0r.   scan should have returned " << num << " records!"
                 << endl;
    }
    // delete scan object
    scan1->endScan();
    delete scan1;
    scan1 = NULL;
	

    // pull every 7th record from the file directly w/o opening a scan
    // by using the file->getRecord() method
    cout << endl;
    cout << "pull every 7th record from file dummy.02 using file->getRecord() " << endl;
    file1 = new HeapFile("dummy.02", status); // open the file
    if (status != OK) error.print(status);
    else 
    {
		// get every 7th record
		for (i=0;i<num;i=i+7)
		{
			// reconstruct record for comparison purposes
    	    sprintf(rec1.s, "This is record %05d", i);
    	    rec1.i = i;
    	    rec1.f = i;

			// now read the record
			//printf("getting record (%d.%d)\n",ridArray[i].pageNo, ridArray[i].slotNo);
			status = file1->getRecord(ridArray[i], dbrec2);
    	    if (status != OK) error.print(status);

			// compare with what we should get back
			if (memcmp(&rec1, dbrec2.data, sizeof(RECORD)) != 0)
			cout << "err0r reading record " << i << " back" << endl;
		}
		cout << "getRecord() tests passed successfully" << endl;
    }
    delete file1; // close the file
    delete [] ridArray;


	// next scan the file deleting all the odd records
    cout << endl;
    cout << "scan file deleting all records whose i field is odd" << endl;

    scan1 = new HeapFileScan("dummy.02", status);
    if (status != OK) error.print(status);
    else 
    {
        scan1->startScan(0, 0, STRING, NULL, EQ);
		i = 0;
		deleted = 0;
		while ((status = scan1->scanNext(rec2Rid)) != FILEEOF)
		{
			// cout << "processing record " << i << i << endl;
			if (status != OK) error.print(status);
			if ((i % 2) != 0)
			{
				// printf("deleting record %d with rid(%d.%d)\n",i,rec2Rid. pageNo, rec2Rid.slotNo);
				status = scan1->deleteRecord(); 
				deleted++;
				if ((status != OK)  && ( status != NORECORDS))
				{
                    cout << "err0r status return from deleteRecord" << endl;
                    error.print(status);
				}
			}
			i++;
		}
		if (status != FILEEOF) error.print(status);
		cout << "deleted " << deleted << " records" << endl;
		if (deleted != num / 2)
            cout << "Err0r.   should have deleted " << num / 2 << " records!" << endl;
		scan1->endScan();
    }
    delete scan1;
    scan1 = NULL;


    cout << endl;
    cout << "scan file, counting number of remaining records" << endl;

    scan1 = new HeapFileScan("dummy.02", status);
    if (status != OK) error.print(status);
    else 
    {
        i = 0;
        scan1->startScan(0, 0, STRING, NULL, EQ);
        while ((status = scan1->scanNext(rec2Rid)) != FILEEOF)
        {
            if ( i > 0 ) // don't delete the first one
            {
                status = scan1->deleteRecord();
                if ((status != OK) && ( status != NORECORDS))
                {
                    cout << "err0r status return from deleteRecord" << endl;
                    error.print(status);
                }
            }
            i++;
        }
        // subtract first record
        i--;

        if (status != FILEEOF) error.print(status);
        scan1->endScan(); 
        delete scan1;
        scan1 = NULL;

        scan1 = new HeapFileScan("dummy.02", status);
        scan1->startScan(0, 0, STRING, NULL, EQ);

        // delete the rest (should only be one)
        while ((status = scan1->scanNext(rec2Rid)) != FILEEOF)
        {
            status = scan1->deleteRecord();
            if ((status != OK) && ( status != NORECORDS))
            {
                cout << "err0r status return from deleteRecord" << endl;
                error.print(status);
            }
            i++;
        }
    
        scan1->endScan();
        delete scan1;

        cout << "scan saw " << i << " records " << endl;
        if (i != (num+1) / 2)
            cout << "Err0r.   scan should have returned " << (num+1) / 2
                 << " records!" << endl;
    }
	
		


    status = destroyHeapFile("dummy.02");
    if (status != OK) 
    {
	cerr << "got err0r status return from  destroyHeapFile" << endl;
    	error.print(status);
    }
    status = createHeapFile("dummy.03");
    if (status != OK) 
    {
	cerr << "got err0r status return from  createHeapFile" << endl;
    	error.print(status);
    }
    iScan = new InsertFileScan("dummy.03", status);
    if (status != OK) 
    {
	cout << "got err0r status return from new insertFileScan call" << endl;
    	error.print(status);
    }

    cout << endl;
    cout << "insert " << num << " variable-size records into dummy.03" << endl;
    int smallest, largest;
    for(i = 0; i < num; i++) {
        rec1Len = 2 + rand() % (sizeof(rec1.s)-2);    // includes NULL!!
        //cout << "record length is " << rec1Len << endl;
        memset((void *)rec1.s, 32 + rec1Len, rec1Len - 1);
        rec1.s[rec1Len-1] = 0;
        rec1Len += sizeof(rec1.i) + sizeof(rec1.f);
        rec1.i = i;
        rec1.f = rec1Len;
        dbrec1.data = &rec1;
        dbrec1.length = rec1Len;

        status = iScan->insertRecord(dbrec1, newRid);
        if (status != OK) 
        {
            cout << "got err0r status return from insertrecord" << endl;
            error.print(status);
        }
        if (i == 0 || rec1Len < smallest) smallest = rec1Len;
        if (i == 0 || rec1Len > largest) largest = rec1Len;
    }
    cout << "inserted " << num << " variable sized records successfully into dummy.03" << endl;
    cout << "smallest record was " << smallest << " bytes, largest was "
         << largest << " bytes" << endl;
    
    delete iScan;
    file1 = NULL;

    cout << endl << "scan dummy.03 using the predicate < num/2 " << endl;
    j = num/2;
    scan1 = new HeapFileScan("dummy.03", status);
    if (status != OK) error.print(status);
    else 
    {
        scan1->startScan(0, sizeof(int), INTEGER, (char*)&j, LT);
        i = 0;

        while ((status = scan1->scanNext(rec2Rid)) != FILEEOF)
        {
            status = scan1->getRecord(dbrec2);
            if (status != OK) break;
            memcpy(&rec2, dbrec2.data, dbrec2.length);
            if (rec2.i >= j || rec2.f != dbrec2.length || rec2.s[0] != 32+dbrec2.length-8)
                cout << "err0r reading record " << i << " back" << endl;
            i++;
        }
        if (status != FILEEOF)
            error.print(status);
        cout << "scan of dummy.03 saw " << i << " records " << endl;
        if (i != num / 2)
            cout << "Err0r.   scan should have returned " << num / 2
                 << " records!" << endl;
    }

    scan1->endScan();
    delete scan1;
    scan1 = NULL;


    //================================================

    cout << endl;
    cout << "Next attempt two concurrent scans on dummy.03 " << endl;
    int Ioffset = (char*)&rec1.i - (char*)&rec1;
    int Ivalue = num/2;
    int Foffset = (char*)&rec1.f - (char*)&rec1;
    float Fvalue = 0;

    scan1 = new HeapFileScan("dummy.03", status);
    if (status != OK) error.print(status);
    status = scan1->startScan(Ioffset, sizeof(int), INTEGER,
                              (char*)&Ivalue, LT);
    if (status != OK) error.print(status);
  
    scan2 = new HeapFileScan("dummy.03", status);
    if (status != OK) error.print(status);
    status = scan2->startScan(Foffset, sizeof(float), FLOAT,
                              (char*)&Fvalue, GTE);
    if (status != OK) error.print(status);
    else 
    {
        int count = 0;
        for(i = 0; i < num; i++)
        {
            status = scan1->scanNext(rec2Rid);
            if (status == OK) count++;
            else if (status != FILEEOF) error.print(status);
            status = scan2->scanNext(rec2Rid);
            if (status == OK) count++;
            else if (status != FILEEOF) error.print(status);
        }
        cout << "scan file1 saw " << count << " records " << endl;
        if (count != num/2 + num)
            cout << "Err0r.   scan should have returned " << num/2 + num
                 << " records!" << endl;
        if (scan1->scanNext(rec2Rid) != FILEEOF)
            cout << "Err0r.   scan past end of file did not return FILEEOF!" << endl;
        if (scan2->scanNext(rec2Rid) != FILEEOF)
            cout << "Err0r.   scan past end of file did not return FILEEOF!" << endl;
    }
    delete scan1;
    delete scan2;
    scan1 = scan2 = NULL;
	
    cout << endl;
    cout << "Destroy dummy.03" << endl;
    if ((status = destroyHeapFile("dummy.03")) != OK) {
        cout << "got err0r status return from destroy file" << endl;
        error.print(status);
    }

    status = createHeapFile("dummy.04");
    if (status != OK) 
    {
	cerr << "got err0r status return from  createHeapFile" << endl;
    	error.print(status);
    }

    iScan = new InsertFileScan("dummy.04", status);
    if (status != OK) 
    {
	cerr << "got err0r status return from new insertFileScan" << endl;
    	error.print(status);
    }
    cout << "inserting " << num << " records into dummy1" << endl;
    for(i = 0; i < num; i++) {
        sprintf(rec1.s, "This is record %05d", i);
        rec1.i = i;
        rec1.f = i;

        dbrec1.data = &rec1;
        dbrec1.length = sizeof(RECORD);
        status = iScan->insertRecord(dbrec1, newRid);
        if (status != OK) 
        {
            cout << "got err0r status return from insertrecord" << endl;
            error.print(status);
        }
    }
    delete iScan;
    file1 = NULL;
	
	

    //bufMgr->clearBufStats();
    int numDeletes = 0;
    cout << endl;
    cout << "now scan dummy.04, deleting records with keys between 1000 and 2000" << endl;

    scan1 = new HeapFileScan("dummy.04", status);
    if (status != OK) error.print(status);
    status = scan1->startScan(0, 0, STRING, NULL, EQ);
    if (status != OK) error.print(status);
    else 
    {
  	i = 0;
  	while ((status = scan1->scanNext(rec2Rid)) != FILEEOF)
	{
	    // reconstruct record i
    	    sprintf(rec1.s, "This is record %05d", i);
    	    rec1.i = i;
    	    rec1.f = i;
    	    status = scan1->getRecord(dbrec2);
    	    if (status != OK) break;
	    if (memcmp(&rec1, dbrec2.data, sizeof(RECORD)) != 0)
                cout << "err0r reading record " << i << " back" << endl;

	    if ((i > 1000) && ( i <= 2000))
            {
                status = scan1->deleteRecord();
                if ((status != OK) && ( status != NORECORDS))
                {
                    cout << "err0r status return from deleteRecord" << endl;
                    error.print(status);
                }
                else numDeletes++;
            }
    	    i++;
  	}
	if (status != FILEEOF) error.print(status);
  	cout << "scan file1 saw " << i << " records " << endl;
	if (i != num)
            cout << "Err0r. scan should have returned " << (int) num 
		<< " records!!" << endl;
        cout << "number of records deleted by scan " << numDeletes << endl;
	if (numDeletes != 1000)
	    cout << "Err0r. Should have deleted 1000 records!!!" << endl;
    }
    cout << endl;
    delete scan1;
	


    // rescan.  should see 1000 fewer records

    scan1 = new HeapFileScan("dummy.04", status);
    if (status != OK) error.print(status);
    scan1->startScan(0, 0, STRING, NULL, EQ);
    i = 0;
    while ((status = scan1->scanNext(rec2Rid)) != FILEEOF)
    {
	i++;
    }
    cout << "should have seen 1000 fewer records after deletions" << endl;
    cout << "saw " << i << "records" << endl;
    delete scan1;
	

    // perform filtered scan #1
    scan1 = new HeapFileScan("dummy.04", status);
    if (status != OK) error.print(status);

    int filterVal1 = num * 3 / 4;
    cout << endl << "Filtered scan matching i field GTE than " << filterVal1 << endl;
    status = scan1->startScan(0, sizeof(int), INTEGER, (char *) &filterVal1, GTE);
    if (status != OK) 
    {
	cerr << "got err0r status return from startScan" << endl;
    	error.print(status);
    }
    else
    {
        // do scan
        i = 0;
        while ((status = scan1->scanNext(rec2Rid)) != FILEEOF)
        {
            // reconstruct record i
            status = scan1->getRecord(dbrec2);
            if (status != OK) break;
            // verify that the scan predicate was actually met
            RECORD *currRec = (RECORD *) dbrec2.data;
            if (! (currRec->i >= filterVal1))
            {
                cerr << "Err0r.   filtered scan returned record that doesn't satisfy predicate "
                     << "i val is " << currRec->i << endl;
                exit(1);
            }
            
            i++;
        }
        if (status != FILEEOF) error.print(status);
        cout << "scan file1 saw " << i << " records " << endl;
        if (i != num/4)
            cout << "Err0r.   scan should have returned " << num/4 << " records!"
                 << " returned "<< i <<endl;
    }

    delete scan1;
	
    // perform filtered scan #2
    scan1 = new HeapFileScan("dummy.04", status);
    if (status != OK) error.print(status);
    // perform filtered scan #2
    float filterVal2 = num * 9 / 10;
    cout << endl << "Filtered scan matching f field GT than " << filterVal2 << endl;
    status = scan1->startScan(sizeof(int), sizeof(float), FLOAT, (char *) &filterVal2, GT);
    if (status != OK) 
    {
	cerr << "got err0r status return from startScan" << endl;
    	error.print(status);
    }
    else
    {
        // do scan
        i = 0;
        while ((status = scan1->scanNext(rec2Rid)) != FILEEOF)
        {
            // reconstruct record i
            status = scan1->getRecord(dbrec2);
            if (status != OK) break;
            // verify that the scan predicate was actually met
            RECORD *currRec = (RECORD *) dbrec2.data;
            if (! (currRec->f > filterVal2))
            {
                cerr << "Err0r.   filtered scan returned record that doesn't satisfy predicate "
                     << "f val is " << currRec->f << endl;
                exit(1);
            }
//            cout << "scan return: " << currRec->f << endl;
            i++;
        }
        if (status != FILEEOF) error.print(status);
        cout << "scan file1 saw " << i << " records " << endl;
        if (i != num/10-1)
            cout << "Err0r.   filtered scan 2 should have returned " << num/10-1 << " records!"
                 << endl;
    }
    delete scan1;
	
	
    // open up the heapFile
    file1 = new HeapFile("dummy.04", status);
    if (status != OK) 
    {
	cerr << "got err0r status return from new HeapFile" << endl;
    	error.print(status);
    }
    delete file1;


    // cout << "BUFSTATS:" << bufMgr->getBufStats() << endl;
    // bufMgr->clearBufStats();

    scan1 = new HeapFileScan("dummy.04", status);
    if (status != OK) error.print(status);
    status = scan1->startScan(0, 20, INTEGER, "Hi" , EQ);
    if ( status == BADSCANPARM)
    {
        cout << endl << "passed BADSCANPARAM test" << endl;
    }
    else
    {
        cout << "should have returned BADSCANPARM, actually returned: " << endl;
        error.print(status);
    }
    
    // add insert for bigger than pagesized record
    iScan = new InsertFileScan("dummy.04", status);
    if (status != OK) error.print(status);
    char bigdata[8192];
    sprintf(bigdata, "big record");
    dbrec1.data = (void *) &bigdata;
    dbrec1.length = 8192;
    status = iScan->insertRecord(dbrec1, rec2Rid);
    if ((status == INVALIDRECLEN) || (status == NOSPACE))
    {
        cout << endl << "passed large record insert test" << endl;
    }
    else
    {
        cout << "got err0r status return from insert record " << endl;
        error.print(status);
        cout << "expected INVALIDRECLEN or NOSPACE" << endl;
    }
    delete iScan;

    delete scan1;

    // MORE ERROR HANDLING TESTS HERE
  
    // get rid of the file
    if ((status = destroyHeapFile("dummy.04")) != OK) {
        cout << endl << "got err0r status return from destroy file" << endl;
        error.print(status);
    }
    delete bufMgr;

    cout << endl << "Done testing." << endl;
    return 1;

}


