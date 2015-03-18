#ifndef UTILITY_H
#define UTILITY_H

#include <memory.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include <iostream>
#include <math.h>
#include <stdio.h>
#include <sys/types.h>
#include <functional>
#include "error.h"
#include <string.h>
using namespace std;
#include "error.h"

// define if debug output wanted

//
// Prototypes for utility layer functions
//

const Status UT_Load(const string & relation, 
		     const string & fileName);

const Status UT_Print(string relation);

void   UT_Quit(void);

#endif
