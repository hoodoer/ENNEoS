#ifndef PARAMETERS_H
#define PARAMETERS_H

// This file holds the various parameters
// used in the project. These will help
// to tweak things, and adapt the classes to
// different applications easier
// Drew Kirkpatrick, drew.kirkpatrick@gmail.com


/********************************************************************************
Parameter Notes:

For really large populations, you might want to increase
param_mMatched (C3 constant from NEAT paper). In the NEAT paper,
they discussed changing it from the default 0.4 to 3.0 when
they were doing a really difficult task, and they went from
populations of 150 to 1000. 
If you do increase param_mMatched, you'll need increase
param_compatibilityThreshold as well. 

Note, C1 is param_mExcess, and C2 is param_mDisjoint. These
are generally left at 1.0.

The param_compatibilityThreshold is used for static speciation
level comparisons. If you wish to use a dynamically changing threshold
to target a set number of species, you need to call
Cga::setCompatibilityThreshold to change the value used.

For example, if you have more than your target number of
species, you can increase the compatibility threshold. If
you have too few species, you can decrease the compatibility
threshold.


In larger populations, you might want to consider adding
higher chances of new nodes/links. 
param_chanceAddNode
param_chanceAddLink
This is because larger populations can tolerate
a larger number of prospective species, and 
greater topological diversity


Remember that because of speciation, the system
is tolerant to frequent mutations, so don't be
afraid to play around with mutation rates. 


If you haven't seen new species develop after 10 generations, you probably
need to change parameters


If your application requires the development of memory (short-term, long-term),
etc, and your neural network (or in the case of NEAT, Recurrent Neural Networks, RNN)
needs to keep track of something over time internally, you'll have to have 
recurrent links. Might want to consider upping the chances of adding recurrent
links
param_chanceAddRecurrentLink


As far as how long a generation "lives", you should probably choose between an
amount of time (param_generationTime), or a number of cycles (param_numTicks). 
Make sure you code up your software to process a generation for that many ticks,
or that length of time before moving on the the Epoch method of the genetic 
algorithm class (Cga class). 


For weight mutation rates, you probably want to set these pretty high (50% or more)
for control tasks, and lower rates (under 1%) for high input stuff like playing 
a game Othello. So if you have lots of inputs, perhaps use low mutation rates,
and if you have a small number of inputs, and are doing some kind of control 
task, use high mutation rates
param_mutationRate
param_activationMutationRate

If you have a small amount of hidden nodes relative to the number of input neurons,
you might want to increase the param_numAddLinkAttempts and
param_numTrysToFindLoopedLink (see AddLink in CGenome class)

If you want to test the validity of your NEAT parameters, look at the
RunXorTestAndExit function in the Cga class.
*********************************************************************************/


//------------------------------------------------------------------------
//
//  this enumeration is used to determine the method
//  the Epoch function uses to resolve population
//  size underflow due to either spawn level
//  rounding issues, or species being killed off
//------------------------------------------------------------------------
enum UnderFlowMode
{
    ORIGINAL_TOURNY_SELECT,
    HIGH_MUTATION_TOURNY_SELECT
};




// Num agents is used internally by the NEAT genetic
// algorithms. numAgents should reflect
// the number of agents managed by a single instance of Cga.
const int    param_numAgents                      = 3000;
//const int    param_numTicks                       = 1;
const int    param_maxNumGenerations              = 500;

// How small to break up the shellcode. It's too hard
// to evolve a single neural network to output the whole
// shellcode, do a few characters at a time and have an array
// of neural networks to divide and conquer
const int    param_shellcodeChunkSize             = 1;

const int    param_maxPermittedNeurons            = 3000;
const int    param_numTrysToFindOldLink           = 10;
const int    param_numTrysToFindLoopedLink        = 10;
const int    param_numAddLinkAttempts             = 10;

const int    param_numGensAllowedNoImprovement    = 35;
const int    param_killWorstSpeciesThisOld        = 60;
const int    param_youngBonusAgeThreshold         = 30;
const int    param_oldAgeThreshold                = 70;
const double param_youngFitnessBonus              = 1.3;
const double param_oldAgePenalty                  = 0.7;
const double param_survivalRate                   = 0.3;



// What method will the Cga use to solve underflow
// in population size?
const UnderFlowMode param_underFlowMode           = HIGH_MUTATION_TOURNY_SELECT;

// The mutation multiplier is used in the HIGH MUTATION
// under flow mode
// const double param_underFlowMutationMultiplier    = 1.5;
const double param_underFlowMutationMultiplier    = 2.0;


// Whether or not to track best ever fitness
// over all generations, or reset each generation (better for competitive coevolution)
const bool   param_resetBestEverFitnessEachGen    = true;


// Mutation stuff
const double param_crossoverRate                  = 0.7;

//const double param_chanceAddNode                  = 0.01; // From original experiment
//const double param_chanceAddLink                  = 0.1;  // From original experiment
//const double param_chanceAddNode                  = 0.025;  // From thesis experiment
//const double param_chanceAddLink                  = 0.25;   // From thesis experiment
// const double param_chanceAddNode                  = 0.5;
// const double param_chanceAddLink                  = 0.7;
const double param_chanceAddRecurrentLink         = 0.0005;
const double param_chanceAddNode                  = 0.05;
const double param_chanceAddLink                  = 0.4;
//const double param_chanceAddRecurrentLink         = 0.02;



//const double param_mutationRate                   = 0.8;   // From original experiment
//const double param_probabilityWeightReplaced      = 0.1;   // From original experiment
//const double param_maxWeightPerturbation          = 0.9;   // From original experiment
const double param_mutationRate                   = 1.2;
const double param_probabilityWeightReplaced      = 0.4;
const double param_maxWeightPerturbation          = 1.0;

//const double param_activationMutationRate         = 0.1; // From original experiment
//const double param_maxActivationPerturbation      = 0.1; // From original experiment
const double param_activationMutationRate         = 0.5;
const double param_maxActivationPerturbation      = 0.5;





// Compatibility calculation parameters
const double param_mExcess                        = 1.0; // C1
const double param_mDisjoint                      = 1.0; // C2
const double param_mMatched                       = 3.0; // C3

// Can be overridden using Cga::SetCompatibilityThreshold for dynamic use
const double param_compatibilityThreshold         = 3.0;

// If using dynamic compatibility thresholds,
// Cga will use these parameters as a minimum
// and maximum
const double param_minCompatibilityThreshold      =   0.1;
const double param_maxCompatibilityThreshold      = 500.0;

// If using dynamic compatibility thresholds, here's
// a convenient place to stash the target number
// of species. Dynamic changing of compatibility
// threshold is NOT handled by the Cga class,
// you need to handle that in your own code
const int    param_targetNumSpecies               = 100;







// Parallel processing parameters:
// Number of execution processes to 
// use for evaluations.
const int    param_numExecutionThreads            = 6;

// How many jobs the execution threads will snag
// at once from global job queue
const int    param_numJobsToLoad                  = 10;


// Parameters influencing XOR testing.
// See RunXorTestAndExit() function
// of Cga class.
const int    param_numXorTestRuns                 = 100;

#endif
