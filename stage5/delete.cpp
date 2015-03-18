#include "catalog.h"
#include "query.h"


/*
 * Delete all tuples in relation satisfying the specified predicate.
 * <i>If no predicate given then delete all tuple for this relation.</i>
 *
 * @param relation
 * @param attrName
 * @param op
 * @param type
 * @param attrValue
 * @return: OK on success
 * an error code otherwise
 */

#include "stdio.h"
#include "stdlib.h"
const Status QU_Delete(const string & relation,
		       const string & attrName,
		       const Operator op,
		       const Datatype type,
		       const char *attrValue)
{
    Status status;
    AttrDesc attrDesc;
    const char* filter;
    //keeps track of how many tuples will be deleted
    int resultTupCnt = 0;
    RID relRID;
    HeapFileScan relScan(relation, status);
    if (status != OK) { return status; }
    //if no attrName given then delete all rows in relation
    if(attrName.length() == 0){
       
       status = relScan.startScan(0, 0, STRING, NULL, EQ);
       if (status != OK) { return status; }
       
       while (relScan.scanNext(relRID) == OK) {
          status = relScan.deleteRecord();
          if (status != OK) { return status; }
              resultTupCnt++;
       }
       printf("deleted %d result tuples \n", resultTupCnt);
       return OK;
    }
       
    //gather info for the search
    status = attrCat->getInfo(relation, attrName, attrDesc);
    if (status != OK) { return status; }

    int tmpInt;
    float tmpFloat;
    //convert to proper data type
    switch (type) {
        case INTEGER:
            tmpInt = atoi(attrValue);
            filter = (char*)&tmpInt;
            break;
        case FLOAT:
            tmpFloat = atof(attrValue);
            filter = (char*)&tmpFloat;
            break;
        case STRING:
            filter = attrValue;
            break;
    }
    //with the book keeping finished, actually scan through the tuples
    status = relScan.startScan(attrDesc.attrOffset, attrDesc.attrLen, type, filter, op);
    if (status != OK) { return status; }

    while (relScan.scanNext(relRID) == OK) {
        //we have match. delete the tuple
        status = relScan.deleteRecord();
        if (status != OK) { return status; }
        resultTupCnt++;
    }

    printf("deleted %d result tuples \n", resultTupCnt);

    //if reached tuples deleted with no issues
    return OK;
}


