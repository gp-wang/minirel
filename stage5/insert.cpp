#include "catalog.h"
#include "query.h"


/*
 * Inserts a record into the specified relation.
 * The value of the attribute is supplied in the attrValue member of the attrInfo structure.
 * If the order of the attributes in attrList[] is not the same as in the relation, they
 * will be rearranged first.
 * <i> If no value is specified for an attribute, the insertion is rejected.</i> 
 * @param relation
 * @param attrCnt
 * @param attrList[]
 * @return: OK on success
 * an error code otherwise
 */

const Status QU_Insert(const string & relation,
        const int attrCnt,
        const attrInfo attrList[])
{

    Status status;
    int relAttrCnt;
    AttrDesc* relAttrs;
    //open InsertFileScan object to seaarch for relation
    InsertFileScan resultRel(relation, status);
    if (status != OK) { return status; }
    // get the relation catalog info
    status = attrCat->getRelInfo(relation, relAttrCnt, relAttrs);
    if (status != OK) { return status; }
    //find the total record size
    int reclen = 0;
    for (int i = 0; i < relAttrCnt; i++) {
        reclen += relAttrs[i].attrLen;
    }
    //initialize pointer to a location of size reclen
    char outputData[reclen];
    Record outputRec;
    outputRec.data = (void *) outputData;
    outputRec.length = reclen;

    for (int i = 0; i < attrCnt; i++) {
        //Iterate through looking for a match
        for (int j = 0; j < relAttrCnt; j++) {
            if (strcmp(relAttrs[j].attrName, attrList[i].attrName) == 0) {
                //NULL values not allowed by Minirel
                if (attrList[i].attrValue == NULL) {
                    return ATTRTYPEMISMATCH; //Not sure if this is the right error
                }

                char* actualAttrValue;
                int tmpInt;
                float tmpFloat;
                //convert to proper data type
                switch (attrList[i].attrType) {
                    case INTEGER:
                        tmpInt = atoi((char*)attrList[i].attrValue);
                        actualAttrValue = (char*)&tmpInt;
                        break;
                    case FLOAT:
                        tmpFloat = atof((char*)attrList[i].attrValue);
                        actualAttrValue = (char*)&tmpFloat;
                        break;
                    case STRING:
                        actualAttrValue = (char*)attrList[i].attrValue;
                        break;
                }
                // we have a match, copy data into the output record
                memcpy(outputData + relAttrs[j].attrOffset, actualAttrValue, relAttrs[j].attrLen);
            }//end if strcmp 
        }//end for j
    }//end for i

    RID outRID;
    //Done creating record, inserting it into table
    status = resultRel.insertRecord(outputRec, outRID);
    if (status != OK) { return status; }
    //if reached tuple inserted with no issues
    return OK;

}

