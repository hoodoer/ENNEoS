#ifndef PHENOTYPE_H
#define PHENOTYPE_H

// Based on the code from the book AI Techniques for Game Programming
// by Mat Buckland. 
// Modified by Drew Kirkpatrick, drew.kirkpatrick@gmail.com


#include <vector>
#include <math.h>
#include <algorithm>

#include "utils.h"
#include "genes.h"


using namespace std;



struct SNeuron;

//------------------------------------------------------------------------
//
//  SLink structure 
//------------------------------------------------------------------------
struct SLink
{
    // the connection weight
    double  dWeight;

    // pointers to the neurons this link connects
    SNeuron*  pIn;
    SNeuron*  pOut;

    // is this link a recurrent link?
    bool    bRecurrent;

    SLink(double dW, SNeuron* pIn, SNeuron* pOut, bool bRec):
        dWeight(dW),
        pIn(pIn),
        pOut(pOut),
        bRecurrent(bRec)
    {}
};





//------------------------------------------------------------------------
//
//  SNeuron
//------------------------------------------------------------------------
struct SNeuron
{
    // what type of neuron is this?
    neuron_type   NeuronType;

    // its identification number
    int           iNeuronID;

    // sum of weights x inputs
    double        dSumActivation;

    // the output from this neuron
    double        dOutput;

    // used in visualization of the phenotype
    double        dSplitY, dSplitX;

    // sets the curvature of the sigmoid function
    double        dActivationResponse;

    // all the links coming into this neuron
    vector<SLink> vecLinksIn;

    // and out
    vector<SLink> vecLinksOut;



    //--- constructortor
    SNeuron(neuron_type type,
            int         id,
            double      y,
            double      x,
            double      ActResponse):
        NeuronType(type),
        iNeuronID(id),
        dSumActivation(0),
        dOutput(0),
        dSplitY(y),
        dSplitX(x),
        dActivationResponse(ActResponse)
    {}
};





//------------------------------------------------------------------------
//
// CNeuralNet
//------------------------------------------------------------------------
class CNeuralNet
{

private:
    vector<SNeuron*>  m_vecpNeurons;

    // the depth of the network
    int               m_iDepth;

    // The genome ID that this
    // neural network was
    // created from
    int               m_genomeID;

    // The raw fitness score
    // of the genome that this
    // neural network was created
    // from.
    double m_dFitness;

    // While all copies of a neural
    // net will share the same genome
    // ID of genome they were created from,
    // the Clone ID can be set to help
    // to help differentiate the copies
    // for debugging or whatever.
    // It's initialized to -1, but
    // can be set by a public member function
    int               m_cloneID;


    vector <double>   outputs;


public:
    CNeuralNet(vector<SNeuron*> neurons,
               int              depth,
               int              genomeID,
               double           rawFitness);

    ~CNeuralNet();



    // you have to select one of these types when updating the network
    // If snapshot is chosen the network depth is used to completely
    // flush the inputs through the network. active just updates the
    // network each timestep
    enum run_type{snapshot, active};

    // update network for this clock cycle
    vector<double>  Update(const vector<double> &inputs, const run_type type);

    int    getNumberOfNeurons();
    int    getNumberOfConnections();
    int    getNumberOfHiddenNodes();
    int    getNumberOfRecurrentConnections();

    int    getGenomeID();
    void   setCloneID(int newCloneID);
    int    getCloneID();

    double getRawFitness();

    // This function is useful for rendering the neural network
    SNeuron* getPointerToNeuron(int neuronIndex);

    // If you're going to use the getPointerToNeuron function
    // for setting up neural network rendering, make sure you call
    // this function once before starting rendering
    void tidyXSPlitsForNeuralNetwork();
};


#endif
