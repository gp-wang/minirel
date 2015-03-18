#include <stdio.h>
#include <string.h>
#include <iostream.h>
int errno;

typedef struct {
  int unique1;
  int unique2;
  int hundred1;
  int hundred2;
  char dummy[84];
} Rels;


main()
{
  FILE *fp;
  
  fp = fopen("rel500.data","r");

  Rels rel;
  for (int i = 0; i < 500; i++) {    
	if (fread((char*)&rel, sizeof(rel), 1, fp) < 1)
		fprintf(stderr, "Error in reading file\n");
    	cout << rel.unique1 << "\t" << rel.unique2 << "\t"
	     << rel.hundred1 << "\t" << rel.hundred2 << "\t"
	     << rel.dummy << endl;
  }
  fclose(fp);

  fp = fopen("rel1000.data","r");

  for (i = 0; i < 1000; i++) {    
        if (fread((char*)&rel, sizeof(rel), 1, fp) < 1)
                fprintf(stderr, "Error in reading file\n");
        cout << rel.unique1 << "\t" << rel.unique2 << "\t"
             << rel.hundred1 << "\t" << rel.hundred2 << "\t"
             << rel.dummy << endl;

  }        
  fclose(fp);

}
