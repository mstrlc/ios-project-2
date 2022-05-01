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
sem_t *s_writeFile;

sem_t *s_mutex;
sem_t *s_oxyQueue;
sem_t *s_hydroQueue;

sem_t *s_barrierMutex;
sem_t *s_barrierTurnstile;
sem_t *s_barrierTurnstile2;

sem_t *s_maxMoleculeCheckO;
sem_t *s_maxMoleculeCheckH;

sem_t *s_moleculeCreated;


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
    s_writeFile = sem_open("/xstrel03/s_writeFile", O_CREAT | O_EXCL, 0666, 1);

    s_mutex = sem_open("/xstrel03/s_mutex", O_CREAT | O_EXCL, 0666, 1);
    s_oxyQueue = sem_open("/xstrel03/s_oxyQueue", O_CREAT | O_EXCL, 0666, 0);
    s_hydroQueue = sem_open("/xstrel03/s_hydroQueue", O_CREAT | O_EXCL, 0666, 0);

    s_barrierMutex = sem_open("/xstrel03/s_barrierMutex", O_CREAT | O_EXCL, 0666, 1);
    s_barrierTurnstile = sem_open("/xstrel03/s_barrierTurnstile", O_CREAT | O_EXCL, 0666, 0);
    s_barrierTurnstile2 = sem_open("/xstrel03/s_barrierTurnstile2", O_CREAT | O_EXCL, 0666, 1);

    s_maxMoleculeCheckO = sem_open("/xstrel03/s_maxMoleculeCheck", O_CREAT | O_EXCL, 0666, 1);
    s_maxMoleculeCheckH = sem_open("/xstrel03/s_maxMoleculeCheck", O_CREAT | O_EXCL, 0666, 2);
    s_moleculeCreated = sem_open("/xstrel03/s_moleculeCreated", O_CREAT | O_EXCL, 0666, 0);
}

void initSharedMemory()
{
    maxMoleculeCount = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    sequenceCounter = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    oxygen = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    hydrogen = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    barrierCount = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    moleculeCounter = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
}

void destroySemaphores()
{
    sem_close(s_writeFile);
    sem_unlink("/xstrel03/s_writeFile");

    sem_close(s_mutex);
    sem_unlink("/xstrel03/s_mutex");

    sem_close(s_oxyQueue);
    sem_unlink("/xstrel03/s_oxyQueue");

    sem_close(s_hydroQueue);
    sem_unlink("/xstrel03/s_hydroQueue");
    
    sem_close(s_barrierMutex);
    sem_unlink("/xstrel03/s_barrierMutex");
    
    sem_close(s_barrierTurnstile);
    sem_unlink("/xstrel03/s_barrierTurnstile");
    sem_close(s_barrierTurnstile2);
    sem_unlink("/xstrel03/s_barrierTurnstile2");

    sem_close(s_maxMoleculeCheckO);
    sem_unlink("/xstrel03/s_maxMoleculeCheckO");
    sem_close(s_maxMoleculeCheckH);
    sem_unlink("/xstrel03/s_maxMoleculeCheckH");
    
    sem_close(s_moleculeCreated);
    sem_unlink("/xstrel03/s_moleculeCreated");

}

void destroySharedMemory()
{
    munmap(maxMoleculeCount, sizeof(int));
    munmap(sequenceCounter, sizeof(int));
    munmap(oxygen, sizeof(int));
    munmap(hydrogen, sizeof(int));
    munmap(barrierCount, sizeof(int));
    munmap(moleculeCounter, sizeof(int));
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
        return;
    }

    sem_wait(s_oxyQueue);

    myPrint("O %d: creating molecule %d\n", idO, (*moleculeCounter));

    // Sleep <0, TB>
    usleep(((rand() % (TB + 1)) * 1000));

    barrierWait1();

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

    return;
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
        return;
    }

    sem_wait(s_hydroQueue);

    barrierWait1();
    
    myPrint("H %d: creating molecule %d\n", idH, (*moleculeCounter));

    sem_wait(s_moleculeCreated);

    myPrint("H %d: molecule %d created\n", idH, (*moleculeCounter));

    barrierWait2();

    return;
}

// Main function
int main(int argc, char const *argv[])
{
    // Destroy shared memory preventively
    destroySemaphores();
    // destroySharedMemory();

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
    
    destroySemaphores();
    destroySharedMemory();

    fclose(fp);    
    return 0;
}
