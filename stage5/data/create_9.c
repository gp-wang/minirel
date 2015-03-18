#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

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
  
  int nitems, i;

  /* create rel500 */
  fp = fopen("rel500.data","wb");

  Rels rel;
  memset(rel.dummy, ' ', sizeof(rel.dummy));
  for (i = 0; i < 500; i++) {    
	rel.unique1 = rand() % 500 + 1;
	rel.unique2 = rand() % 500 + 1;
	rel.hundred1 = rand() % 100 + 1;
	rel.hundred2 = rand() % 100 + 1;
	sprintf(rel.dummy, "rel500.%3d", i);
	if (fwrite((void*)&rel, sizeof(rel), 1, fp) < 1)
		fprintf(stderr, "Error in creating file\n");
  }
  fclose(fp);

  /* create rel1000 */
  fp = fopen("rel1000.data","wb");

  for (i = 0; i < 1000; i++) {    
        rel.unique1 = rand() % 1000 + 1;
        rel.unique2 = rand() % 1000 + 1;
        rel.hundred1 = rand() % 100 + 1;
        rel.hundred2 = rand() % 100 + 1;
        sprintf(rel.dummy, "rel1000.%3d", i);
        if (fwrite((void*)&rel, sizeof(rel), 1, fp) < 1)
                fprintf(stderr, "Error in creating file\n");    
  }        
  fclose(fp);

}
