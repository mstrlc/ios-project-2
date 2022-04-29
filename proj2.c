#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <ctype.h>

// Semaphore pointers
sem_t * s_writeFile;

// Shared memory pointers
int *sequenceCounter = NULL;
int *oxygenCounter = NULL;
int *hydrogenCounter = NULL;

// Output file pointer
FILE *fp;

// Function definition
void initSempahores()
{
    s_writeFile = sem_open("/xstrel03/s_writeFile", O_CREAT, 0666, 1);
}

void initSharedMemory()
{
    sequenceCounter = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    oxygenCounter = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    hydrogenCounter = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
}

void destroySemaphores()
{
    sem_close(s_writeFile);
    sem_unlink("/xstrel03/s_writeFile");
}

void destroySharedMemory()
{
    munmap(sequenceCounter, sizeof(int));
    munmap(oxygenCounter, sizeof(int));
    munmap(hydrogenCounter, sizeof(int));
}

void barrier(int n)
{
}

void oxygenQueue(int oxygenID, int TI, int TB)
{
}

void hydrogenQueue(int hydrogenID)
{
}

void createOxygen(int NO, int TI, int TB)
{
    for (int i = 0; i < NO; i++)
    {
        pid_t oxygen = fork();
        if (oxygen == 0) // Child process
        {
            sem_wait(s_writeFile);
            int oxygenID = *oxygenCounter;
            fprintf(fp, "%d: O %d: started\n", *sequenceCounter, *oxygenCounter);
            (*sequenceCounter)++;
            (*oxygenCounter)++;
            fflush(fp);
            sem_post(s_writeFile);

            usleep(rand() % (TI+1) * 1000);
            
            sem_wait(s_writeFile);
            fprintf(fp, "%d: O %d: going to queue\n", *sequenceCounter, oxygenID);
            fflush(fp);
            (*sequenceCounter)++;
            sem_post(s_writeFile);

            oxygenQueue(oxygenID, TI, TB);

            exit(0);
        }
    }
}

void createHydrogen(int NH, int TI)
{
    for (int i = 0; i < NH; i++)
    {
        pid_t hydrogen = fork();
        if (hydrogen == 0) // Child process
        {
            sem_wait(s_writeFile);
            int hydrogenID = *hydrogenCounter;
            fprintf(fp, "%d: H %d: started\n", *sequenceCounter, *hydrogenCounter);
            (*sequenceCounter)++;
            (*hydrogenCounter)++;
            fflush(fp);
            sem_post(s_writeFile);

            usleep(rand() % (TI+1) * 1000);

            sem_wait(s_writeFile);
            fprintf(fp, "%d: H %d: going to queue\n", *sequenceCounter, hydrogenID);
            fflush(fp);
            (*sequenceCounter)++;
            sem_post(s_writeFile);

            hydrogenQueue(hydrogenID);

            exit(0);
        }
    }
}

// Main function
int main(int argc, char const *argv[])
{
    destroySemaphores();
    destroySharedMemory();

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

    if (NO < 0 || NH < 0 || TI < 0 || TI > 1000 || TB < 0 || TB > 1000) // Check if arguments are valid
    {
        fprintf(stderr, "Wrong arguments supplied.\n");
        return 1;
    }

    // Create file for output
    fp = fopen("proj2.out", "w");
    if (fp == NULL)
    {
        fprintf(stderr, "Problem creating file.\n");
        return 1;
    }

    initSempahores();
    initSharedMemory();

    setbuf(fp, NULL);
    setbuf(stdout, NULL);

    *sequenceCounter = 1;
    *oxygenCounter = 1;
    *hydrogenCounter = 1;

    // Create processes to create hydrogen and oxygen
    pid_t createOxygenProcess = fork();
    if (createOxygenProcess == 0)
    {
        createOxygen(NO, TI, TB);
        exit(0);
    }

    pid_t createHydrogenProcess = fork();
    if (createHydrogenProcess == 0)
    {
        createHydrogen(NH, TI);
        exit(0);
    }

    // Wait for processes to finish
    while(wait(NULL) > 0);

    destroySemaphores();
    destroySharedMemory();

    fclose(fp);    
    return 0;
}
