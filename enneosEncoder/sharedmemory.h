#ifndef SHAREDMEMORY_H
#define SHAREDMEMORY_H

#include <semaphore.h>
#include <queue>


// From main.cpp
extern bool DONE;


extern bool sharedMemoryReady;

void initPosixSemaphore();
void closeSemaphore();



struct botEvaluationData
{
    unsigned int botIndex;
    unsigned int genomeID;
    double       score;
    bool         chunkSolution;
};



// Thunker thread functions
bool thunkerThreadGetJobToDo(unsigned int &assignedJob);
void thunkerThreadAddEvalResult(botEvaluationData evalData);


// Puppet master functions
void puppetMasterAddJobToQueue(unsigned int newEvalJob);
int  puppetMasterGetResultsQueueSize();
bool puppetMasterGetEvalResults(botEvaluationData &evalData);
void puppetMasterClearAllQueues();
void puppetMasterClearResultsQueue();

// Generic functions
int getGlobalJobQueueSize();






#endif // SHAREDMEMORY_H
