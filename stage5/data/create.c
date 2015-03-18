#include <stdio.h>

int errno;

typedef struct {
  int soapid;
  char sname[28];
  char network[4];
  float rating;
} Soaps;

typedef struct {
  int starid;
  char stname[20];
  char plays[12];
  int soapid;
} Stars;

main()
{
  FILE *fp;
  
  Soaps soaps[] = {
    { 0, "Days of Our Lives", "NBC", 7.02 },
    { 1, "General Hospital", "ABC", 9.81 },
    { 2, "Guiding Light", "CBS", 4.02 },
    { 3, "One Life to Live", "ABC", 2.31 },
    { 4, "Santa Barbara", "NBC", 6.44 },
    { 5, "The Young and the Restless", "CBS", 5.50 },
    { 6, "As the World Turns", "CBS", 7.00 },
    { 7, "Another World", "NBC", 1.97 },
    { 8, "All My Children", "ABC", 8.82 }
  };

  Stars stars[29] = {
    { 0, "Hayes, Kathryn", "Kim", 6 },
    { 1, "DeFreitas, Scott", "Andy", 6 },
    { 2, "Grahn, Nancy", "Julia", 4 },
    { 3, "Linder, Kate", "Esther", 5 },
    { 4, "Cooper, Jeanne", "Katherine", 5 },
    { 5, "Ehlers, Beth", "Harley", 2 },
    { 6, "Novak, John", "Keith", 4 },
    { 7, "Elliot, Patricia", "Renee", 3 },
    { 8, "Hutchinson, Fiona", "Gabrielle", 5 },
    { 9, "Carey, Phil", "Asa", 5 },
    { 10, "Walker, Nicholas", "Max", 3 },
    { 11, "Ross, Charlotte", "Eve", 0 },
    { 12, "Anthony, Eugene", "Stan", 8 },
    { 13, "Douglas, Jerry", "John", 5 },
    { 14, "Holbrook, Anna", "Sharlene", 7 },
    { 15, "Hammer, Jay", "Fletcher", 2 },
    { 16, "Sloan, Tina", "Lillian", 2 },
    { 17, "DuClos, Danielle", "Lisa", 3 },
    { 18, "Tuck, Jessica", "Megan", 3 },
    { 19, "Ashford, Matthew", "Jack", 0 },
    { 20, "Novak, John", "Keith", 4 },
    { 21, "Larson, Jill", "Opal", 8 },
    { 22, "McKinnon, Mary", "Denise", 7 },
    { 23, "Barr, Julia", "Brooke", 8 },
    { 24, "Borlenghi, Matt", "Brian", 8 },
    { 25, "Hughes, Finola", "Anna", 1 },
    { 26, "Rogers, Tristan", "Robert", 1 },
    { 27, "Richardson, Cheryl", "Jenny", 1 },
    { 28, "Evans, Mary Beth", "Kayla", 0 }
  };

  int nitems;

  /* create Soaps */
  fp = fopen("stars.data","wb");
  nitems = fwrite( (void *)stars, sizeof(Stars), 29, fp);
  if ( nitems != 29 )
    fprintf(stderr, "Error in creating file\n");
  fclose(fp);

  /* create Stars */
  fp = fopen("soaps.data","wb");
  nitems = fwrite( (void *)soaps, sizeof(Soaps), 9, fp);
  if ( nitems != 9 )
    fprintf(stderr, "Error in creating file\n");
  fclose(fp);
}
