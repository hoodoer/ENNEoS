#include <thread>

#include "Cga.h"
#include "timer.h"



// Based on the code from the book AI Techniques for Game Programming
// by Mat Buckland. 
// Modified by Drew Kirkpatrick, drew.kirkpatrick@gmail.com





//-------------------------------------------------------------------------
//	this constructor creates a base genome from supplied values and creates 
//	a population of 'size' similar (same topology, varying weights) genomes
//-------------------------------------------------------------------------
Cga::Cga(int  size,
         int  inputs,
         int  outputs): m_iPopSize(size),
    m_iGeneration(0),
    m_pInnovation(NULL),
    m_iNextGenomeID(0),
    m_iNextSpeciesID(0),
    m_iFittestGenome(0),
    m_dBestEverFitness(0),
    m_dTotFitAdj(0),
    m_dAvFitAdj(0)
{
    // create the population of genomes
    for (int i=0; i<m_iPopSize; ++i)
    {
        m_vecGenomes.push_back(CGenome(m_iNextGenomeID++, inputs, outputs));
    }


    // Make sure XOR testing mode is off by default
    xorTestMode = false;

    // Make sure debug is turned off by default
    debugMode = false;


    // Make sure automatic hall of fame
    // handling is on by default
    manualHallOfFameControl = false;


    // Set the method Epoch will use
    // to handle population size underflow
    popSizeUnderFlowMode = HIGH_MUTATION_TOURNY_SELECT;


    // create the innovation list. First create a minimal genome
    CGenome genome(1, inputs, outputs);

    // create the innovations
    m_pInnovation = new CInnovation(genome.Genes(), genome.Neurons());

    // create the network depth lookup table
    vecSplits = Split(0, 1, 0);
    tempVecSplits.clear();


    // Default to the compatibility threshold in the parameters file
    compatibilityThresholdToUse   = param_compatibilityThreshold;
}





//------------------------------------- destructor -----------------------------
//
//------------------------------------------------------------------------
Cga::~Cga()
{
    if (m_pInnovation)
    {
        delete m_pInnovation;

        m_pInnovation = NULL;
    }
}





//-------------------------------CreatePhenotypes-------------------------
//
//	cycles through all the members of the population and creates their
//  phenotypes. Returns a vector containing pointers to the new phenotypes
//-------------------------------------------------------------------------
vector<CNeuralNet*> Cga::CreatePhenotypes()
{
    vector<CNeuralNet*> brains;
    int depth;

    for (int i=0; i<m_iPopSize; i++)
    {
        // calculate max network depth
        depth = CalculateNetDepth(m_vecGenomes[i]);
        m_vecGenomes[i].SetDepth(depth);

        // create new phenotype
        CNeuralNet* net = m_vecGenomes[i].CreatePhenotype();

        brains.push_back(net);
    }

    return brains;
}











//-------------------------- CalculateNetDepth ---------------------------
//
//  searches the lookup table for the dSplitY value of each node in the
//  genome and returns the depth of the network based on this figure
//------------------------------------------------------------------------
int Cga::CalculateNetDepth(const CGenome &gen)
{
    int MaxSoFar = 0;

    for (int nd=0; nd<gen.NumNeurons(); ++nd)
    {
        for (int i=0; i<vecSplits.size(); ++i)
        {

            if ((gen.SplitY(nd) == vecSplits[i].val) &&
                    (vecSplits[i].depth > MaxSoFar))
            {
                MaxSoFar = vecSplits[i].depth;
            }
        }
    }

    return MaxSoFar + 2;
}






//-----------------------------------AddNeuronID----------------------------
//
//	just checks to see if a node ID has already been added to a vector of
//  nodes. If not then the new ID  gets added. Used in Crossover.
//------------------------------------------------------------------------
void Cga::AddNeuronID(const int nodeID, vector<int> &vec)
{
    for (int i=0; i<vec.size(); i++)
    {
        if (vec[i] == nodeID)
        {
            // already added
            return;
        }
    }

    vec.push_back(nodeID);

    return;
}





//------------------------------------- Epoch ----------------------------
//
//  This function performs one epoch of the genetic algorithm and returns 
//  a vector of pointers to the new phenotypes
//------------------------------------------------------------------------
vector<CNeuralNet*> Cga::Epoch(const vector<double> &FitnessScores)
{
    // first check to make sure we have the correct amount of fitness scores
    if (FitnessScores.size() != m_vecGenomes.size())
    {
        cout<<"Cga::Epoch, scores/genomes mismatch!"<<endl;
        exit(1);
    }



    // Go ahead and purge all of the species, and update
    // their ages.
    vector<CSpecies>::iterator speciesPtr = m_vecSpecies.begin();
    while (speciesPtr != m_vecSpecies.end())
    {
        speciesPtr->Purge();
        speciesPtr->AgeOneGeneration();
        speciesPtr++;
    }


    // Go ahead and delete all of the phenotypes, we only
    // need the genomes at this point
    for (int i = 0; i < m_vecGenomes.size(); i++)
    {
        m_vecGenomes[i].DeletePhenotype();
    }


    // update the genomes with the fitnesses scored in the last run
    for (int gen=0; gen<m_vecGenomes.size(); ++gen)
    {
        m_vecGenomes[gen].SetFitness(FitnessScores[gen]);
    }


    // sort genomes and keep a record of the best performers
    SortAndRecord();

    // separate the population into species of similar topology, adjust
    // fitnesses and calculate spawn levels
    SpeciateAndCalculateSpawnLevels();


    // Test all the old species to see if they're too old, and
    // their performance is too poor to allow them to reproduce
    TestOldSpeciesForKilling();


    // this will hold the new population of genomes
    vector<CGenome> NewPop;



    // request the offspring from each species. The number of children to
    // spawn is a double which we need to convert to an int.
    int NumSpawnedSoFar = 0;




    CGenome baby;



    // now to iterate through each species selecting offspring to be mated and
    // mutated
    for (int spc=0; spc<m_vecSpecies.size(); ++spc)
    {
        // because of the number to spawn from each species is a double
        // rounded up or down to an integer it is possible to get an overflow
        // of genomes spawned. This statement just makes sure that doesn't
        // happen
        if (NumSpawnedSoFar < param_numAgents)
        {
            // this is the amount of offspring this species is required to
            // spawn. Rounded simply rounds the double up or down.
            int NumToSpawn = Rounded(m_vecSpecies[spc].NumToSpawn());

            bool bChosenBestYet = false;

            while (NumToSpawn--)
            {
                // first grab the best performing genome from this species and transfer
                // to the new population without mutation. This provides per species
                // elitism
                if (!bChosenBestYet)
                {
                    baby = m_vecSpecies[spc].Leader();

                    bChosenBestYet = true;
                }
                else
                {
                    // if the number of individuals in this species is only one
                    // then we can only perform mutation
                    if (m_vecSpecies[spc].NumMembers() == 1)
                    {
                        // spawn a child
                        baby = m_vecSpecies[spc].Spawn();
                    }
                    // if greater than one we can use the crossover operator
                    else
                    {
                        // spawn1
                        CGenome g1 = m_vecSpecies[spc].Spawn();

                        if (RandDouble() < param_crossoverRate)
                        {
                            // spawn2, make sure it's not the same as g1
                            CGenome g2 = m_vecSpecies[spc].Spawn();

                            // number of attempts at finding a different genome
                            int NumAttempts = 5;

                            while ( (g1.ID() == g2.ID()) && (NumAttempts--) )
                            {
                                g2 = m_vecSpecies[spc].Spawn();
                            }

                            if (g1.ID() != g2.ID())
                            {
                                baby = Crossover(g1, g2);
                            }
                        }
                        else
                        {
                            baby = g1;
                        }
                    }


                    ++m_iNextGenomeID;

                    baby.SetID(m_iNextGenomeID);

                    // now we have a spawned child lets mutate it! First there is the
                    // chance a neuron may be added
                    if (baby.NumNeurons() < param_maxPermittedNeurons)
                    {
                        baby.AddNeuron(param_chanceAddNode,
                                       *m_pInnovation,
                                       param_numTrysToFindOldLink);
                    }


                    // now there's the chance a link may be added
                    if (xorTestMode)
                    {
                        // You can't use recurrent links
                        // in XOR testing, it's cheating
                        baby.AddLink(param_chanceAddLink,
                                     0.0,
                                     *m_pInnovation,
                                     param_numTrysToFindLoopedLink,
                                     param_numAddLinkAttempts);
                    }
                    else
                    {
                        // Normal add link call, with chance
                        // of recurrent links
                        baby.AddLink(param_chanceAddLink,
                                     param_chanceAddRecurrentLink,
                                     *m_pInnovation,
                                     param_numTrysToFindLoopedLink,
                                     param_numAddLinkAttempts);
                    }



                    // mutate the weights
                    baby.MutateWeights(param_mutationRate,
                                       param_probabilityWeightReplaced,
                                       param_maxWeightPerturbation);

                    baby.MutateActivationResponse(param_activationMutationRate,
                                                  param_maxActivationPerturbation);
                }

                // sort the babies genes by their innovation numbers
                baby.SortGenes();

                // add to new pop
                NewPop.push_back(baby);

                ++NumSpawnedSoFar;

                if (NumSpawnedSoFar == param_numAgents)
                {
                    NumToSpawn = 0;
                }
            }
        }
    }


    // We could end up with too many genomes due to rounding issues
    // If we have more than the target number, delete them randomly until we
    // have the correct number. Rando death system.
    while (NumSpawnedSoFar > param_numAgents)
    {
        // pick one at random
        int deathIndex = RandInt(0, NumSpawnedSoFar);
        NewPop.erase(NewPop.begin() + deathIndex);
        NumSpawnedSoFar--;
    }


    // Handle cases where the new population size
    // isn't high enough. Fill in the remaining missing
    // genomes with something.
    vector <int> pickedGenomesIDs;

    switch (popSizeUnderFlowMode)
    {
    case ORIGINAL_TOURNY_SELECT:
        // if there is an underflow due to the rounding error and the amount
        // of offspring falls short of the population size additional children
        // need to be created and added to the new population. This is achieved
        // simply, by using tournament selection over the entire population.
        if (NumSpawnedSoFar < param_numAgents)
        {
            // calculate amount of additional children required
            int Rqd = param_numAgents - NumSpawnedSoFar;

            CGenome randomlySelectedGenome;
            bool    cleanPick;

            // grab them
            while (Rqd--)
            {
                cleanPick              = true;
                randomlySelectedGenome = TournamentSelection(m_iPopSize/20);
                int selectedGenomeID   = randomlySelectedGenome.ID();

                if (debugMode)
                {
                    cout<<"---- Tournament Selection randomly picked genome ID: "<<selectedGenomeID<<endl;
                }

                // Make sure the picked genome isn't already
                // in the new population
                for (unsigned int i = 0; i < NewPop.size(); i++)
                {
                    if (selectedGenomeID == NewPop[i].ID())
                    {
                        cleanPick = false;
                        break;
                    }
                }


                if (cleanPick)
                {
                    if (debugMode)
                    {
                        cout<<"---- Tournament Selection adding genomeID: "<<selectedGenomeID<<endl;
                    }

                    NewPop.push_back(randomlySelectedGenome);
                }
                else
                {
                    if (debugMode)
                    {
                        cout<<"---- Tournament Selection picked genomeID: "<<selectedGenomeID<<", but need to pick again."<<endl;
                    }

                    Rqd++;
                }
            }
        }
        break;

    case HIGH_MUTATION_TOURNY_SELECT:
        // This method has the TournamentSelect function
        // pick pretty high fitness genomes. Instead of
        // having to double check that we didn't pick a
        // duplicate genome that's already in the new
        // population, we take that genome and mutate
        // it at higher rates than normal, giving
        // it a new genome ID in the process.
        if (NumSpawnedSoFar < param_numAgents)
        {
            // calculate amount of additional children required
            int Rqd = param_numAgents - NumSpawnedSoFar;

            CGenome randomlySelectedGenome;

            // grab genomes to fill the empty space in the population
            while (Rqd--)
            {
                int  numTriesToFindUnique = 10;
                bool repeatedGenomeID;
                int  selectedGenomeID;

                while (numTriesToFindUnique--)
                {
                    randomlySelectedGenome = TournamentSelection(m_iPopSize/5);
                    selectedGenomeID       = randomlySelectedGenome.ID();
                    repeatedGenomeID       = false;

                    if (debugMode)
                    {
                        cout<<"---- HIGH MUTATE Tourny Select randomly picked genome ID: "<<selectedGenomeID<<", Try counter: "<<numTriesToFindUnique<<endl;
                    }

                    for (int i = 0; i < pickedGenomesIDs.size(); i++)
                    {
                        if (selectedGenomeID == pickedGenomesIDs[i])
                        {
                            if (debugMode)
                            {
                                cout<<"X-XX-X HIGH MUTATE Tourny Select already picked that genome, trying again..."<<endl;
                            }

                            repeatedGenomeID = true;
                            break;
                        }
                    }

                    if (!repeatedGenomeID)
                    {
                        if (debugMode)
                        {
                            cout<<"X-XX-X HIGH MUTATE Tourny Select found a unique genome ID"<<endl;
                        }

                        numTriesToFindUnique = 0;
                    }
                }


                if (debugMode)
                {
                    cout<<"---- HIGH MUTATE Tourny Select ended up picking genome ID: "<<selectedGenomeID<<endl;
                }


                // Store this genome ID so we can tell if we've
                // picked it before or not. Try to keep a nice
                // broad selection of genomes to base our fill-in
                // mutated genomes off of.
                pickedGenomesIDs.push_back(selectedGenomeID);



                // Ok, now we've picked a pretty high fitness genome. Let's
                // mutate it heavily, and give it a new genomeID
                ++m_iNextGenomeID;
                randomlySelectedGenome.SetID(m_iNextGenomeID);

                if (debugMode)
                {
                    cout<<"--++ HIGH MUTATE Tourny Select changed genome ID to: "<<randomlySelectedGenome.ID()<<", commencing mutation."<<endl;
                }

                // Add neurons...
                if (randomlySelectedGenome.NumNeurons() < param_maxPermittedNeurons)
                {
                    randomlySelectedGenome.AddNeuron(param_underFlowMutationMultiplier * param_chanceAddNode,
                                                     *m_pInnovation,
                                                     param_numTrysToFindOldLink);
                }

                // Add links...
                if (xorTestMode)
                {
                    // If we're in XOR testing mode, make sure recurrent links are disabled
                    randomlySelectedGenome.AddLink(param_underFlowMutationMultiplier * param_chanceAddLink,
                                                   0.0,
                                                   *m_pInnovation,
                                                   param_numTrysToFindLoopedLink,
                                                   param_numAddLinkAttempts);
                }
                else
                {
                    randomlySelectedGenome.AddLink(param_underFlowMutationMultiplier * param_chanceAddLink,
                                                   param_underFlowMutationMultiplier * param_chanceAddRecurrentLink,
                                                   *m_pInnovation,
                                                   param_numTrysToFindLoopedLink,
                                                   param_numAddLinkAttempts);

                }

                // Mutate the weights
                randomlySelectedGenome.MutateWeights(param_underFlowMutationMultiplier * param_mutationRate,
                                                     param_probabilityWeightReplaced,
                                                     param_maxWeightPerturbation);

                randomlySelectedGenome.MutateActivationResponse(param_underFlowMutationMultiplier * param_activationMutationRate,
                                                                param_maxActivationPerturbation);

                randomlySelectedGenome.SortGenes();

                NewPop.push_back(randomlySelectedGenome);
            }
        }
        break;

    default:
        cout<<"Error, invalid popSizeUnderFlowMode in Epoch method."<<endl;
        exit(1);
    }


    // Make sure the underflow code
    // properly filled in the right
    // number of genomes
    if (NewPop.size() != m_iPopSize)
    {
        cout<<"Error in Epoch, population size is incorrect after underflow code, popSize: "<<NewPop.size()<<endl;
        exit(1);
    }


    // replace the current population with the new one
    m_vecGenomes = NewPop;


    // Now we have our new population of genomes.
    // Double check that there aren't any duplicates.
    vector <int> popGenomeIDs;

    for (unsigned int i = 0; i < m_vecGenomes.size(); i++)
    {
        popGenomeIDs.push_back(m_vecGenomes[i].ID());
    }

    sort(popGenomeIDs.begin(), popGenomeIDs.end());

    for (unsigned int i = 0; i < popGenomeIDs.size() - 1; i++)
    {
        if (popGenomeIDs[i] == popGenomeIDs[i+1])
        {
            cout<<"!!!!!!! Error, at end of Epoch, new population contains duplicate Genomes! (GenomeIDs: "<<popGenomeIDs[i]<<")"<<endl;

            cout<<"Species leader Genome ID dump:"<<endl;

            for (unsigned int j = 0; j < m_vecSpecies.size(); j++)
            {
                cout<<"Species ID: "<<m_vecSpecies[j].ID()<<", Leader genome ID: "<<m_vecSpecies[j].Leader().ID()<<endl;
            }

            for (unsigned int k = 0; k < param_numAgents; k++)
            {
                cout<<"^^^ genomeIndex: "<<k<<". ID: "<<m_vecGenomes[k].ID()<<", species: "<<m_vecGenomes[k].GetSpecies()<<endl;
            }

            exit(1);
        }
    }


    // create the new phenotypes
    vector<CNeuralNet*> new_phenotypes;



    for (int gen=0; gen<m_vecGenomes.size(); ++gen)
    {
        // calculate max network depth
        int depth = CalculateNetDepth(m_vecGenomes[gen]);
        m_vecGenomes[gen].SetDepth(depth);

        CNeuralNet* phenotype = m_vecGenomes[gen].CreatePhenotype();

        new_phenotypes.push_back(phenotype);
    }

    // increase generation counter
    ++m_iGeneration;

    return new_phenotypes;
}















//--------------------------- SortAndRecord-------------------------------
//
//  sorts the population into descending fitness, keeps a record of the
//  best n genomes and updates any fitness statistics accordingly
//------------------------------------------------------------------------
void Cga::SortAndRecord()
{
    bool resetBestEverFitnessScores;

    // If we're doing XOR testing, ignore the
    // param_resetBestEverFitnessEachGen
    if (xorTestMode)
    {
        resetBestEverFitnessScores = false;
    }
    else
    {
        resetBestEverFitnessScores = param_resetBestEverFitnessEachGen;
    }


    // sort the genomes according to their unadjusted (no fitness sharing)
    // fitnesses
    sort(m_vecGenomes.begin(), m_vecGenomes.end());

    // Resetting "best fitness ever" each generation,
    // so it's just a measure of best fitness for this
    // particular generation
    if (resetBestEverFitnessScores)
    {
        m_dBestEverFitness = m_vecGenomes[0].Fitness();
    }
    // Keeping track of best fitness ever across all generations
    else
    {
        // is the best genome this generation the best ever?
        if (m_vecGenomes[0].Fitness() > m_dBestEverFitness)
        {
            m_dBestEverFitness = m_vecGenomes[0].Fitness();
        }
    }

    // keep a record of the n best genomes
    StoreBestGenomes();
}






//----------------------------- StoreBestGenomes -------------------------
//
//  used to keep a record of the previous populations best genomes so that 
//  they can be displayed if required.
//------------------------------------------------------------------------
void Cga::StoreBestGenomes()
{
    // clear old record
    m_vecBestGenomes.clear();

    for (int gen=0; gen<param_numAgents; ++gen)
    {
        m_vecBestGenomes.push_back(m_vecGenomes[gen]);
    }

    // Only automagically do this if hall of fame
    // isn't being controlled externally
    if (!manualHallOfFameControl)
    {
        // Grab the best genomes from this generation
        // to add to the hall of fame
        m_vecHallOfFameGenomes.push_back((m_vecBestGenomes[0]));
    }
}





//----------------- GetBestPhenotypesFromLastGeneration ------------------
//
//  returns a std::vector of the n best phenotypes from the previous
//  generation
//------------------------------------------------------------------------
vector<CNeuralNet*> Cga::GetBestPhenotypesFromLastGeneration(int counter)
{
    vector<CNeuralNet*> brains;
    int depth;

    for (int gen=0; gen < counter; ++gen)
    {
        // calculate max network depth
        depth = CalculateNetDepth(m_vecBestGenomes[gen]);
        m_vecBestGenomes[gen].SetDepth(depth);
        brains.push_back(m_vecBestGenomes[gen].CreatePhenotype());
    }

    return brains;
}


void Cga::ClearBestPhenotypesFromLastGeneration(int counter)
{
    for (unsigned int i = 0; i < counter; i++)
    {
        m_vecBestGenomes[i].DeletePhenotype();
    }
}






//----------------- GetSingleBestPhenotypeFromLastGeneration ---------
//
//  returns a pointer to the best fitness phenotype from the
//  last generation.
//------------------------------------------------------------------------
CNeuralNet* Cga::GetSingleBestPhenotypeFromLastGeneration()
{
    CNeuralNet* brain;
    int depth;

    depth = CalculateNetDepth(m_vecBestGenomes[0]);
    m_vecBestGenomes[0].SetDepth(depth);
    brain = m_vecBestGenomes[0].CreatePhenotype();

    return brain;
}



//----------------- GetSingleGenome ---------
//
//  returns a copy of the genome at a given index
//
//------------------------------------------------------------------------
CGenome Cga::GetSingleGenome(int index)
{
    if (index >= m_vecGenomes.size())
    {
        cout<<"Error, trying to retrieve genome index that is out of bounds: "<<index<<endl;
        exit(1);
    }
    return m_vecGenomes.at(index);
}



//----------------- GetNumberOfGenotypesSharingTopFitness ---------
//
//  Counts the number of genomes that have tied for the best score
//------------------------------------------------------------------------
int Cga::GetNumberOfGenotypesSharingTopFitness()
{
    int    numTiedGenomes = 0;
    double bestScore      = m_vecBestGenomes[0].Fitness();


    for (int i = 0; i < m_vecBestGenomes.size(); i++)
    {
        if (m_vecBestGenomes[i].Fitness() == bestScore)
        {
            numTiedGenomes++;
        }
        else
        {
            return numTiedGenomes;
        }
    }
    return numTiedGenomes;
}





//----------------- GetHallOfFamePhenotypes  ---------------------------
//
//  returns a std::vector of the phenotypes of the hall of fame
//------------------------------------------------------------------------
vector<CNeuralNet*> Cga::GetHallOfFamePhenotypes()
{
    vector<CNeuralNet*> brains;
    int depth;

    for (int i = 0; i < m_vecHallOfFameGenomes.size(); i++)
    {
        // calculate max network depth
        depth = CalculateNetDepth(m_vecHallOfFameGenomes[i]);
        m_vecHallOfFameGenomes[i].SetDepth(depth);
        brains.push_back(m_vecHallOfFameGenomes[i].CreatePhenotype());
    }

    return brains;
}







//----------------- GetCustomArchivePhenotypes  ---------------------------
//
//  returns a std::vector of the phenotypes in the custom archive
//------------------------------------------------------------------------
vector<CNeuralNet*> Cga::GetCustomArchivePhenotypes()
{
    vector<CNeuralNet*> brains;
    int depth;

    for (int i = 0; i < m_vecCustomArchiveGenomes.size(); i++)
    {
        // calculate max network depth
        depth = CalculateNetDepth(m_vecCustomArchiveGenomes[i]);
        m_vecCustomArchiveGenomes[i].SetDepth(depth);
        brains.push_back(m_vecCustomArchiveGenomes[i].CreatePhenotype());
    }

    return brains;
}









//----------------- GetLeaderPhenotypesFromEachSpecies---------------------
//
//  returns a std::vector of the phenotypes of the best of each species
//------------------------------------------------------------------------
vector<CNeuralNet*> Cga::GetLeaderPhenotypesFromEachSpecies()
{
    m_vecSpeciesLeaderGenomes.clear();

    vector<CNeuralNet*> brains;
    int depth;

    for (int sp = 0; sp < m_vecSpecies.size(); sp++)
    {
        m_vecSpeciesLeaderGenomes.push_back(m_vecSpecies[sp].Leader());
        depth = CalculateNetDepth(m_vecSpeciesLeaderGenomes[sp]);
        m_vecSpeciesLeaderGenomes[sp].SetDepth(depth);
        brains.push_back(m_vecSpeciesLeaderGenomes[sp].CreatePhenotype());
    }

    return brains;
}









//--------------------------- AdjustSpecies ------------------------------
//
//  this functions simply iterates through each species and calls 
//  AdjustFitness for each species
//------------------------------------------------------------------------
void Cga::AdjustSpeciesFitnesses()
{
    for (int sp=0; sp<m_vecSpecies.size(); ++sp)
    {
        m_vecSpecies[sp].AdjustFitnesses();
    }
}





//------------------ SpeciateAndCalculateSpawnLevels ---------------------
//
//  separates each individual into its respective species by calculating
//  a compatibility score with every other member of the population and 
//  niching accordingly. The function then adjusts the fitness scores of
//  each individual by species age and by sharing and also determines
//  how many offspring each individual should spawn.
//------------------------------------------------------------------------
void Cga::SpeciateAndCalculateSpawnLevels()
{
    int  speciationPassCounter  = 1;
    bool speciesLeadersAdjusted;
    bool speciateAgain;
    bool bAdded;

    m_dTotFitAdj = 0;
    m_dAvFitAdj  = 0;

    // The species leaders need to be
    // set for appropriate comparisons of
    // genomes for inclusion into species.
    // Keep looping through this speciation
    // code until the leaders don't change,
    // and no new species are added. Then
    // you can do fitness sharing and such
    // after the loop
    do
    {
        speciationPassCounter++;
        speciesLeadersAdjusted = false;
        speciateAgain          = false;
        bAdded                 = false;


        // We shouldn't end up looping in this forever.
        // If we do this too many times, somethign is amiss.
        // Check for crazy numbers of passes, and exit with
        // an error.
        if (speciationPassCounter >= 500)
        {
            cout<<"Error, Cga::SpeciateAndCalculateSpawnLevels seemingly stuck in loop, pass counter: "<<speciationPassCounter<<endl;
            exit(1);
        }

        // Go ahead and purge all of the species
        vector<CSpecies>::iterator speciesPtr = m_vecSpecies.begin();
        while (speciesPtr != m_vecSpecies.end())
        {
            speciesPtr->Purge();
            speciesPtr++;
        }


        // iterate through each genome and speciate
        for (int gen=0; gen<m_vecGenomes.size(); ++gen)
        {
            // calculate its compatibility score with each species leader. If
            // compatible add to species. If not, create a new species
            for (int spc=0; spc<m_vecSpecies.size(); ++spc)
            {
                double compatibility = m_vecGenomes[gen].GetCompatibilityScore(m_vecSpecies[spc].Leader());

                // if this individual is similar to this species add to species
                if (compatibility <= compatibilityThresholdToUse)
                {
                    speciesLeadersAdjusted = m_vecSpecies[spc].AddMember(m_vecGenomes[gen]);

                    if (speciesLeadersAdjusted)
                    {
                        speciateAgain = true;
                    }

                    // let the genome know which species it's in
                    m_vecGenomes[gen].SetSpecies(m_vecSpecies[spc].ID());

                    bAdded = true;

                    break;
                }
            }

            if (!bAdded)
            {
                // we have not found a compatible species so let's create a new one
                m_vecSpecies.push_back(CSpecies(m_vecGenomes[gen], m_iNextSpeciesID++));
                speciateAgain = true;
            }

            bAdded = false;
        }
    } while (speciateAgain);

    // We now have cleanly assigned species leaders after the loop above.
    // Speciation is good now


    // Make sure we rid ouselves of any species with no members
    vector<CSpecies>::iterator curSp = m_vecSpecies.begin();
    while (curSp != m_vecSpecies.end())
    {
        if (curSp->NumMembers() == 0)
        {
            curSp->Purge();
            curSp = m_vecSpecies.erase(curSp);
        }
        else
        {
            ++curSp;
        }
    }


    // now all the genomes have been assigned a species the fitness scores
    // need to be adjusted to take into account sharing and species age.
    AdjustSpeciesFitnesses();




    // calculate new adjusted total & average fitness for the population
    for (int gen=0; gen<m_vecGenomes.size(); ++gen)
    {
        m_dTotFitAdj += m_vecGenomes[gen].GetAdjFitness();
    }

    m_dAvFitAdj = m_dTotFitAdj/m_vecGenomes.size();


    // calculate how many offspring each member of the population
    // should spawn
    for (int gen=0; gen<m_vecGenomes.size(); ++gen)
    {
        double ToSpawn = m_vecGenomes[gen].GetAdjFitness() / m_dAvFitAdj;
        m_vecGenomes[gen].SetAmountToSpawn(ToSpawn);
    }

    // iterate through all the species and calculate how many offspring
    // each species should spawn
    for (int spc=0; spc<m_vecSpecies.size(); ++spc)
    {
        m_vecSpecies[spc].CalculateSpawnAmount();
    }
}




//------------------ TestOldSpeciesForReproductionRights -----------------
//
//  Go through the older species, and make sure the worst one can't
//  reproduce. Use the species leader fitness (which has been adjusted at
//  this point) to judge the species itself.
//------------------------------------------------------------------------
void Cga::TestOldSpeciesForKilling()
{
    int    worstSpeciesIndex = 0;
    int    numberOfSpecies   = m_vecSpecies.size();
    bool   killableSpecies   = false;

    // This is padded a little bit, elsewise the species with index 0 will never get killed off
    double worstFitnessSeen    = m_vecSpecies[0].BestFitness() + 1.0;

    // If there's only 1 species, obviously we
    // shouldn't kill it off.
    if (numberOfSpecies == 1)
    {
        return;
    }

    // If the species is old enough, and fitness bad enough, consider it for killing.
    for (int i = 0; i < numberOfSpecies; i++)
    {
        if ((m_vecSpecies[i].Age() > param_killWorstSpeciesThisOld) &&
                (m_vecSpecies[i].BestFitness() < worstFitnessSeen)  &&
                (m_vecSpecies[i].BestFitness() < m_dBestEverFitness))
        {
            worstFitnessSeen  = m_vecSpecies[i].BestFitness();
            worstSpeciesIndex = i;
            killableSpecies   = true;
        }
    }

    if (killableSpecies)
    {
        m_vecSpecies[worstSpeciesIndex].Purge();
        m_vecSpecies.erase(m_vecSpecies.begin() + worstSpeciesIndex);
    }
}





//--------------------------- TournamentSelection ------------------------
//
//------------------------------------------------------------------------
CGenome Cga::TournamentSelection(const int NumComparisons)
{
    double BestFitnessSoFar = 0;

    int ChosenOne = 0;

    // Select NumComparisons members from the population at random testing
    // against the best found so far
    for (int i=0; i<NumComparisons; ++i)
    {
        int  ThisTry  = RandInt(0, m_vecGenomes.size()-1);

        if (m_vecGenomes[ThisTry].Fitness() > BestFitnessSoFar)
        {
            ChosenOne = ThisTry;

            BestFitnessSoFar = m_vecGenomes[ThisTry].Fitness();
        }
    }

    // return the champion
    return m_vecGenomes[ChosenOne];
}






//-----------------------------------Crossover----------------------------
//	Perform crossover of two genomes
//------------------------------------------------------------------------
CGenome Cga::Crossover(CGenome& mum, CGenome& dad)
{

    // helps make the code clearer
    enum parent_type{MUM, DAD,};

    // first, calculate the genome we will using the disjoint/excess
    // genes from. This is the fittest genome.
    parent_type best;

    // if they are of equal fitness use the shorter (because we want to keep
    // the networks as small as possible)
    if (mum.Fitness() == dad.Fitness())
    {
        // if they are of equal fitness and length just choose one at
        // random
        if (mum.NumGenes() == dad.NumGenes())
        {
            best = (parent_type)RandInt(0, 1);
        }
        else
        {
            if (mum.NumGenes() < dad.NumGenes())
            {
                best = MUM;
            }
            else
            {
                best = DAD;
            }
        }
    }
    else
    {
        if (mum.Fitness() > dad.Fitness())
        {
            best = MUM;
        }
        else
        {
            best = DAD;
        }
    }


    // these vectors will hold the offspring's nodes and genes
    vector<SNeuronGene>  BabyNeurons;
    vector<SLinkGene>    BabyGenes;


    // temporary vector to store all added node IDs
    vector<int> vecNeurons;

    // create iterators so we can step through each parents genes and set
    // them to the first gene of each parent
    vector<SLinkGene>::iterator curMum = mum.StartOfGenes();
    vector<SLinkGene>::iterator curDad = dad.StartOfGenes();

    // this will hold a copy of the gene we wish to add at each step
    SLinkGene SelectedGene;

    // step through each parents genes until we reach the end of both
    while (!((curMum == mum.EndOfGenes()) && (curDad == dad.EndOfGenes())))
    {
        // the end of mum's genes have been reached
        if ((curMum == mum.EndOfGenes())&&(curDad != dad.EndOfGenes()))
        {
            // if dad is fittest
            if (best == DAD)
            {
                // add dads genes
                SelectedGene = *curDad;
            }

            // move onto dad's next gene
            ++curDad;
        }

        // the end of dads's genes have been reached
        else if ((curDad == dad.EndOfGenes()) && (curMum != mum.EndOfGenes()))
        {
            // if mum is fittest
            if (best == MUM)
            {
                // add mums genes
                SelectedGene = *curMum;
            }

            // move onto mum's next gene
            ++curMum;
        }

        // if mums innovation number is less than dads
        else if (curMum->InnovationID < curDad->InnovationID)
        {
            // if mum is fittest add gene
            if (best == MUM)
            {
                SelectedGene = *curMum;
            }

            // move onto mum's next gene
            ++curMum;
        }

        // if dads innovation number is less than mums
        else if (curDad->InnovationID < curMum->InnovationID)
        {
            // if dad is fittest add gene
            if (best == DAD)
            {
                SelectedGene = *curDad;
            }

            // move onto dad's next gene
            ++curDad;
        }

        // if innovation numbers are the same
        else if (curDad->InnovationID == curMum->InnovationID)
        {
            // grab a gene from either parent
            if (RandDouble() < 0.5f)
            {
                SelectedGene = *curMum;
            }
            else
            {
                SelectedGene = *curDad;
            }

            // move onto next gene of each parent
            ++curMum;
            ++curDad;
        }

        // add the selected gene if not already added
        if (BabyGenes.size() == 0)
        {
            BabyGenes.push_back(SelectedGene);
        }
        else
        {
            if (BabyGenes[BabyGenes.size()-1].InnovationID !=
                    SelectedGene.InnovationID)
            {
                BabyGenes.push_back(SelectedGene);
            }
        }

        // Check if we already have the nodes referred to in SelectedGene.
        // If not, they need to be added.
        AddNeuronID(SelectedGene.FromNeuron, vecNeurons);
        AddNeuronID(SelectedGene.ToNeuron, vecNeurons);
    }



    // now create the required nodes. First sort them into order
    sort(vecNeurons.begin(), vecNeurons.end());

    for (int i=0; i<vecNeurons.size(); i++)
    {
        BabyNeurons.push_back(m_pInnovation->CreateNeuronFromID(vecNeurons[i]));
    }

    // finally, create the genome
    CGenome babyGenome(m_iNextGenomeID++,
                       BabyNeurons,
                       BabyGenes,
                       mum.NumInputs(),
                       mum.NumOutputs());

    return babyGenome;
}









//------------------------------- Split ----------------------------------
//
//  this function is used to create a lookup table that is used to
//  calculate the depth of the network. 
//------------------------------------------------------------------------
vector<SplitDepth> Cga::Split(double low, double high, int depth)
{
    double span = high-low;

    tempVecSplits.push_back(SplitDepth(low + span/2, depth+1));

    if (depth > 6)
    {
        return tempVecSplits;
    }
    else
    {
        Split(low, low+span/2, depth+1);
        Split(low+span/2, high, depth+1);

        return tempVecSplits;
    }
}





//--------------------- SetCompatibilityThreshold------------------------
//
//  this function is used to change the compatibility threshold
//  used for speciation
//------------------------------------------------------------------------
void Cga::SetCompatibilityThreshold(double newThreshold)
{
    if (newThreshold > param_maxCompatibilityThreshold)
    {
        compatibilityThresholdToUse = param_maxCompatibilityThreshold;
    }
    else if (newThreshold < param_minCompatibilityThreshold)
    {
        compatibilityThresholdToUse = param_minCompatibilityThreshold;
    }
    else
    {
        compatibilityThresholdToUse = newThreshold;
    }
}





//--------------------- GetCurrentCompatibilityThreshold------------------
//
//  this function is used to get the current compatibility threshold
//  value
//------------------------------------------------------------------------
double Cga::GetCurrentCompatibilityThreshold()
{
    return compatibilityThresholdToUse;
}





//--------------------- GetNumHallOfFamers-------------------------------
//
//  this function is used to get the size of the hall of fame
//------------------------------------------------------------------------
int Cga::GetNumHallOfFamers()
{
    return m_vecHallOfFameGenomes.size();
}




//--------------------- GetNumCustomArchivers-------------------------------
//
//  this function is used to get the size of the custom archive
//------------------------------------------------------------------------
int Cga::GetNumCustomArchivers()
{
    return m_vecCustomArchiveGenomes.size();
}




//--------------------- AddGenomeToCustomArchive-------------------------------
//
//  this function is used to add genomes to the custom archive
//------------------------------------------------------------------------
void Cga::AddGenomeToCustomArchive(int index, int genomeID)
{
    if (genomeID != m_vecGenomes.at(index).ID())
    {
        cout<<"Error in Cga::AddGenomeToCustomArchive, genome ID's don't match!"<<endl;
        exit(1);
    }

    m_vecCustomArchiveGenomes.push_back(m_vecGenomes.at(index));
}




//--------------------- ClearCustomArchive-------------------------------
//
//  this function is used to delete all genomes in the custom archive
//------------------------------------------------------------------------
void Cga::ClearCustomArchive()
{
    for (unsigned int i = 0; i < m_vecCustomArchiveGenomes.size(); i++)
    {
        m_vecCustomArchiveGenomes[i].DeletePhenotype();
    }

    m_vecCustomArchiveGenomes.clear();
}





//--------------------- SetManualHallOfFameControl-------------------------------
//
//  this function is used to set manualHallOfFameControl
//------------------------------------------------------------------------
void Cga::SetManualHallOfFameControl(bool newValue)
{
    manualHallOfFameControl = newValue;
}





//--------------------- AddGenomeToHallOfFame-------------------------------
//
//  this function is used to set add a genome to the hall of fame.
//  this is only valid if manual control of hall of fame is set
//  to true.
//------------------------------------------------------------------------
void Cga::AddGenomeToHallOfFame(int index, int genomeID)
{
    if (!manualHallOfFameControl)
    {
        cout<<"Cga::AddGenomeToHallOfFame, trying to add genome to Hall of Fame when manual control is turned off!"<<endl;
        exit(1);
    }

    if (m_vecGenomes.at(index).ID() != genomeID)
    {
        cout<<"Cga::AddGenomeToHallOfFame, genome ID's don't match!"<<endl;
        exit(1);
    }

    m_vecHallOfFameGenomes.push_back(m_vecGenomes[index]);
}




//--------------------- WriteGenomeToFile---------------------------------
//
//  this function is used to get the size of the hall of fame
//------------------------------------------------------------------------
bool Cga::WriteGenomeToFile(int genomeID, string fileName)
{
    // search for the genome with that ID number, and
    // save it's genes into a .dna file for later use

    // Make sure the depth is calculated for the genome
    // before it is written.
    int depth;

    cout<<"Cga is saving genome ID: "<<genomeID<<" to file: "<<fileName<<endl;

    // First search the general vector of genomes
    for (unsigned int i = 0; i < m_vecGenomes.size(); i++)
    {
        if (m_vecGenomes[i].ID() == genomeID)
        {
            depth = CalculateNetDepth(m_vecGenomes[i]);
            m_vecGenomes[i].SetDepth(depth);
            return m_vecGenomes[i].WriteGenomeToFile(fileName);
        }
    }

    // Well, it wasn't in the general vector of genomes,
    // try the vector of Hall of Famers
    for (unsigned int i = 0; i < m_vecHallOfFameGenomes.size(); i++)
    {
        if (m_vecHallOfFameGenomes[i].ID() == genomeID)
        {
            depth = CalculateNetDepth(m_vecHallOfFameGenomes[i]);
            m_vecHallOfFameGenomes[i].SetDepth(depth);
            return m_vecHallOfFameGenomes[i].WriteGenomeToFile(fileName);
        }
    }

    // Ok. Wasn't in either of those, try the vector of the best genomes
    for (unsigned int i = 0; i < m_vecBestGenomes.size(); i++)
    {
        if (m_vecBestGenomes[i].ID() == genomeID)
        {
            depth = CalculateNetDepth(m_vecBestGenomes[i]);
            m_vecBestGenomes[i].SetDepth(depth);
            return m_vecBestGenomes[i].WriteGenomeToFile(fileName);
        }
    }

    cout<<"Error, Cga failed to find genome ID "<<genomeID<<" for saving!"<<endl;
    return false;
}




// Test function. Not for general use, just for
// debuggin purposes.
void Cga::dumpHofGenomesToFile(string descriptor)
{
    int depth;
    stringstream filenameStream;

    for (unsigned int i = 0; i < m_vecHallOfFameGenomes.size(); i++)
    {
        filenameStream.str("");
        depth = CalculateNetDepth(m_vecHallOfFameGenomes[i]);
        m_vecHallOfFameGenomes[i].SetDepth(depth);
        filenameStream<<descriptor.c_str()<<"_"<<m_vecHallOfFameGenomes[i].ID();
        //        cout<<"Cga dump HOF code, counter: "<<i<<", dumping: "<<filenameStream.str()<<endl;
        m_vecHallOfFameGenomes[i].WriteGenomeToFile(filenameStream.str());
    }
}







//--------------------- RunXorTestAndExit---------------------------
//  Runs a XOR test with the current parameters in parameters.h
//  Sees if a network can be evolved (not using recurrent links!)
//  that can solve XOR networks. To solve them, a hidden node is
//  required (at least one).
//  XOR test NN's take two inputs (0 or 1). If one, and *only* one of the
//  inputs is 1, is the output true. Such as:
//  Inputs (0, 0)   output 0
//  Inputs (1, 0)   output 1
//  Inputs (0, 1)   output 1
//  Inputs (1, 1)   output 0
//------------------------------------------------------------------
void Cga::RunXorTestAndExit()
{
    // By setting this to true, Epoch will
    // perform mutations and such correctly
    // for XOR tests
    xorTestMode = true;

    vector <CNeuralNet*> xorBrainsVector;
    vector <double>      brainInputsVector;
    vector <double>      brainOutputsVector;
    vector <double>      fitnessVector;

    vector <int>         generationSolutionFoundVector;
    vector <int>         numHiddenNodesInSolutionsVector;

    double brainInput1;
    double brainInput2;
    double brainOutput;
    double expectedOutput;

    double fitness;
    int    numTestsCorrect;

    bool   solutionFound;

    int    numRunsFoundSolution = 0;

    // These hold the order of
    // inputs. This will be
    // randomized each generation
    int    testOrder[4];
    double testPatterns[2][4];

    testPatterns[0][0] = 0.0;
    testPatterns[1][0] = 0.0;

    testPatterns[0][1] = 1.0;
    testPatterns[1][1] = 0.0;

    testPatterns[0][2] = 0.0;
    testPatterns[1][2] = 1.0;

    testPatterns[0][3] = 1.0;
    testPatterns[1][3] = 1.0;


    // This function runs, and then exits. By the time
    // this function is called, the Cga has already been
    // setup for the particular experimental application.
    // We'll have to undo some of that to run the internal
    // XOR experiment.

    // How many times do we repeat the test?
    for (int xorTestRuns = 0; xorTestRuns < param_numXorTestRuns; xorTestRuns++)
    {
        cout<<"XOR test run "<<xorTestRuns+1<<" of "<<param_numXorTestRuns<<endl;
        // Reset everything for a fresh start
        m_vecGenomes.clear();
        m_vecBestGenomes.clear();
        m_vecHallOfFameGenomes.clear();
        m_vecSpeciesLeaderGenomes.clear();
        m_vecSpecies.clear();
        vecSplits.clear();
        tempVecSplits.clear();

        m_iGeneration      = 0;
        m_pInnovation      = NULL;
        m_iNextGenomeID    = 0;
        m_iNextSpeciesID   = 0;
        m_iFittestGenome   = 0;
        m_dBestEverFitness = 0;
        m_dTotFitAdj       = 0;
        m_dAvFitAdj        = 0;

        solutionFound      = false;

        // For XOR test network, 2 inputs, 1 output
        for (int i = 0; i < m_iPopSize; i++)
        {
            m_vecGenomes.push_back(CGenome(m_iNextGenomeID++, 2, 1));
        }

        // create the innovation list. First create a minimal genome
        CGenome genome(1, 2, 1);

        // create the innovations
        m_pInnovation = new CInnovation(genome.Genes(), genome.Neurons());

        // create the network depth lookup table
        vecSplits = Split(0, 1, 0);


        // Default to the compatibility threshold in the parameters file
        compatibilityThresholdToUse   = param_compatibilityThreshold;

        xorBrainsVector = CreatePhenotypes();



        // Now run a test
        for (int generationCounter = 0; generationCounter < param_maxNumGenerations; generationCounter++)
        {
            // For each generation, randomly pick a new order of inputs
            for (int i = 0; i < 4; i++)
            {
                testOrder[i] = -1;
            }

            for (int i = 0; i < 4; i++)
            {
                int randomInputIndex = RandInt(0, 3);
                for (int j = 0; j < 4; j++)
                {
                    if (randomInputIndex == testOrder[j])
                    {
                        randomInputIndex = RandInt(0, 3);
                        j = -1;
                    }
                }
                testOrder[i] = randomInputIndex;
            }
            // Test order randomized now


            fitnessVector.clear();


            // Now loop through all the brains and test them
            for (int brainCounter = 0; brainCounter < m_iPopSize; brainCounter++)
            {
                fitness         = 0.0;
                numTestsCorrect = 0;

                // Each brain is tested 4 times
                for (int testCase = 0; testCase < 4; testCase++)
                {
                    brainInputsVector.clear();
                    brainOutputsVector.clear();

                    brainInput1 = testPatterns[0][testOrder[testCase]];
                    brainInput2 = testPatterns[1][testOrder[testCase]];

                    brainInputsVector.push_back(brainInput1);
                    brainInputsVector.push_back(brainInput2);

                    brainOutputsVector = xorBrainsVector.at(brainCounter)->Update(brainInputsVector, CNeuralNet::active);

                    // Only one output in the XOR test
                    brainOutput = brainOutputsVector.at(0);


                    // figure out the desired output
                    if ((brainInput1 == 0.0) && (brainInput2 == 0.0))
                    {
                        expectedOutput = 0.0;
                    }
                    else if ((brainInput1 == 1.0) && (brainInput2 == 0.0))
                    {
                        expectedOutput = 1.0;
                    }
                    else if ((brainInput1 == 0.0) && (brainInput2 == 1.0))
                    {
                        expectedOutput = 1.0;
                    }
                    else if ((brainInput1 == 1.0) && (brainInput2 == 1.0))
                    {
                        expectedOutput = 0.0;
                    }

                    // Brain says true (anything 0.5 or higher is true
                    if (brainOutput >= 0.5)
                    {
                        if (expectedOutput == 1.0)
                        {
                            // Hey, the brain got it right!
                            numTestsCorrect++;
                        }
                    }
                    else // brain says false
                    {
                        if (expectedOutput == 0.0)
                        {
                            // Hey, the brain got it right!
                            numTestsCorrect++;
                        }
                    }

                    // Figure out the fitness score adjustment
                    fitness += fabs(expectedOutput - brainOutput);
                }


                // Found the XOR solution!
                if (numTestsCorrect == 4)
                {
                    cout<<"Found the XOR solution in generation "<<generationCounter<<endl;
                    cout<<"Solution has: "<<xorBrainsVector.at(brainCounter)->getNumberOfHiddenNodes()<<" hidden nodes"<<endl;
                    cout<<endl;

                    // A little record keeping...
                    numRunsFoundSolution++;
                    generationSolutionFoundVector.push_back(generationCounter);
                    numHiddenNodesInSolutionsVector.push_back(xorBrainsVector.at(brainCounter)->getNumberOfHiddenNodes());



                    // A dash of error checking...
                    if (xorBrainsVector.at(brainCounter)->getNumberOfHiddenNodes() == 0)
                    {
                        cout<<"Error during XOR test validation, solution with 0 hidden nodes, which is impossible"<<endl;
                        exit(1);
                    }

                    if (xorBrainsVector.at(brainCounter)->getNumberOfRecurrentConnections() > 0)
                    {
                        cout<<"Error during XOR test validation, solution found with recurrent connections. No recurrent connections allowed during XOR validation!"<<endl;
                        exit(1);
                    }


                    // Skip the rest of the generation....
                    solutionFound     = true;
                    generationCounter = param_maxNumGenerations;
                    brainCounter      = m_iPopSize;
                }

                // Scoring stuff
                // Reverse it so higher fitness means better performance
                fitness = 4.0 - fitness;
                fitness = fitness * fitness;

                // Square the fitness
                fitnessVector.push_back(fitness);
            }

            // Epoch. Returns the new brains
            if (!solutionFound)
            {
                xorBrainsVector = Epoch(fitnessVector);
            }
        }
    }



    // Calculate and print out XOR test results
    float meanGeneration     = 0;
    float meanNumHiddenNodes = 0.0;

    for (int i = 0; i < generationSolutionFoundVector.size(); i++)
    {
        meanGeneration += (float)generationSolutionFoundVector.at(i);
    }

    meanGeneration /= numRunsFoundSolution;


    for (int i = 0; i < numHiddenNodesInSolutionsVector.size(); i++)
    {
        meanNumHiddenNodes += (float)numHiddenNodesInSolutionsVector.at(i);
    }

    meanNumHiddenNodes /= numRunsFoundSolution;




    // Now that we have the means, we can figure out the standard deviations..
    float generationSumSquaredDeviations = 0.0;
    float generationFoundStdDev          = 0.0;

    for (int i = 0; i < generationSolutionFoundVector.size(); i++)
    {
        float deviation = generationSolutionFoundVector.at(i) - meanGeneration;
        generationSumSquaredDeviations += (deviation * deviation);
    }

    generationFoundStdDev = sqrt(generationSumSquaredDeviations/(generationSolutionFoundVector.size() - 1));


    float numHiddenNodesSumSquaredDeviations = 0.0;
    float numHiddenNodesStdDev               = 0.0;

    for (int i = 0; i < numHiddenNodesInSolutionsVector.size(); i++)
    {
        float deviation = numHiddenNodesInSolutionsVector.at(i) - meanNumHiddenNodes;
        numHiddenNodesSumSquaredDeviations += (deviation * deviation);
    }

    numHiddenNodesStdDev = sqrt(numHiddenNodesSumSquaredDeviations/(numHiddenNodesInSolutionsVector.size() - 1));


    cout<<"Number of runs: "<<param_numXorTestRuns<<", number of runs that solved XOR problem: "<<numRunsFoundSolution<<endl;
    cout<<"Percentage of success: "<<100.0 * ((float)numRunsFoundSolution/(float)param_numXorTestRuns)<<"%"<<endl;
    cout<<"Average number of generations required to find solution: "<<meanGeneration<<endl;
    cout<<"   Generation found Standard Deviation: "<<generationFoundStdDev<<endl;
    cout<<"Average number of hidden nodes in solution: "<<meanNumHiddenNodes<<endl;
    cout<<"   Number of hidden nodes Standard Deviation: "<<numHiddenNodesStdDev<<endl;


    // Now exit the program
    exit(0);
}





void Cga::setDebugMode(bool newDebugMode)
{
    debugMode = newDebugMode;
}
