//=============================================================================
// Generate simple WI Tuple unique1 tuples
//=============================================================================

#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define NUMRANDOMIZEPASSES 10

int main(int argc, char *argv[])
{
    // get command line args
    if (argc != 3)
    {
        fprintf(stderr, 
                "Usage: %s <total num tuples> <output filename>\n",
                argv[0]);
        return 1;
    }
    int tupleCount = atoi(argv[1]);
    char *outputFilename = argv[2];

    // gen the tuples
    int nums[tupleCount];

    // fill in in order
    for (int i = 0; i < tupleCount; i++)
    {
        nums[i] = i;
    }
    // randomize
    srand(time(NULL));
    for (int pass = 0; pass < NUMRANDOMIZEPASSES; pass++)
    {
        for (int i = 0; i < tupleCount; i++)
        {
            // swap curr entry to new pos
            int newPos = rand() % tupleCount;
            int tmpVal = nums[newPos];
            nums[newPos] = nums[i];
            nums[i] = tmpVal; 
        }
    }

    // write the tuples to the output file
    int outfd = open(outputFilename, O_WRONLY | O_CREAT | O_TRUNC);
    if (-1 == outfd)
    {
        perror("Error opening file for writing\n");
        exit(1);
    }
    // write the tuples
    for (int i = 0; i < tupleCount; i++)
    {
        if (write(outfd, &nums[i], sizeof(int)) == -1)
        {
            perror("Error writing tuple\n");
            return 1;
        }
    }
        
    close(outfd);
    printf("Done.\n");
    return 0;

} // end main

    
