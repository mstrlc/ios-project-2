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
#include <stdbool.h>
#include <stdarg.h>
#include <fcntl.h>
#include <sys/stat.h>

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

// Function definition
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

void oxygenProcess(int idO, int NO, int TI, int TB)
{
    srand(time(NULL) * getpid());

    // Started
    myPrint("O %d: started\n", idO);

    // Sleep <0, TI>
    usleep(((rand() % (TI + 1)) * 1000));
    
    // Going to queue
    myPrint("O %d: going to queue\n", idO);    

    // Creating molecule
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

    sem_wait(s_maxMoleculeCheckO);
    if (*moleculeCounter > *maxMoleculeCount)
    {
        myPrint("O %d: not enough H\n", idO);
        sem_post(s_maxMoleculeCheckO);
        // sem_post(s_mutex);
        exit(0);
    }

    sem_wait(s_oxyQueue);

    barrierWait1();

    myPrint("O %d: creating molecule %d\n", idO, (*moleculeCounter));

    // Sleep <0, TB>
    usleep(((rand() % (TB + 1)) * 1000));

    // Molecule created    
    myPrint("O %d: molecule %d created\n", idO, (*moleculeCounter));

    sem_post(s_moleculeCreated);
    sem_post(s_moleculeCreated);

    barrierWait2();

    (*moleculeCounter)++;

    sem_post(s_maxMoleculeCheckO);
    sem_post(s_maxMoleculeCheckH);
    sem_post(s_maxMoleculeCheckH);

    sem_post(s_mutex);

    exit(0);
}

void hydrogenProcess(int idH, int NH, int TI)
{
    srand(time(NULL) * getpid());

    // Started
    myPrint("H %d: started\n", idH);

    // Sleep <0, TI>
    usleep(((rand() % (TI + 1)) * 1000));
    
    // Going to queue
    myPrint("H %d: going to queue\n", idH);
    
    // Creating molecule
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

    sem_wait(s_maxMoleculeCheckH);
    if (*moleculeCounter > *maxMoleculeCount)
    {
        myPrint("H %d: not enough O or H\n", idH);
        sem_post(s_maxMoleculeCheckH);

        exit(0);
    }

    sem_wait(s_hydroQueue);

    barrierWait1();
    
    myPrint("H %d: creating molecule %d\n", idH, (*moleculeCounter));

    sem_wait(s_moleculeCreated);

    myPrint("H %d: molecule %d created\n", idH, (*moleculeCounter));

    barrierWait2();

    exit(0);
}

// Main function
int main(int argc, char const *argv[])
{
    // Destroy shared memory preventively
    destroySharedMemory();
    destroySemaphores();

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

    initSharedMemory();
    initSempahores();

    // Create file for output
    fp = fopen("proj2.out", "w");
    if (fp == NULL)
    {
        fprintf(stderr, "Problem creating file.\n");
        return 1;
    }

    setbuf(fp, NULL);
    setbuf(stdout, NULL);

    *sequenceCounter = 1;
    *oxygen = 0;
    *hydrogen = 0;
    *barrierCount = 0;
    *moleculeCounter = 1;

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
            oxygenProcess(i, NO, TI, TB);
            return 0;
        }
    }

    for (int i = 1; i <= NH; i++)
    {
        pid_t hydrogenPid = fork();
        if (hydrogenPid == 0) // Child process
        {
            hydrogenProcess(i, NH, TI);
            return 0;
        }
    }

    // Wait for processes to finish
    while(wait(NULL) > 0);
    
    fclose(fp);    
    
    destroySharedMemory();
    destroySemaphores();

    return 0;
}