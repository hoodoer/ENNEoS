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


// Chuck Cruncher thread funnctions
bool chunkCruncherThreadGetJobToDo(unsigned int &assignedChunk);
// Hmm, we don't need to report back the results, we can just
// save the genome directly from the chunk cruncher thread....
//void chunkCruncherThreadAddEvalResult(botEvaluationData evalData);


// Puppet master functions
void puppetMasterAddJobToQueue(unsigned int newEvalJob);
int  puppetMasterGetResultsQueueSize();
bool puppetMasterGetEvalResults(botEvaluationData &evalData);
void puppetMasterClearAllQueues();
void puppetMasterClearResultsQueue();

// New puppet master function for threading on
// chunk crunchers
void puppetMasterAddChunkToQueue(unsigned int newChunkEvalJob);


// Generic functions
int getGlobalJobQueueSize();






#endif // SHAREDMEMORY_H
