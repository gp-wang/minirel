#include <stdio.h>
#include "catalog.h"
#include "utility.h"


#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) < (b) ? (a) : (b))


//
// Compute widths of columns in attribute array.
//

const Status UT_computeWidth(const int attrCnt, 
			     const AttrDesc attrs[], 
			     int *&attrWidth)
{
  attrWidth = new int [attrCnt];
  if (!attrWidth)
    return INSUFMEM;

  for(int i = 0; i < attrCnt; i++) {
    int namelen = strlen(attrs[i].attrName);
    switch(attrs[i].attrType) {
    case INTEGER:
    case FLOAT:
      attrWidth[i] = MIN(MAX(namelen, 5), 7);
      break;
    case STRING:
      attrWidth[i] = MIN(MAX(namelen, attrs[i].attrLen), 20);
      break;
    }
  }

  return OK;
}


//
// Prints values of attributes stored in buffer pointed to
// by recPtr. The desired width of columns is in attrWidth.
//

void UT_printRec(const int attrCnt, const AttrDesc attrs[], int *attrWidth,
		 const Record & rec)
{
  for(int i = 0; i < attrCnt; i++) {
    char *attr = (char *)rec.data + attrs[i].attrOffset;
    switch(attrs[i].attrType) {
    case INTEGER:
      int tempi;
      memcpy(&tempi, attr, sizeof(int));
      printf("%-*d  ", attrWidth[i], tempi);
      break;
    case FLOAT:
      float tempf;
      memcpy(&tempf, attr, sizeof(float));
      printf("%-*.2f  ", attrWidth[i], tempf);
      break;
    default:
      printf("%-*.*s  ", attrWidth[i], attrWidth[i], attr);
      break;
    }
  }

  printf("\n");
}


//
// Prints the contents of the specified relation.
//
// Returns:
// 	OK on success
// 	an error code otherwise
//

const Status UT_Print(string relation)
{
  Status status;
  RelDesc rd;
  AttrDesc *attrs;
  int attrCnt;

  if (relation.empty()) relation = RELCATNAME;

  // get relation data
  if ((status = relCat->getInfo(relation, rd)) != OK) return status;

  // get attribute data
  if ((status = attrCat->getRelInfo(rd.relName, attrCnt, attrs)) != OK)
    return status;

  // compute width of output columns

  int *attrWidth;
  if ((status = UT_computeWidth(attrCnt, attrs, attrWidth)) != OK)
    return status;

  // open data file
  HeapFileScan *hfile = new HeapFileScan(rd.relName, status);
  if (!hfile) return INSUFMEM;
  if (status != OK) return status;

  cout << "Relation name: " << rd.relName << endl << endl;

  int i;
  for(i = 0; i < attrCnt; i++) {
    printf("%-*.*s ", attrWidth[i], attrWidth[i],
	   attrs[i].attrName);
  }
  printf("\n");

  for(i = 0; i < attrCnt; i++) {
    for(int j = 0; j < attrWidth[i]; j++)
      putchar('-');
    printf("  ");
  }
  printf("\n");

  if ((status = hfile->startScan(0, 0, INTEGER, NULL, EQ)) != OK)
    return status;

  Record rec;
  RID rid;

  int records = 0;
  while((status = hfile->scanNext(rid)) == OK) {
    if ((status = hfile->getRecord(rec)) != OK)
      return status;
    UT_printRec(attrCnt, attrs, attrWidth, rec);
    records++;
  }
  if (status != FILEEOF)
    return status;

  cout << endl << "Number of records: " << records << endl;

  delete []attrWidth;
  free(attrs);

  // close scan and data file

  if ((status = hfile->endScan()) != OK)
    return status;
  delete hfile;

  return OK;
}
