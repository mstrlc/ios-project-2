#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>

// Semaphore pointers
sem_t *s_writeFile = NULL;

sem_t *s_mutex = NULL;
sem_t *s_oxyQueue = NULL;
sem_t *s_hydroQueue = NULL;

sem_t *s_barrierMutex = NULL;
sem_t *s_barrierTurnstile = NULL;
sem_t *s_barrierTurnstile2 = NULL;

sem_t *s_maxMoleculeCheckO = NULL;
sem_t *s_maxMoleculeCheckH = NULL;

sem_t *s_moleculeCreated = NULL;

// Shared memory pointers
int *maxMoleculeCount = NULL;

int *moleculeCounter = NULL;
int *sequenceCounter = NULL;

int *oxygen = NULL;
int *hydrogen = NULL;

int *barrierCount = NULL;

// Output file pointer
FILE *fp;

// Semaphore initialization
void initSempahores()
{    
    sem_init(s_writeFile, 1, 1);

    sem_init(s_mutex, 1, 1);
    sem_init(s_oxyQueue, 1, 0);
    sem_init(s_hydroQueue, 1, 0);

    sem_init(s_barrierMutex, 1, 1);
    sem_init(s_barrierTurnstile, 1, 0);
    sem_init(s_barrierTurnstile2, 1, 1);

    sem_init(s_maxMoleculeCheckO, 1, 1);
    sem_init(s_maxMoleculeCheckH, 1, 2);
    sem_init(s_moleculeCreated, 1, 0);
}

// Shared memory initialization
void initSharedMemory()
{
    s_writeFile = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);

    s_mutex = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    s_oxyQueue = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    s_hydroQueue = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);

    s_barrierMutex = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    s_barrierTurnstile = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    s_barrierTurnstile2 = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);

    s_maxMoleculeCheckO  = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    s_maxMoleculeCheckH = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    s_moleculeCreated = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);

    maxMoleculeCount = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    
    moleculeCounter = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    sequenceCounter = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    
    oxygen = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    hydrogen = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    
    barrierCount = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
}

// Semaphore cleanup
void destroySemaphores()
{
    sem_destroy(s_writeFile);

    sem_destroy(s_mutex);
    sem_destroy(s_oxyQueue);
    sem_destroy(s_hydroQueue);

    sem_destroy(s_barrierMutex);
    sem_destroy(s_barrierTurnstile);
    sem_destroy(s_barrierTurnstile2);

    sem_destroy(s_maxMoleculeCheckO);
    sem_destroy(s_maxMoleculeCheckH);
    sem_destroy(s_moleculeCreated);
}

// Shared memory cleanup
void destroySharedMemory()
{
    munmap(s_writeFile, sizeof(sem_t));

    munmap(s_mutex, sizeof(sem_t));
    munmap(s_oxyQueue, sizeof(sem_t));
    munmap(s_hydroQueue, sizeof(sem_t));

    munmap(s_barrierMutex, sizeof(sem_t));
    munmap(s_barrierTurnstile, sizeof(sem_t));
    munmap(s_barrierTurnstile2, sizeof(sem_t));

    munmap(s_maxMoleculeCheckO, sizeof(sem_t));
    munmap(s_maxMoleculeCheckH, sizeof(sem_t));
    munmap(s_moleculeCreated, sizeof(sem_t));

    munmap(maxMoleculeCount, sizeof(int));

    munmap(moleculeCounter, sizeof(int));
    munmap(sequenceCounter, sizeof(int));
    
    munmap(oxygen, sizeof(int));
    munmap(hydrogen, sizeof(int));
    
    munmap(barrierCount, sizeof(int));
}

// Print function for easier printing
// Print given string to output file with sequence
// Semaphore is used to prevent problems with print 
void myPrint(const char *format, ...)
{
    sem_wait(s_writeFile);
    va_list args;
    va_start(args, format);
    fprintf(fp, "%d: ", *sequenceCounter);
    fflush(fp);
    vfprintf(fp, format, args);
    fflush(fp);
    (*sequenceCounter)++;
    va_end(args);
    sem_post(s_writeFile);
}

// Barrier function
// Inspired by the Little Book of Semaphores
// Part one is before critical section, waits for all processes to reach the barrier
void barrierWait1()
{
    sem_wait(s_barrierMutex);
    (*barrierCount)++;
    if (*barrierCount == 3)
    {
        sem_wait(s_barrierTurnstile2);
        sem_post(s_barrierTurnstile);
    }
    sem_post(s_barrierMutex);

    sem_wait(s_barrierTurnstile);
    sem_post(s_barrierTurnstile);
}

// After the critical section
void barrierWait2()
{
    sem_wait(s_barrierMutex);
    (*barrierCount)--;
    if (*barrierCount == 0)
    {
        sem_wait(s_barrierTurnstile);
        sem_post(s_barrierTurnstile2);
    }
    sem_post(s_barrierMutex);

    sem_wait(s_barrierTurnstile2);
    sem_post(s_barrierTurnstile2);
}

// Process of oxygen
void oxygenProcess(int idO, int TI, int TB)
{
    // Set the seed of rand to get truly random numbers
    srand(time(NULL) * getpid());

    // Started
    myPrint("O %d: started\n", idO);

    // Sleep <0, TI>
    usleep(((rand() % (TI + 1)) * 1000));
    
    // Going to queue
    myPrint("O %d: going to queue\n", idO);    

    // Creating molecule
    // Inspired by the Little Book of Semaphores
    sem_wait(s_mutex);

    (*oxygen)++;
    if(*hydrogen >= 2)
    {
        sem_post(s_hydroQueue);
        sem_post(s_hydroQueue);
        (*hydrogen) -= 2;
        sem_post(s_oxyQueue);
        (*oxygen) -= 1;
    }
    else
    {
        sem_post(s_mutex);
    }

    // Check if the number of molecule that would be created is not larger than the number of molecules that can be created
    // If it is, we know that the process wouldn't be able to create a molecule so it is ended
    sem_wait(s_maxMoleculeCheckO);
    if (*moleculeCounter > *maxMoleculeCount)
    {
        sem_post(s_maxMoleculeCheckO);
        myPrint("O %d: not enough H\n", idO);
        exit(0);
    }

    sem_wait(s_oxyQueue);

    // Creating molecule
    myPrint("O %d: creating molecule %d\n", idO, (*moleculeCounter));

    // Sleep <0, TB>
    usleep(((rand() % (TB + 1)) * 1000));

    barrierWait1();

    // Tell the two other hydrogen atoms that the molecule is created
    sem_post(s_moleculeCreated);
    sem_post(s_moleculeCreated);

    // Molecule created    
    myPrint("O %d: molecule %d created\n", idO, (*moleculeCounter));

    barrierWait2();

    (*moleculeCounter)++;

    // Post the semaphore for checking the number of molecules to max number
    sem_post(s_maxMoleculeCheckO);
    sem_post(s_maxMoleculeCheckH);
    sem_post(s_maxMoleculeCheckH);

    sem_post(s_mutex);

    // Exit successfully
    exit(0);
}

// Process of hydrogen
void hydrogenProcess(int idH, int TI)
{
    // Set the seed of rand to get truly random numbers
    srand(time(NULL) * getpid());

    // Started
    myPrint("H %d: started\n", idH);

    // Sleep <0, TI>
    usleep(((rand() % (TI + 1)) * 1000));
    
    // Going to queue
    myPrint("H %d: going to queue\n", idH);
    
    // Creating molecule
    // Inspired by the Little Book of Semaphores
    sem_wait(s_mutex);

    (*hydrogen)++;
    if((*hydrogen) >= 2 && (*oxygen) >= 1)
    {
        sem_post(s_hydroQueue);
        sem_post(s_hydroQueue);
        (*hydrogen) -= 2;
        sem_post(s_oxyQueue);
        (*oxygen) -= 1;
    }
    else
    {        
        sem_post(s_mutex);
    }

    // Check if the number of molecule that would be created is not larger than the number of molecules that can be created
    // If it is, we know that the process wouldn't be able to create a molecule so it is ended
    sem_wait(s_maxMoleculeCheckH);
    if (*moleculeCounter > *maxMoleculeCount)
    {
        myPrint("H %d: not enough O or H\n", idH);
        sem_post(s_maxMoleculeCheckH);

        exit(0);
    }

    sem_wait(s_hydroQueue);

    // Creating molecule
    myPrint("H %d: creating molecule %d\n", idH, (*moleculeCounter));

    barrierWait1();    

    // Wait until oxygen molecule tells the hydrogens that the molecule is created
    sem_wait(s_moleculeCreated);

    // Molecule created
    myPrint("H %d: molecule %d created\n", idH, (*moleculeCounter));

    barrierWait2();

    // Exit successfully
    exit(0);
}

// Check if a string is a number
// Used in argument checking
// All positive numbers return the number
// Negative numbers, empty strings or strings with characters other than 0-9 return -1
int isNumber(char *string)
{
    bool isNumber = true;

    if(strcmp(string, "") == 0)
    {
        isNumber = false;
    }
    else
    {
        for (size_t i = 0; i < strlen(string); i++)
        {
            if(!isdigit(string[i]))
            {
                isNumber = false;
                break;
            }
        }
    }

    if(isNumber)
    {
        return atoi(string);
    }
    else
    {
        return -1;
    }    
}

// Main function
int main(int argc, char *argv[])
{
    // Destroy shared memory preventively
    destroySharedMemory();
    destroySemaphores();

    // Get command line arguments and check for validity
    if (argc != 5)
    {
        fprintf(stderr, "Wrong arguments supplied.\n");
        return 1;
    }

    int NO = isNumber(argv[1]); // Number of oxygens
    int NH = isNumber(argv[2]); // Number of hydrogens
    int TI = isNumber(argv[3]); // Max time before queue
    int TB = isNumber(argv[4]); // Max time to create molecule

    if (NO < 1 || NH < 1 || TI < 0 || TI > 1000 || TB < 0 || TB > 1000) // Check if arguments are valid
    {
        fprintf(stderr, "Wrong arguments supplied.\n");
        return 1;
    }

    initSharedMemory();
    initSempahores();

    // Create file for output / open for reading
    fp = fopen("proj2.out", "w");
    if (fp == NULL)
    {
        fprintf(stderr, "Problem creating file.\n");
        destroySharedMemory();
        destroySemaphores();
        return 1;
    }

    // Set buffering to avoid problems with printing
    setbuf(fp, NULL);

    // Initialize shared variables
    *oxygen = 0;          // Number of queing oxygens 
    *hydrogen = 0;        // Number of queing hydrogens
    *barrierCount = 0;    // Number of processes in barrier

    *sequenceCounter = 1; // Order of prints
    *moleculeCounter = 1; // Number of molecules created

    // Calculate the maximum number of molecules
    // Used to check if there is enough atoms
    if (NO < 1 || NH < 2)
    {
        *maxMoleculeCount = 0;
    }
    else if (NO > NH/2)
    {
        *maxMoleculeCount = NH/2; 
    }
    else
    {
        *maxMoleculeCount = NO;
    }

    // Create processes to create oxygen and hydrogen
    for (int i = 1; i <= NO; i++)
    {
        pid_t oxygenPid = fork();
        if (oxygenPid == 0) // Child process
        {
            oxygenProcess(i, TI, TB);
            return 0;
        }
    }

    for (int i = 1; i <= NH; i++)
    {
        pid_t hydrogenPid = fork();
        if (hydrogenPid == 0) // Child process
        {
            hydrogenProcess(i, TI);
            return 0;
        }
    }

    // Wait for all child processes to finish
    while(wait(NULL) > 0);
    
    fclose(fp);    

    // Deallocate resources
    destroySharedMemory();
    destroySemaphores();

    return 0;
}