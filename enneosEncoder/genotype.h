#ifndef NEATGENOTYPE_H
#define NEATGENOTYPE_H

// Based on the code from the book AI Techniques for Game Programming
// by Mat Buckland. 
// Modified by Drew Kirkpatrick, drew.kirkpatrick@gmail.com

#include <vector>
#include <fstream>
#include <iomanip>
#include <string.h>
#include <limits>

#include "phenotype.h"
#include "utils.h"
#include "CInnovation.h"
#include "genes.h"
#include "parameters.h"

using namespace std;


class Cga;
class CInnovation;


//------------------------------------------------------------------------
//
// CGenome class definition. A genome basically consists of a vector of
// link genes, a vector of neuron genes and a fitness score.
//------------------------------------------------------------------------
class CGenome 
{

private:

    // its identification number
    int m_GenomeID;

    // all the neurons which make up this genome
    vector<SNeuronGene> m_vecNeurons;

    // and all the the links
    vector<SLinkGene> m_vecLinks;

    // pointer to its phenotype
    CNeuralNet* m_pPhenotype;

    // the depth of this genomes network
    // Set by default to be -1. Needs
    // to be calculated and set before
    // creating a neural network
    // function for calculating depth
    // is part of the Cga class
    int m_iNetDepth;

    // its raw fitness score
    double m_dFitness;

    // its fitness score after it has been placed into a
    // species and adjusted accordingly
    double m_dAdjustedFitness;

    // the number of offspring this individual is required to spawn
    // for the next generation
    double m_dAmountToSpawn;

    // keep a record of the number of inputs and outputs
    int m_iNumInputs, m_iNumOutPuts;

    // keeps a track of which species this genome is in (only used
    // for display purposes)
    int m_iSpecies;

    // returns true if the specified link is already part of the genome
    bool DuplicateLink(int NeuronIn, int NeuronOut);

    // given a neuron id this function just finds its position in
    // m_vecNeurons
    int GetElementPos(int neuron_id);

    // tests if the passed ID is the same as any existing neuron IDs. Used
    // in AddNeuron
    bool AlreadyHaveThisNeuronID(const int ID);


    // This function is for tokenizing a string, used
    // for processing saved genome dna text files
    void Tokenize(const string& str, vector<string>& tokens,
                  const string& delimiters);


public:

    CGenome();

    // this constructor creates a minimal genome where there are output &
    // input neurons and every input neuron is connected to each output neuron
    CGenome(int id, int inputs, int outputs);

    // this constructor creates a genome from a vector of SLinkGenes
    // a vector of SNeuronGenes and an ID number
    CGenome(int                 id,
            vector<SNeuronGene> neurons,
            vector<SLinkGene>   genes,
            int                 inputs,
            int                 outputs);

    ~CGenome();

    // copy constructor
    CGenome(const CGenome& g);

    // assignment operator
    CGenome& operator =(const CGenome& g);

    // create a neural network from the genome
    CNeuralNet* CreatePhenotype();

    // delete the neural network
    void DeletePhenotype();

    // add a link to the genome dependent upon the mutation rate
    void AddLink(double      MutationRate,
                 double      ChanceOfRecurrent,
                 CInnovation &innovation,
                 int         NumTrysToFindLoop,
                 int         NumTrysToAddLink);

    // and a neuron
    void AddNeuron(double      MutationRate,
                   CInnovation &innovation,
                   int         NumTrysToFindOldLink);

    // this function mutates the connection weights
    void MutateWeights(double  mut_rate,
                       double  prob_new_mut,
                       double  dMaxPertubation);

    // perturbs the activation responses of the neurons
    void MutateActivationResponse(double mut_rate,
                                  double MaxPertubation);

    // calculates the compatibility score between this genome and
    // another genome
    double GetCompatibilityScore(const CGenome &genome);

    void SortGenes();

    // overload '<' used for sorting. From fittest to poorest.
    friend bool operator<(const CGenome& lhs, const CGenome& rhs)
    {
        return (lhs.m_dFitness > rhs.m_dFitness);
    }


    // ------accessor methods
    int	    ID()const{return m_GenomeID;}
    void    SetID(const int val){m_GenomeID = val;}

    int     Depth(){return m_iNetDepth;}
    void    SetDepth(int val){m_iNetDepth = val;}

    int     NumGenes()const{return m_vecLinks.size();}
    int     NumNeurons()const{return m_vecNeurons.size();}
    int     NumInputs()const{return m_iNumInputs;}
    int     NumOutputs()const{return m_iNumOutPuts;}

    double  AmountToSpawn()const{return m_dAmountToSpawn;}
    void    SetAmountToSpawn(double num){m_dAmountToSpawn = num;}

    void    SetFitness(const double num){m_dFitness = num;}
    void    SetAdjFitness(const double num){m_dAdjustedFitness = num;}
    double  Fitness()const{return m_dFitness;}
    double  GetAdjFitness()const{return m_dAdjustedFitness;}

    int     GetSpecies()const{return m_iSpecies;}
    void    SetSpecies(int spc){m_iSpecies = spc;}

    double  SplitY(const int val)const{return m_vecNeurons[val].dSplitY;}

    vector<SLinkGene>   Genes()const{return m_vecLinks;}
    vector<SNeuronGene> Neurons()const{return m_vecNeurons;}

    vector<SLinkGene>::iterator StartOfGenes(){return m_vecLinks.begin();}
    vector<SLinkGene>::iterator EndOfGenes(){return m_vecLinks.end();}


    // File operations, save genome to a file, or
    // read a genome back in from a file
    bool    WriteGenomeToFile(string fileName);
    bool    ReadGenomeFromFile(string fileName);
};



#endif
