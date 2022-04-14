#include <stdio.h>
#include <stdlib.h>


int main(int argc, char const *argv[])
{
    // Get command line arguments
    if (argc != 5)
    {
        fprintf(stderr, "Wrong arguments supplied.\n");
        return 1;
    }
    int NO = atoi(argv[1]); // Number of oxygens
    int NH = atoi(argv[2]); // Number of hydrogens
    int TI = atoi(argv[3]); // Maximum time in ms for atom to wait before queing
    int TB = atoi(argv[4]); // Maximum time in ms needed to create one molecule

    if (TI < 0 || TI > 1000 || TB < 0 || TB > 1000) // Check if arguments are valid
    {
        fprintf(stderr, "Wrong arguments supplied.\n");
        return 1;
    }

    // ! debug
    printf("NO: %d, NH: %d, TI: %d, TB: %d\n", NO, NH, TI, TB);

    return 0;    
}
