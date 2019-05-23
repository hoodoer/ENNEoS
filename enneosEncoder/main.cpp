// This proof of concept program is to explore the use of
// neuroevolution as a method for encoding shellcode.
// This program is the encoder, which uses genetic algorithms
// to evolve a neural network that will output the desired
// shellcode at runtime.
//
// Lots of code borrowed from:
// https://github.com/hoodoer/NEAT_Thesis
//
// Drew Kirkpatrick
// @hoodoer
// drew.kirkpatrick@gmail.com
//
// ENNEoS (Evolutionary Neural Network Encoder of Shellcode/Shenanigans)
//
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <fstream>
#include <signal.h>
#include <iomanip>

#include "time.h"
#include "parameters.h"
#include "shellybot.h"
#include "Cga.h"
#include "sharedmemory.h"
#include "timer.h"


// This is the shellcode to learn
#include "encodedOutput.h"

// Hmm, need my second shellcode


using namespace std;


/*******************************************************************************
  General global types and variables. Yes, global. I feel no shame. Come at me.
********************************************************************************/

enum FitnessFunctionType{SINGLE, DOUBLE};

FitnessFunctionType RunType = DOUBLE;


bool DONE                      = false;
bool THREAD_READY              = false;
bool EXECUTION_THREADS_ONLINE  = false;

unsigned int generationCounter     = 1;
unsigned int chunkCounter          = 0;

const int nanoSleepAmount      = 100; // millionth of a second


string passphrase;

// Number of inputs and outputs to neural network
unsigned int numInputs;
unsigned int numOutputs;



// As the thunker threads stimulate the
// neural network, they'll record in this
// data structure whether they used the real
// passphrase or not, and what the shellcode
// output was. This data structure can be passed
// to a fitness function for scoring.
struct BrainTestData
{
    vector <bool> usedRealPassphrase;
    vector < vector<unsigned char> > brainOutputs;
    vector < vector<double> > rawBrainOutputs;
};


// Structure to hold data on individual
// neural networks used to break up
// the shellcode. You can't evolve once
// neural network to output the whole shellcode,
// but you can evolve a neural network to output
// a few characters. Break up the shellcode into
// pieces, and solve for a few characters at a time
// see param_shellcodeChunkSize
struct ShellcodePieceData
{
    int outputSize; // might be smaller than param_shellcodeChunkSize at the end of the shellcode
    vector <double> desiredNumericOutput; // what the neural network outputs
    vector <unsigned char> desiredBinaryOutput; // the actual hex chars we'll need

    // We can save the DNA that solves this
    // chunk of the shellcode here
    CGenome solutionGenome;
};




// Vector of the bots that have the brains
vector <ShellyBot*> shenaniganBots;


// Vector of bits of the shellcode, making
// bit size chunks for a neural network to tackle
vector <ShellcodePieceData> shellcodePieces;




// This is going to have to be rewritten to handle multiple
// shellcodes. Number of chunks will need to match the
// largest shellcode. At somepoint we'll probably want
// some buffer space so it's not so easy to determine
// the maximum size of all contain shellcodes? This
// would slow down encoding, but give possible doubt
// to a reverse engineer wondering if they've found
// all the shellcodes. Is there value there?
void chunkShellCode()
{
    int numChunks = __EncodedBinaryData_len / param_shellcodeChunkSize;
    cout<<"$$$$ Number of shellcode chunks is: "<<numChunks<<endl;
    int leftovers = __EncodedBinaryData_len % param_shellcodeChunkSize;
    cout<<"$$$$ Leftovers calc: "<<leftovers<<endl<<endl;;

    for (int i = 0; i < numChunks; i++)
    {
        ShellcodePieceData shellcodeChunk;

        shellcodeChunk.outputSize = param_shellcodeChunkSize;

        for (int j = 0; j < param_shellcodeChunkSize; j++)
        {
            int index = (i * param_shellcodeChunkSize) + j;
            //    cout<<"chunky index: "<<index<<endl;
            shellcodeChunk.desiredBinaryOutput.push_back(__EncodedBinaryData[index]);
            shellcodeChunk.desiredNumericOutput.push_back((double)__EncodedBinaryData[index]/255.0);
        }

        shellcodePieces.push_back(shellcodeChunk);
    }

    if (leftovers)
    {
        ShellcodePieceData shellcodeChunk;
        shellcodeChunk.outputSize = leftovers;

        for (int i = 0; i < leftovers; i++)
        {
            int index = (numChunks * param_shellcodeChunkSize) + i;
            //     cout<<"chunky index: "<<index<<endl;
            shellcodeChunk.desiredBinaryOutput.push_back(__EncodedBinaryData[index]);
            shellcodeChunk.desiredNumericOutput.push_back((double)__EncodedBinaryData[index]/255.0);
        }

        shellcodePieces.push_back(shellcodeChunk);
    }

    cout<<"---- Shellcode split into "<<shellcodePieces.size()<<" chunks"<<endl;
    //   exit(1);
}



// Useful for generating random strings to use
// as input during testing
typedef unsigned int uint;

string generateRandomString(uint maxOutputLength = 15, string allowedChars = "abcdefghijklmnaoqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890")
{
    //    uint outputLength = rand() % maxOutputLength + 1;
    uint outputLength = numInputs;

    int randomIndex;
    string outputString = "";

    for (uint i = 0; i < outputLength; ++i)
    {
        randomIndex = rand() % allowedChars.length();
        outputString += allowedChars[randomIndex];
    }

    if (outputString.empty())
    {
        return generateRandomString(maxOutputLength, allowedChars);
    }
    else
    {
        return outputString;
    }
}





// This function pushes all of the planned standard evaluations into a
// queue to be pulled by the various execution threads
// to execute
void setupEvaluationJobs()
{
    // Now setup the regular evaluation queue stuff...
    for (unsigned int i = 0; i < param_numAgents; i++)
    {
        // An evaluation job is simply a
        // reference to a bot's index
        // number. The individual thunking
        // threads will peel a few off
        // at a time to crunch on
        puppetMasterAddJobToQueue(i);
    }

    // Ok, this chunk job queue is for our new
    // threading approach
    for (unsigned int i = 0; i < shellcodePieces.size(); i++)
    {
        // Queue the indices of the different chunks
        // And the chunk crunchers will work on them
        puppetMasterAddChunkToQueue(i);
    }
}





// Take the neural network hex code output and
// "grade" it. This grade will determine the
// neural network's fitness going into the next
// evolutionary epoch

double lowestDiff      = 300.0;
int    lowestCharDiff  = 2000;
int    maxMatchedChars = 0;

double singleShellcodeFitnessFunction(BrainTestData &brainData)
{
    bool useRawOutput = true;

    // Calculate off the raw double output
    // of the neural network
    if (useRawOutput)
    {
        double brainScore = 0.0;
        double floatDiff;
        double diff;
        double bonus = 0.0;
        static int bonusAmount = 1000;

        int charTest = 0;


        for (unsigned int i = 0; i < brainData.usedRealPassphrase.size(); i++)
        {
            floatDiff = 0.0;

            // The inner loop is for going through the individual
            // hex characters
            for (unsigned int j = 0; j < numOutputs; j++)
            {
                // Get an integer that represents how "far" apart the actual shellcode
                // character and the neural network output character is. 0 to 255.
                diff = fabs(shellcodePieces.at(chunkCounter).desiredNumericOutput.at(j) - brainData.rawBrainOutputs.at(i).at(j));
                floatDiff += diff;
                // cout<<"$$$ Looking for double: "<<(double)__EncodedBinaryData[j]/255.0<<", got double: "<<brainData.rawBrainOutputs.at(i).at(j)<<endl;

                // Gotta a character perfectly, give 'em a bonus!
                if (diff < 0.003)
                {
                    bonus += (double)bonusAmount;
                }


                // Test for solution (char)
                charTest += (double)abs((int)shellcodePieces.at(chunkCounter).desiredBinaryOutput.at(j) - (int)brainData.brainOutputs.at(i).at(j));
            }

            if (charTest < lowestCharDiff)
            {
                cout<<"!!! New lowest Char Diff found: "<<charTest<<endl;
                lowestCharDiff = charTest;
            }


            // We need to convert brainScore to an average
            // across the outputs. Average char difference
            //  cout<<"^^^^ charDiff before mods: "<<charDiff<<endl;
            floatDiff = floatDiff/(double)numOutputs;
            //   cout<<"^^^^ charDiff after mods: "<<charDiff<<endl;


            // Debugging
            if (floatDiff < lowestDiff)
            {
                lowestDiff = floatDiff;
                cout<<"!!! New lowest shellcode diff average found: "<<lowestDiff<<endl;
            }


            if (charTest == 0)
            {
                cout<<"!!!!! Solution Found!"<<endl;
                return -1.0;
            }


            // If we stimulated the neural net with
            // the real passphrase, we want to reward
            // smaller differences from the target shellcode
            if (brainData.usedRealPassphrase.at(i))
            {
                brainScore += fabs(floatDiff - 2.0);
            }
            else
            {
                // We didn't pass in the real passphrase, we don't want the real shellcode spit out
                // Maybe something simple, like the average difference is at least some set amount
                // Doesn't have to be perfectly different, just not the actual shellcode
                if (floatDiff >= 80.0)
                {
                    // If the average character difference is 80 (out of 255)
                    // I'll call that different enough from the actual shellcode
                    // and give it a bonus.
                    //  brainScore += 200.0;
                    //    cout<<"    Diff is enough, BONUS time! diff: "<<charDiff<<endl;

                }
                else
                {
                    //    cout<<"    Diff didn't meet minimum, skipping bonus, diff: "<<charDiff<<endl;
                }
            }
            //        cout<<endl;
        }

        // Bonuses are good to inflate the score of better performing neural networks
        brainScore += bonus;

        // Square it to give it a nice ramp
        brainScore = brainScore * brainScore;

        return brainScore;
    }
    else // calculat off the char output, less nuance
    {
        // Check out the grace of this error handling
        if (brainData.usedRealPassphrase.size() != brainData.brainOutputs.size())
        {
            cout<<"Error in CalculateScore, vector sizes don't match. Go home."<<endl;
            exit(1);
        }

        double brainScore = 0.0;
        double charDiff;
        double diff;
        double bonus = 0.0;



        // Loop through all the
        // times the neural network was stimulated.
        // The scoring will be different depending on whether the
        // neural network was stimulated with the real passphrase
        // or a random string. Real passphrase we want shellcode,
        // random string we want something that isn't the shellcode.
        for (unsigned int i = 0; i < brainData.usedRealPassphrase.size(); i++)
        {
            charDiff = 0.0;


            // The inner loop is for going through the individual
            // hex characters
            for (unsigned int j = 0; j < numOutputs; j++)
            {
                // Get an integer that represents how "far" apart the actual shellcode
                // character and the neural network output character is. 0 to 255.
                diff = (double)abs((int)__EncodedBinaryData[j] - (int)brainData.brainOutputs.at(i).at(j));
                charDiff += diff;

                // Gotta a character perfectly, give 'em a bonus!
                if (diff == 0.0)
                {
                    bonus += 1000.0;
                }
            }

            // We need to convert brainScore to an average
            // across the outputs. Average char difference

            charDiff = charDiff/(double)numOutputs;
            //  cout<<"^^^^ charDiff after mods: "<<charDiff<<endl;


            // Debugging
            if (charDiff < lowestDiff)
            {
                lowestDiff = charDiff;
                cout<<"!!! New lowest shellcode diff average found: "<<lowestDiff<<endl;
            }

            // If we stimulated the neural net with
            // the real passphrase, we want to reward
            // smaller differences from the target shellcode
            if (brainData.usedRealPassphrase.at(i))
            {
                //   cout<<"++ Real passphrase, unmod score: "<<charDiff<<endl;
                brainScore += fabs(charDiff - 255.0);
                //   cout<<"    Adjusted for real passphrase: "<<brainScore<<endl;
            }
            else
            {
                //   cout<<"-- !Passphrase used"<<endl;
                // We didn't pass in the real passphrase, we don't want the real shellcode spit out
                // Maybe something simple, like the average difference is at least some set amount
                // Doesn't have to be perfectly different, just not the actual shellcode
                if (charDiff >= 80.0)
                {
                    // If the average character difference is 80 (out of 255)
                    // I'll call that different enough from the actual shellcode
                    // and give it a bonus.
                    //  brainScore += 200.0;
                    //    cout<<"    Diff is enough, BONUS time! diff: "<<charDiff<<endl;

                }
                else
                {
                    //    cout<<"    Diff didn't meet minimum, skipping bonus, diff: "<<charDiff<<endl;
                }
            }
            //        cout<<endl;
        }
        //    cout<<"Adding a bonus of: "<<bonus<<endl;
        //    cout<<"At end of calcScore, brainScore total is: "<<brainScore<<endl;
        brainScore += bonus;
        //    cout<<"After bonus: "<<brainScore<<endl;
        // Square it to give it a nice ramp
        brainScore = brainScore * brainScore;
        //    cout<<std::fixed<<"Final score: "<<brainScore<<endl;
        //    cout<<endl;

        return brainScore;
    }
}






// Take the neural network hex code output and
// "grade" it. This grade will determine the
// neural network's fitness going into the next
// evolutionary epoch
// This fitness function is the proof-of-concept
// for "double stuffing" a neural network with
// two different shellcodes. One nopsled, one sneaky
// something else
double doubleShellcodeFitnessFunction(BrainTestData &brainData)
{
    double brainScore = 0.0;
    double floatDiff;
    double diff;
    double bonus = 0.0;
    double desiredValue;
    
    static int    bonusAmount = 1000;
    static double nopValue    = 144.0;
    static double nopRawValue = nopValue/255.0;


    // Hmm, in this new approach, we need a cumulative char test for
    // whether a solution is found. It doesn't work for a single pass through
    // the loops, the network has to pass perfectly on all loops.
    // Ok, that should be fixed. But now I need a good metric to
    // monitor if actual progress is being made, short of an
    // actual solution
    int charTest = 0;
    
    // If in any loop we don't solve correctly, we flip this to false.
    bool solutionFound = true;

    for (unsigned int i = 0; i < brainData.usedRealPassphrase.size(); i++)
    {
       // cout<<"-- DoubleFitFunc score loop pass: "<<i<<endl;
        floatDiff = 0.0;
       //charTest  = 0;

        // The inner loop is for going through the individual
        // hex characters
        for (unsigned int j = 0; j < numOutputs; j++)
        {
            // Get an integer that represents how "far" apart the actual shellcode
            // character and the neural network output character is. 0 to 255.
            // If the stimulus was the real passphrase, we want the desired output to be shellcode
            // If the stimulas wasn't the passphrase, we want something else (e.g. nopsled)

            if (brainData.usedRealPassphrase.at(i))
            {
                desiredValue = shellcodePieces.at(chunkCounter).desiredNumericOutput.at(j);
              //  cout<<"  Processing REAL passphrase input..."<<endl;

            }
            else
            {
                // Not the real password, it should be a nopsled idealy
                // 144 == 0x90 == nop
                // Remember that the brain is outputting values in the 0.0->1.0 range
                // so convert your character value to this expectation (i.e. 144.0/255.0)
          //      cout<<"  Processing bullshit passphrase input..."<<endl;
                desiredValue = nopRawValue;
            }
           // cout<<endl;
            
          //  cout<<std::fixed<<"  Desired brain output: "<<desiredValue<<endl;
           // cout<<std::fixed<<"  Actual brain output: "<<brainData.rawBrainOutputs.at(i).at(j)<<endl;

            diff = fabs(desiredValue - brainData.rawBrainOutputs.at(i).at(j));
            floatDiff += diff;
//            cout<<"$$$ Looking for double: "<<desiredValue<<", got double: "<<brainData.rawBrainOutputs.at(i).at(j)<<endl;

            // Gotta a character perfectly, give 'em a bonus!
            if (diff < 0.003)
            {
                bonus += (double)bonusAmount;
            }


            // Test for solution (char)
            //cout<<"-- CharTest component desiredValue: "<<(int)(desiredValue * 255.0)<<endl;
            //cout<<"-- CharTest component output: "<<(int)brainData.brainOutputs.at(i).at(j)<<endl;
            
            charTest += abs((int)(desiredValue * 255.0) - (int)brainData.brainOutputs.at(i).at(j));
           // cout<<"CharTest is: "<<charTest<<endl;

        }



        // We need to convert brainScore to an average
        // across the outputs. Average char difference
        //  cout<<"^^^^ charDiff before mods: "<<charDiff<<endl;
        floatDiff = floatDiff/(double)numOutputs;
     //   cout<<std::fixed<<"  Float Diff Value: "<<floatDiff<<endl;
     //   cout<<"- Starting loop brainScore: "<<brainScore<<endl;
        
       // cout<<"- LoopScore: "<<fabs(1.0-floatDiff)<<endl;
        brainScore += fabs(1.0-floatDiff);
        
      //  cout<<"- Ending loop brainScore: "<<brainScore<<endl;

   //     cout<<endl<<endl;


        // Debugging
        if (floatDiff < lowestDiff)
        {
            lowestDiff = floatDiff;
            cout<<std::fixed<<"!!! New lowest shellcode diff average found: "<<lowestDiff<<endl;
        }

        if (charTest != 0)
        {
          solutionFound = false;
        }
    }
    
    if (charTest < lowestCharDiff)
    {
      //cout<<"!!! New lowest Char Diff found: "<<charTest<<endl;
      //cout<<"-------      charTest: "<<charTest<<endl;
      //cout<<"-------lowestCharDiff: "<<lowestCharDiff<<endl;
      lowestCharDiff = charTest;
    }

    
    // Test if this neural network
    // passed all checks...
    if (solutionFound)
    {
      cout<<"!!!! BOOM - Solution Found. Have a scone and a happy dance."<<endl;
     // exit(1);
      return -1.0;
    }

    // Bonuses are good to inflate the score of better performing neural networks
    brainScore += bonus;

    // Square it to give it a nice ramp
    brainScore = brainScore * brainScore;
    
    //cout<<"End of doublay fitness function, brainScore is: "<<brainScore<<endl;
    //exit(1);

    return brainScore;
}





// New and improved. Or, new and busted. But I'm going to get there.
// New multi-threading model will take advantage of the shellcode chunking.
// This'll be much more efficient at utilizing resources, and scale out the
// wazoo.
// Chunker crunch'un thread
void *runChunkCrunchingThread(void *iValue)
{
    struct timespec sleepTime;
    sleepTime.tv_sec  = 0;
    sleepTime.tv_nsec = nanoSleepAmount;

    // Let's get my unique thread index number
    int threadNumber = *((int *)iValue);

    // Setting this to true will allow the
    // next thread to startup and get its
    // index number
    THREAD_READY = true;


    // We need bot wrappers for our
    // neural networks
    vector <ShellyBot*> chunkyShenaniganBots;


    // Each chunk crunching thread gets it's own
    // genetics lab for evolution.
    Cga madScienceLab(param_numAgents, numInputs, numOutputs);



    // We can't have a thread per chunk, we have too many chunks
    // Scale the threads to the hardware, then queue chunks
    queue <unsigned int> myChunkQueue;


    // The chunk this thread is responsible for solving
    unsigned int chunkIndex;



    // We'll need to wait for shared
    // memory to be initialized here
    // eventually
    while (!sharedMemoryReady)
    {
        nanosleep(&sleepTime, NULL);
    }

    cout<<"Chunker-thunker numero "<<threadNumber<<" is pumped and eager to contribute to rube goldberish shenanigans"<<endl;

    // DONE is a global. Deal with it.
    while (!DONE)
    {
        // Check the local job queue, if it's empty
        // go grab some more from the global chunk
        // job queue.
        while (myChunkQueue.empty())
        {
            for (unsigned int i = 0; i < param_numJobsToLoad; i++)
            {
                // I don't have any more chunk jobs on my
                // to-do list, get some more
                if (chunkCruncherThreadGetJobToDo(chunkIndex))
                {
                    myChunkQueue.push(chunkIndex);
                }
                else
                {
                    // Global job queue is empty,
                    // run away, run away
                    pthread_exit(NULL);
                  //  break;
                }

                nanosleep(&sleepTime, NULL);
            }
        }


        // Ok, we have our shellcode chunks, let's get to it.
        if (!myChunkQueue.empty())
        {
            chunkIndex = myChunkQueue.front();
            myChunkQueue.pop();


            // ok, now we need a whole evolutionary system
            // right about.... here.

        }

        nanosleep(&sleepTime, NULL);
    }
}




// Number crunch'un thread
void *runThunkingThread(void *iValue)
{
    struct timespec sleepTime;
    sleepTime.tv_sec  = 0;
    sleepTime.tv_nsec = nanoSleepAmount;

    // Let's get my unique thread index number
    int threadNumber = *((int *)iValue);

    // Setting this to true will allow the
    // next thread to startup and get its
    // index number
    THREAD_READY = true;

    // Queue of bot index's to evaluate
    queue <unsigned int> myGenerationJobQueue;

    unsigned int jobIndex;

    // We'll need to wait for shared
    // memory to be initialized here
    // eventually
    while (!sharedMemoryReady)
    {
        nanosleep(&sleepTime, NULL);
    }

    cout<<"Thunker numero "<<threadNumber<<" is pumped and eager to contribute to rube goldberish shenanigans"<<endl;

    // DONE is a global. Deal with it.
    while (!DONE)
    {
        // Check local shenanigan job queue, if empty grab some
        // from the global job queue. In this version of multi-threading
        // the "job" is the index of a bot to crunch.
        while (myGenerationJobQueue.empty())
        {
            for (int i = 0; i < param_numJobsToLoad; i++)
            {
                // I don't have any more jobs on my
                // to-do list, get some more
                if (thunkerThreadGetJobToDo(jobIndex))
                {
                    myGenerationJobQueue.push(jobIndex);
                }
                else
                {
                    // Global job queue is empty,
                    // stop trying to load more jobs to do
                    break;
                }

                nanosleep(&sleepTime, NULL);
            }
        }


        // Ok, we should have some jobs to do now.
        //  cout<<"--Thunker "<<threadNumber<<" has jobs: "<<myGenerationJobQueue.size()<<endl;


        // Work the first job in my local queue
        if (!myGenerationJobQueue.empty())
        {
            jobIndex = myGenerationJobQueue.front();
            myGenerationJobQueue.pop();

            bool   passphraseUsed = false;
            bool   thisLoopRealPassphrase;

            // The bot we're testing and the i/o
            ShellyBot             *testingBot;
            string                inputToUse;
            vector<unsigned char> brainOutputs;
            vector<double>        rawBrainOutputs;
            BrainTestData         testData;



            // Setup our bot pointer for the assessment
            testingBot          = shenaniganBots.at(jobIndex);

            // The testingBot score results
            // The score is added later down after the "assessment"
            botEvaluationData jobResults;
            jobResults.botIndex = jobIndex;


            int testingLoops;

            // How many times we "test" the neural
            // network depends heavily on what kinda proof of concept
            // we're doing. Single shellcode and don't care about passphrase?
            // Two shellcodes and you do?
            // This is a bit messy
            switch (RunType)
            {
            case SINGLE:
                testingLoops = 1;
                break;

            case DOUBLE:
            //    testingLoops = 20;
                testingLoops = 5;
                break;

            default:
                testingLoops = 1;
                break;
            }

            // TESTING TIME!
            // We can use the testingLoops
            // to control the number of stimulus we
            // throw at the neural net in the bot.
            for (int i = 0; i < testingLoops; i++)
            {
                // !! Short circuiting this to just the correct
                // passphrase. Really just want to see accurate shellcode
                // pop out as a proof of concept
                //!!!!!!!!!!!!!!!!!!!!!!!!

                // If we're to the last input, and we
                // haven't used the passphrase yet, we gotta
                // use it now
                thisLoopRealPassphrase = false;
                
                //cout<<"$ Testing Loop: "<<i<<endl;
                
                if (passphraseUsed)
                {
                    inputToUse = generateRandomString(passphrase.length());
                    //cout<<"** Defaulting to leftover random phrases..."<<endl;
                }
                else // we haven't used the real trigger yet
                {
                    // If this is our last input, we
                    // need to use the real one
                    if (i == testingLoops-1)
                    {
                        inputToUse            = passphrase;
                        passphraseUsed         = true;
                        thisLoopRealPassphrase = true;
                        //cout<<"** Sneaking in the last possible true passphrase"<<endl;
                    }
                    else if (RandBool()) // randomly pick if we use it
                    {
                        inputToUse             = passphrase;
                        passphraseUsed         = true;
                        thisLoopRealPassphrase = true;
                        //cout<<"** Randomly selected TRUE passphrase"<<endl;
                    }
                    else
                    {
                        inputToUse = generateRandomString(passphrase.length());
                        //cout<<"** Randomly chose fake passphrase"<<endl;
                    }
                }

                // We need to record result data to pass
                // to the fitness function
                if (thisLoopRealPassphrase)
                {
                    testData.usedRealPassphrase.push_back(true);
                    //cout<<"-- Using real passphrase..."<<endl;
                }
                else
                {
                    testData.usedRealPassphrase.push_back(false);
                    //cout<<"-- Rendom passphrase..."<<endl;
                }


                // Let's push the input into the brain and see what pops out
                testingBot->setBrainInputs(inputToUse);
                testingBot->update();
                testingBot->getBrainCharOutputs(brainOutputs);
                testingBot->getBrainRawOutputs(rawBrainOutputs);

                // Take the brain output and push it onto our testData
                // so it can be scored later
                testData.brainOutputs.push_back(brainOutputs);
                testData.rawBrainOutputs.push_back(rawBrainOutputs);



                // Play nice
               // nanosleep(&sleepTime, NULL);
            }


            // Ok, now we need to pass the outputs to a func that will
            // calculate it's difference from the target if the passphrase is correct
            // and make sure it's just off for  na incorrect passphrase?
            // Sum those two to make the total score?
            switch (RunType)
            {
            case SINGLE:
                jobResults.score = singleShellcodeFitnessFunction(testData);
                break;

            case DOUBLE:
                jobResults.score = doubleShellcodeFitnessFunction(testData);
                break;

            default:
                cout<<"Something is amiss here. What did you screw up with RunType?"<<endl;
                exit(1);
            }

            jobResults.genomeID      = testingBot->getBrainGenomeID();
            jobResults.chunkSolution = false;
            // cout<<std::fixed<<"@@ Brain score: "<<jobResults.score<<endl;

            // This bot solved this shellcode chunk, let's save the genome
            // and move onto the next chunk
            if (jobResults.score == -1.0)
            {
                jobResults.chunkSolution = true;
            }


            // Push results back to puppetmaster for doing that evolutionary thing
            thunkerThreadAddEvalResult(jobResults);
        }

        nanosleep(&sleepTime, NULL);
    }
}






// This process handles the job distribution to the
// thunking threads, and handles calling the relevant
// genetic algorithm stuff at the end of a generation.
void runPuppetMaster()
{
    static struct timespec sleepTime;
    sleepTime.tv_sec  = 0;
    sleepTime.tv_nsec = nanoSleepAmount;

    bool   generationEvalsComplete;
    double highScore;
    double alltimeHighScore     = 0.0;
    int    gensSinceImprovement = 0;

    Timer generationTimer;
    Timer chunkTimer;
    Timer totalTimeAllGenerations;

    float generationTime = 0.0;
    float chunkTime      = 0.0;
    float totalRunTime   = 0.0;

    bool chunkSolved;

    // This will be used for fiddling with speciation
    const double compatibilityThresholdAdjustment = 0.1;

    // This is for saving the metadata file for the
    // saved chunk genomes
    ofstream genomeMetadataStream;
    string   genomeMetadataFilename;
    bool     metaDataFileOpen = false;


    // Seed the psuedo random function
    srand((unsigned)time(NULL));


    // Loop through all the chunks we need to solve for
    for (chunkCounter = 0; chunkCounter < shellcodePieces.size(); chunkCounter++)
    {
        cout<<"Puppet master starting evolution for chunk number: "<<chunkCounter<<endl;
        chunkSolved          = false;
        highScore            = 0;
        alltimeHighScore     = 0.0;
        gensSinceImprovement = 0;
        generationTime       = 0.0;
        chunkTime            = 0.0;
        generationCounter    = 1;

        lowestDiff           = 300.0;
        lowestCharDiff       = 2000;
        maxMatchedChars      = 0;


        chunkTimer.reset();

        shenaniganBots.clear();

        // Do the normal generation stuff below


        // Time for some fun stuff...
        // We need some ShellBrain bots. The bots will get
        // brains from the Cga type, the madScienceLab
        // below
        for (int i = 0; i < param_numAgents; i++)
        {
            shenaniganBots.push_back(new ShellyBot());
        }

        numOutputs = shellcodePieces.at(chunkCounter).outputSize;

        // We need a genetics lab. The CGA class handles most
        // everything for us. Mostly.
        Cga madScienceLab(param_numAgents, numInputs, numOutputs);



        // This runs a smoke test of the genetic algorithm
        // It evolves a Xor solution, without the benefit
        // of using short term memory (recurrent neural network connections)
        // You should run this to make sure it can solve the Xor problem with
        // you're evolutionary parameters. It simply runs the test and exits the program
        // madScienceLab.RunXorTestAndExit();


        // Need a place to stash the brains
        vector<CNeuralNet*> botBrainVector;


        // Kindly ask the madScienceLab to give us the brains (i.e. phenotypes)
        botBrainVector = madScienceLab.CreatePhenotypes();


        // Let's install those shiny new brains into our ShellyBots
        for (int i = 0; i < param_numAgents; i++)
        {
            shenaniganBots[i]->insertNewBrain(botBrainVector[i]);
        }



        // Gonna need a max generation cutoff
        // Should it be based on generations?
        // Or overall time?
        // Can we calculate a % progress based
        // on how _close_ we are to the original
        // shellcode?
        // We should just run it until
        // a solution is found, forget max generations
        while (!DONE && !chunkSolved)
        {
            // Reset stuff for the generation
            puppetMasterClearAllQueues();
            generationTimer.reset();
            generationEvalsComplete = false;

            cout<<endl<<"**********************************************************************"<<endl;
            cout<<"Puppet Master shellcode chunk: ("<<chunkCounter+1<<" of "<<shellcodePieces.size()<<"), generation: "<<generationCounter<<endl;
            cout<<endl;

            /****************************************
            // Setup job queue
            ***************************************/
            setupEvaluationJobs();

            // Wait for the thunkers to be done


            while (!generationEvalsComplete)
            {
                // Ok, we need to monitor for job results
                // from our thunkers and process them

                // Check to see if we found a solution

                // Smite the current generation of bots for being a disappointment and sucking at shellcode

                // Epoch and evolve all the things.

                // Get new brains, rinse and repeat.



                // we have the right number of results...
                if (puppetMasterGetResultsQueueSize() == param_numAgents)
                {
                    // We have the results for the entire generation
                    // We need to:
                    // 1) Process the data
                    // 2) Perform genetic epoch
                    // 3) Setup the next generation

                    highScore = 0.0;

                    // This vector will contain the fitness
                    // scores of the bots. This is what
                    // defines "fittest" in the evolutionary
                    // sense.
                    // We need scores to be greater than zero,
                    // and zero is the lowest score. So we'll
                    // add 1.0 to the scores, and then square
                    // the results to give us our final fitness
                    // scores to feed into the genetic algorithm
                    vector <double> shellyBotFitnessScores(param_numAgents);

                    botEvaluationData evalData;


                    int numResultsToProcess = puppetMasterGetResultsQueueSize();
                    double averageScore     = 0.0;

                    for (int i = 0 ; i < numResultsToProcess; i++)
                    {
                        if (!puppetMasterGetEvalResults(evalData))
                        {
                            cout<<"Eval results queue is empty when it shouldn't be!"<<endl;
                            exit(1);
                        }

                        // Check if the results I'm processing is a
                        // bot that solved the shellcode chunk
                        // If so we can skip epoch, save the genome,
                        // and move onto the next chunk.
                        if (evalData.chunkSolution)
                        {
                            cout<<"Puppet Master is processing a shellcode chunk solution..."<<endl;

                            shellcodePieces.at(chunkCounter).solutionGenome = madScienceLab.GetSingleGenome(evalData.botIndex);

                            if (shellcodePieces.at(chunkCounter).solutionGenome.ID() != evalData.genomeID)
                            {
                                cout<<"Error, non-matching genomeIDs in puppet master processing of chunk solution."<<endl;
                                exit(1);
                            }



                            stringstream fileNameStream;
                            string       dnaFileName;
                            string       fullFileName;
                            string       outputDirectory = "./output/";


                            fileNameStream<<"shellcodeGenome_"<<chunkCounter;

                            dnaFileName = fileNameStream.str();
                            fullFileName = outputDirectory + dnaFileName;

                            if (!shellcodePieces.at(chunkCounter).solutionGenome.WriteGenomeToFile(fullFileName))
                            {
                                cout<<"Error saving chunk solution sdna to file!"<<endl;
                                exit(1);
                            }

                            // We're done, let's get out of here
                            // and start workign on the next chunk
                            numResultsToProcess     = 0;
                            generationEvalsComplete = true;
                            chunkSolved             = true;


                            if (!metaDataFileOpen)
                            {
                                genomeMetadataFilename = "./output/shellcodeChunkIndex.dat";
                                genomeMetadataStream.open(genomeMetadataFilename.c_str(), ios::out);

                                if (genomeMetadataStream.fail())
                                {
                                    cout<<"Failed to open genome metadata file: "<<genomeMetadataFilename<<endl;
                                    exit(1);
                                }

                                metaDataFileOpen = true;
                            }

                            genomeMetadataStream<<chunkCounter<<", "
                                               <<dnaFileName<<".dna, "
                                              <<evalData.genomeID<<", "
                                             <<shellcodePieces.at(chunkCounter).solutionGenome.ID()<<", numNeurons: "
                                            <<shellcodePieces.at(chunkCounter).solutionGenome.NumNeurons()
                                           <<endl;

                            continue;
                        }

                        shellyBotFitnessScores.erase(shellyBotFitnessScores.begin() + evalData.botIndex);
                        shellyBotFitnessScores.insert(shellyBotFitnessScores.begin() + evalData.botIndex, evalData.score);
                        averageScore += evalData.score;

                        if (evalData.score > highScore)
                        {
                            highScore = evalData.score;
                        }
                    }

                    averageScore = averageScore/(double)numResultsToProcess;


                    /*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
                                       OH BABY IT'S EPOCH TIME!
                      !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
                    // cout<<"----------EPOCH----------"<<endl;
                    madScienceLab.Epoch(shellyBotFitnessScores);


                    int    numLeaders = 5;
                    vector <CNeuralNet*> bestBrains = madScienceLab.GetBestPhenotypesFromLastGeneration(numLeaders);


                    madScienceLab.ClearBestPhenotypesFromLastGeneration(numLeaders);


                    // We're done here, moving on...
                    generationEvalsComplete = true;
                    generationCounter++;

                    if (highScore > alltimeHighScore)
                    {
                        alltimeHighScore = highScore;
                        gensSinceImprovement = 0;
                    }
                    else
                    {
                        gensSinceImprovement++;
                    }

                    puppetMasterClearResultsQueue();
                    generationTime = generationTimer.total();
                    chunkTime      = chunkTimer.total();
                    totalRunTime   = totalTimeAllGenerations.total();

                    cout<<"Lowest difference in characters (in steps): "<<lowestCharDiff<<endl;
                    cout<<"Generations since last breakthrough: "<<gensSinceImprovement<<endl;
                    cout<<endl;
                    cout<<"Generation completed in "<<generationTime<<" seconds."<<endl;
                    cout<<"         Chunk run time "<<chunkTime<<" seconds."<<endl;
                    cout<<"         Total run time "<<totalRunTime<<" seconds."<<endl;
                    //                cout<<" --> Percentage of solution found: "<<100.0*(highScore/65025.0)<<endl;


                    // Need speciation compatibility tweaking
                    // We use speciation to group and protect different subgroups
                    // in our population. We want to tweak the compatibility threshold
                    // in order to maintain a target number of species. Grouping of
                    // individuals in the population is based on how similar their
                    // genome is
                    double currentCompatThreshold;
                    int    currentNumSpecies;

                    currentCompatThreshold = madScienceLab.GetCurrentCompatibilityThreshold();
                    currentNumSpecies      = madScienceLab.NumSpecies();

                    if (currentNumSpecies < param_targetNumSpecies)
                    {
                        // Adjust for more species
                        madScienceLab.SetCompatibilityThreshold(currentCompatThreshold - compatibilityThresholdAdjustment);
                    }
                    else if (currentNumSpecies > param_targetNumSpecies)
                    {
                        // Adjust for fewer species
                        madScienceLab.SetCompatibilityThreshold(currentCompatThreshold + compatibilityThresholdAdjustment);
                    }
                    // Done adjusting species compatibility thresholds



                    // Need to grab new round of brains and insert
                    // into our little shellybots.
                    vector <CNeuralNet*> newBrains;

                    // Looks like I gutted Cga too much, I don't
                    // have a method anymore for getting brains.
                    // That's problematic.
                    newBrains = madScienceLab.CreatePhenotypes();

                    for (int i = 0; i < param_numAgents; i++)
                    {
                        shenaniganBots[i]->insertNewBrain(newBrains[i]);
                    }
                }

                nanosleep(&sleepTime, NULL);
            }
        }
    }

    if (metaDataFileOpen)
    {
        genomeMetadataStream.close();
        metaDataFileOpen = false;
    }
}









void printProgramUsage()
{
    cout<<"Call program with the passphrase to trigger the shellcode output:"<<endl;
    cout<<"Usage: enneosEncoder [PASSPHRASE]"<<endl;
}





// Parses command line arguments passed to
// the program on startup
void parseArguments(int argc, char *argv[])
{
    switch (argc)
    {
    case 2:
        passphrase = argv[1];
        cout<<"Using passphrase: "<<passphrase<<endl;
        break;
    default:
        printProgramUsage();
        exit(1);
    }
}



// Handle signals
void signalHandler(int signal)
{
    switch (signal)
    {
    case SIGINT: // Time to exit
        cout<<"Caught a SIGINT!"<<endl;
        DONE = true;
        exit(0);
        break;

    default:
        // Other signals...
        break;
    }
}




void calcInputOutputCount()
{
    numInputs = passphrase.size();
}







int main(int argc, char *argv[])
{
    cout<<"Starting ENNEoS encoder. Let's evolve some brains to hide shellcode. "<<endl;
    cout<<endl;


    static struct timespec sleepTime;

    sleepTime.tv_sec  = 0;
    sleepTime.tv_nsec = nanoSleepAmount;


    // Used to look for arguments at the command line
    parseArguments(argc, argv);

    chunkShellCode();

    // Sort out how many inputs/outputs our brains will need
    calcInputOutputCount();

    cout<<endl<<"*********************************"<<endl;

    // Setup our semaphore
    initPosixSemaphore();


    // Should catch some signals and stuff to
    // exit gracefully
    signal(SIGINT, signalHandler);


    // Main process will become the job handler,
    // split of arbitrary threads to handle
    // computation and evolution
    pthread_t executionThreads[param_numExecutionThreads];

    // Spawn the execution threads for crunch'un
    for (int i = 0; i < param_numExecutionThreads; i++)
    {
        THREAD_READY = false;
        // Note that this version of threading is really inefficient for shellcode
        // chunks. The refactoring needs to be finished, badly, when there's time.
        // Each thread should have its own genetical algorithm (CGA instance)
        // and the queue of work fed to threads should be individual chunks
        pthread_create(&executionThreads[i], NULL, runThunkingThread, (void *)&i);

        while (!THREAD_READY)
        {
            // We need to wait for the threads to grab
            // it's index number from the void* iterator pointer
            // before moving onto the next thread. This makes sure
            // every execution thread has the correct index number
            nanosleep(&sleepTime, NULL);
        }
    }

    cout<<"All thunkun threads online, waking up the puppet master..."<<endl;

    runPuppetMaster();


    // Need clean up codes
    closeSemaphore();


    return 0;
}
