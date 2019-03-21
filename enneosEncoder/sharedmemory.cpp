#include <iostream>
#include "sharedmemory.h"


using namespace std;

/**********************************************************************
             Shared Memory specific functions and variables
                The various threads communicate
                through shared memory, so we need
                semaphores for controlling access
*********************************************************************/

// The queue of jobs the job manager
// cooks up. These jobs are sent to
// the execution threads
// In this queue, the jobs are
// an index of bots. Early version
// of multi-threading
queue <unsigned int> globalEvalJobQueue;


// The queu for the newer version of multi-threading
// takes into account the chunking of
// shellcode, which makes solutions easier
// and is better for multithreading.
// Each thread will have a unique
// genetic algorithm to solve for a specific
// chunk of the shellcode
queue <unsigned int> globalChunkJobQueue;



// Execution threads put the score results into this
// queue when they finish an evaluation
queue <botEvaluationData> globalEvalResultsQueue;


// Semaphore stuffs
sem_t processSemaphore;
bool  sharedMemoryReady = false;





// Lock the shared memory
inline void shmSemLock()
{
    if (sem_wait(&processSemaphore) != 0)
    {
        cout<<"Error locking semaphore"<<endl;
        DONE = true;
        exit(1);
    }
}


// Unlock the shared memory
inline void shmSemUnlock()
{
    if (sem_post(&processSemaphore) != 0)
    {
        cout<<"Error unlocking semaphore"<<endl;
        DONE = true;
        exit(1);
    }
}




// Initialize the semaphore to
// allow for save share memory
// access by the various threads
// This should only be called
// by the main func
void initPosixSemaphore()
{
    if (sem_init(&processSemaphore, 0, 1) != 0)
    {
        cout<<"Init'ing unamed semaphore failed!"<<endl;
        DONE = true;
        exit(1);
    }

    sharedMemoryReady = true;
}



// Called by main func to destroy the
// unamed semaphore
void closeSemaphore()
{
    sem_destroy(&processSemaphore);
}






////////////////////////////////////////////////////////////////

// Called by thunker thread to
// retrieve an eval job for it's job queue
// Returns false if the job queue is empty,
// and there are no more jobs to do
bool thunkerThreadGetJobToDo(unsigned int &assignedJob)
{
    bool returnValue;

    shmSemLock();
    {
        if (!globalEvalJobQueue.empty())
        {
            returnValue = true;
            assignedJob = globalEvalJobQueue.front();
            globalEvalJobQueue.pop();
        }
        else
        {
            returnValue = false;
        }
    }
    shmSemUnlock();

    return returnValue;
}




// Called by thunker thread to post up an assessment
// score for the puppetmaster to use for evolution
void thunkerThreadAddEvalResult(botEvaluationData evalData)
{
    shmSemLock();
    {
        globalEvalResultsQueue.push(evalData);
    }
    shmSemUnlock();
}










// Allows the puppet master to add another
// requested evaluation to the back of
// the job queue
void puppetMasterAddJobToQueue(unsigned int newEvalJob)
{
    shmSemLock();
    {
        globalEvalJobQueue.push(newEvalJob);
    }
    shmSemUnlock();
}



// How many results do I have to process?
int puppetMasterGetResultsQueueSize()
{
    int returnValue;

    shmSemLock();
    {
        returnValue = globalEvalResultsQueue.size();
    }
    shmSemUnlock();

    return returnValue;
}



// Gets a result to process. Returns false if there's
// nothing to process
bool puppetMasterGetEvalResults(botEvaluationData &evalData)
{
    bool returnValue;

    shmSemLock();
    {
        if (!globalEvalResultsQueue.empty())
        {
            returnValue = true;
            evalData = globalEvalResultsQueue.front();
            globalEvalResultsQueue.pop();
        }
        else
        {
            returnValue = false;
        }
    }
    shmSemUnlock();

    return returnValue;
}



// Clear out all the queues for a fresh start
void puppetMasterClearAllQueues()
{
    shmSemLock();
    {
        while (!globalEvalJobQueue.empty())
        {
            globalEvalJobQueue.pop();
            cout<<"!! Warning! Non-empty queues being cleared!"<<endl;
        }

        while (!globalEvalResultsQueue.empty())
        {
            globalEvalResultsQueue.pop();
            cout<<"!! Warning! Non-empty queues being cleared!"<<endl;
        }
    }
    shmSemUnlock();
}


// Just clear the results queue
void puppetMasterClearResultsQueue()
{
    shmSemLock();
    {
        while (!globalEvalResultsQueue.empty())
        {
            globalEvalResultsQueue.pop();
        }
    }
    shmSemUnlock();
}






// Get the size of the global job queue
int getGlobalJobQueueSize()
{
    int returnValue;
    shmSemLock();
    {
        returnValue = globalEvalJobQueue.size();
    }
    shmSemUnlock();

    return returnValue;
}
