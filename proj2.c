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
sem_t *s_writeFile;

sem_t *s_mutex;
sem_t *s_oxyQueue;
sem_t *s_hydroQueue;

sem_t *s_barrierMutex;
sem_t *s_barrierTurnstile;
sem_t *s_barrierTurnstile2;

// Shared memory pointers
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
    s_writeFile = sem_open("/xstrel03/s_writeFile", O_CREAT, 0666, 1);

    s_mutex = sem_open("/xstrel03/s_mutex", O_CREAT, 0666, 1);
    s_oxyQueue = sem_open("/xstrel03/s_oxyQueue", O_CREAT, 0666, 0);
    s_hydroQueue = sem_open("/xstrel03/s_hydroQueue", O_CREAT, 0666, 0);

    s_barrierMutex = sem_open("/xstrel03/s_barrierMutex", O_CREAT, 0666, 1);
    s_barrierTurnstile = sem_open("/xstrel03/s_barrierTurnstile", O_CREAT, 0666, 0);
    s_barrierTurnstile2 = sem_open("/xstrel03/s_barrierTurnstile2", O_CREAT, 0666, 1);
}

void initSharedMemory()
{
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
}

void destroySharedMemory()
{
    munmap(sequenceCounter, sizeof(int));
    munmap(oxygen, sizeof(int));
    munmap(hydrogen, sizeof(int));
    munmap(barrierCount, sizeof(int));
    munmap(moleculeCounter, sizeof(int));
}

int barrierWait(char element, int id)
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

    // Critical point
    sem_wait(s_writeFile);
    int molecule = (*moleculeCounter);
    fprintf(fp, "%d: %c %d: creating molecule %d\n", *sequenceCounter, element, id, molecule);
    fflush(fp);
    (*sequenceCounter)++;
    sem_post(s_writeFile);


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

    return molecule;
}

void oxygenProcess(int idO, int TI, int TB)
{
    srand(time(NULL) * getpid());

    // Started
    sem_wait(s_writeFile);
    fprintf(fp, "%d: O %d: started\n", *sequenceCounter, idO);
    fflush(fp);
    (*sequenceCounter)++;
    sem_post(s_writeFile);

    // Sleep <0, TI>
    usleep(((rand() % (TI + 1)) * 1000));
    
    // Going to queue
    sem_wait(s_writeFile);
    fprintf(fp, "%d: O %d: going to queue\n", *sequenceCounter, idO);
    fflush(fp);
    (*sequenceCounter)++;
    sem_post(s_writeFile);

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

    sem_wait(s_oxyQueue);

    int molecule = barrierWait('O', idO);

    (*moleculeCounter)++;

    sem_post(s_mutex);


    // Sleep <0, TB>
    usleep(((rand() % (TB + 1)) * 1000));

    // Molecule created
    sem_wait(s_writeFile);
    fprintf(fp, "%d: O %d: molecule %d created\n", *sequenceCounter, idO, molecule);
    fflush(fp);
    (*sequenceCounter)++;
    sem_post(s_writeFile);

    exit(0);
}

void hydrogenProcess(int idH, int TI)
{
    srand(time(NULL) * getpid());
    
    // Started
    sem_wait(s_writeFile);
    fprintf(fp, "%d: H %d: started\n", *sequenceCounter, idH);
    fflush(fp);
    (*sequenceCounter)++;
    sem_post(s_writeFile);

    // Sleep <0, TI>
    usleep(((rand() % (TI + 1)) * 1000));
    
    // Going to queue
    sem_wait(s_writeFile);
    fprintf(fp, "%d: H %d: going to queue\n", *sequenceCounter, idH);
    fflush(fp);
    (*sequenceCounter)++;
    sem_post(s_writeFile);

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

    sem_wait(s_hydroQueue);
    
    // Molecule created

    int molecule = barrierWait('H', idH);

    exit(0);
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
    *oxygen = 0;
    *hydrogen = 0;
    *barrierCount = 0;
    *moleculeCounter = 1;

    // Create processes to create oxygen and hydrogen
    for (int i = 1; i <= NO; i++)
    {
        pid_t oxygenPid = fork();
        if (oxygenPid == 0) // Child process
        {
            oxygenProcess(i, TI, TB);
            exit(0);
        }
    }

    for (int i = 1; i <= NH; i++)
    {
        pid_t hydrogenPid = fork();
        if (hydrogenPid == 0) // Child process
        {
            hydrogenProcess(i, TI);
            exit(0);
        }
    }


    // Wait for processes to finish
    while(wait(NULL) > 0);

    destroySemaphores();
    destroySharedMemory();

    fclose(fp);    
    return 0;
}
