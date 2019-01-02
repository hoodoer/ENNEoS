#ifndef CGA_H
#define	CGA_H

// Based on the code from the book AI Techniques for Game Programming
// by Mat Buckland. 
// Modified by Drew Kirkpatrick, drew.kirkpatrick@gmail.com

#include <vector>
#include <semaphore.h>

#include "phenotype.h"
#include "genotype.h"
#include "CSpecies.h"
#include "parameters.h"

using namespace std;



//------------------------------------------------------------------------
//
//  this structure is used in the creation of a network depth lookup
//  table.
//------------------------------------------------------------------------
struct SplitDepth
{
    double val;

    int    depth;

    SplitDepth(double v, int d):val(v), depth(d){}
};


//------------------------------------------------------------------------
// CGA class
//------------------------------------------------------------------------
class Cga
{

private:
    // current population
    vector<CGenome> m_vecGenomes;

    // keep a record of the last generations best genomes.
    vector<CGenome> m_vecBestGenomes;

    // keep a record of best performers for all
    // time. Best genome from each generation
    // is added to this vector. This can be
    // controlled manually if you want
    // by setting manualHallOfFameControl
    // to true.
    vector<CGenome> m_vecHallOfFameGenomes;

    // a record of the leader of each species
    vector<CGenome> m_vecSpeciesLeaderGenomes;


    // a record of genomes. Inclusion in
    // this record is controlled externally.
    // Handy for things like novelty archives
    // and such
    vector<CGenome> m_vecCustomArchiveGenomes;

    // all the species
    vector<CSpecies>  m_vecSpecies;

    // to keep track of innovations
    CInnovation*      m_pInnovation;

    // current generation
    int               m_iGeneration;
    int               m_iNextGenomeID;
    int               m_iNextSpeciesID;
    int               m_iPopSize;

    // adjusted fitness scores
    double            m_dTotFitAdj, m_dAvFitAdj;

    // index into the genomes for the fittest genome
    int               m_iFittestGenome;

    double            m_dBestEverFitness;


    // this holds the precalculated split depths. They are used
    // to calculate a neurons x/y position for rendering and also
    // for calculating the flush depth of the network when a
    // phenotype is working in 'snapshot' mode.
    vector<SplitDepth> vecSplits;

    // Temporary vector for Split Depth, used during
    // vector construction
    vector<SplitDepth> tempVecSplits;



    // used in Crossover
    void AddNeuronID(int nodeID, vector<int> &vec);


    // separates each individual into its respective species by calculating
    // a compatibility score with every other member of the population and
    // niching accordingly. The function then adjusts the fitness scores of
    // each individual by species age and by sharing and also determines
    // how many offspring each individual should spawn.
    void SpeciateAndCalculateSpawnLevels();


    // Checks the older species (how old set by a parameter) to see
    // if their performance is bad enough to justify not allowing
    // them to reproduce
    void TestOldSpeciesForKilling();


    // adjusts the fitness scores depending on the number sharing the
    // species and the age of the species.
    void AdjustSpeciesFitnesses();

    CGenome Crossover(CGenome& mum, CGenome& dad);

    CGenome TournamentSelection(const int NumComparisons);


    // searches the lookup table for the dSplitY value of each node in the
    // genome and returns the depth of the network based on this figure
    int CalculateNetDepth(const CGenome &gen);


    // sorts the population into descending fitness, keeps a record of the
    // best n genomes and updates any fitness statistics accordingly
    void SortAndRecord();

    // keeps a record of the n best genomes from the last population.
    // (used to display the best genomes)
    void StoreBestGenomes();


    // a recursive function used to calculate a lookup table of split
    // depths
    vector<SplitDepth> Split(double low, double high, int depth);


    // This value is used for the compatibility
    // threshold to determine speciation. This
    // value is set to param_compatibilityThreshold
    // on initialization, and can be left like that
    // if you wish to have a static threshold. You can
    // use the setCompatibilityThreshold call to
    // change this dynamically if you wish, which is
    // helpful if you want to target a set number of
    // species. See the notes in parameters.h for
    // tips on how to handle this.
    double  compatibilityThresholdToUse;


    // This value is set to true when running the
    // built in XOR test validation. It changes
    // how epoch is handled, so it knows to
    // ignore certain parameters that aren't
    // compatible with XOR testing
    bool    xorTestMode;


    // Simple bool for turning on
    // debug print statements
    bool    debugMode;


    // Determines which approach the Epoch
    // method uses to fill in missing genomes
    // in the population at the end of Epoch
    UnderFlowMode popSizeUnderFlowMode;

    // False by default, if set to
    // true, the Cga class will not
    // automatically enter the best
    // scoring genome per generation into
    // the hall of fame. You can add
    // genomes to the hall of fame
    // using whatever metric you want
    // when this is set to true.
    // See AddGenomeToHallOfFame function
    bool manualHallOfFameControl;





public:
    Cga(int  size,
        int  inputs,
        int  outputs);

    ~Cga();

    vector<CNeuralNet*> Epoch(const vector<double> &FitnessScores);


    // iterates through the population and creates the phenotypes
    vector<CNeuralNet*> CreatePhenotypes();

    // returns a vector of the n best phenotypes from the previous generation
    // This is just the neural nets of the last generation, sorted
    // by fitness score
    vector<CNeuralNet*> GetBestPhenotypesFromLastGeneration(int count);

    // Clear the phenotypes from best genomes of last generation
    void ClearBestPhenotypesFromLastGeneration(int count);

    // returns a vector of the hall of fame phenotypes
    vector<CNeuralNet*> GetHallOfFamePhenotypes();

    // returns a vector of the custom archive phenotypes;
    vector<CNeuralNet*> GetCustomArchivePhenotypes();

    // returns a vector of the champs from each
    // species
    vector<CNeuralNet*> GetLeaderPhenotypesFromEachSpecies();

    // Returns a pointer to the best scoring phenotype
    // from the last generation. Handy for data collection stuff
    // Not thread safe for actual usage since Cga doesn't do
    // duplication associated with this call. If you need duplicates,
    // use it through hall-of-famer duplication calls
    CNeuralNet* GetSingleBestPhenotypeFromLastGeneration();


    // Used gofr getting a specific genome from the genetic algorithm
    CGenome GetSingleGenome(int index);

    // Returns the number of phenotypes that are
    // tied with the best score
    int GetNumberOfGenotypesSharingTopFitness();


    int     NumSpecies()const{return m_vecSpecies.size();}
    double  BestEverFitness()const{return m_dBestEverFitness;}


    // Used for changing from a static compatibility threshold
    // to a dynamic one for targeting a specific number of
    // species in a population
    void    SetCompatibilityThreshold(double newThreshold);
    double  GetCurrentCompatibilityThreshold();

    int     GetNumHallOfFamers();
    int     GetNumCustomArchivers();

    void    AddGenomeToCustomArchive(int index, int genomeID);
    void    ClearCustomArchive();


    // Manual control of hall of fame inclusion
    // control functions. AddGentomeToHallOfFame
    // needs to be called BEFORE epoch.
    void    SetManualHallOfFameControl(bool newValue);
    void    AddGenomeToHallOfFame(int index, int genomeID);


    // For saving genome data to a file
    bool    WriteGenomeToFile(int genomeID, string fileName);


    // Test function. Not for general use, just for
    // debuggin purposes.
    void dumpHofGenomesToFile(string descriptor);


    // Runs a XOR test using the parameters in
    // parameters.h. Useful for seeing if the
    // parameters are valid. Program exits
    // after the validation test. It's a good
    // idea to run this after screwing around
    // with your parameters to see if they're
    // valid.
    void RunXorTestAndExit();

    // Change debug mode
    void setDebugMode(bool newDebugMode);
};


#endif
