#include <memory.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include <iostream>
#include <stdio.h>
#include "page.h"
#include "buf.h"
#include "catalog.h"
#include "utility.h"

extern BufMgr *bufMgr;
extern RelCatalog *relCat;
extern AttrCatalog *attrCat;

//
// Closes the catalog files in preparation for shutdown.
//
// No return value.
//

void UT_Quit(void)
{
  // close relcat and attrcat

  delete relCat;
  delete attrCat;

  // delete bufMgr to flush out all dirty pages

  delete bufMgr;

  exit(1);
}
