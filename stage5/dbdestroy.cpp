#include <sys/types.h>
#include <functional>
#include <string.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <sstream>
#include <vector>
using namespace std;


int main(int argc, char *argv[])
{
  if (argc < 2) {
    cerr << "Usage: " << argv[0] << " dbname" << endl;
    return 1;
  }

  char command[128];

  cout << "Enter y if you want to delete " << argv[1] << "/*" << endl;

  if (scanf("%s", command) >= 1
      && (command[0] == 'y' || command[0] == 'Y')) {
    sprintf(command, "rm -r %s", argv[1]);
    cout << "Executing " << command << endl;
    return system(command);
  }

  cout << "Database not destroyed." << endl;

  return 0;
}
