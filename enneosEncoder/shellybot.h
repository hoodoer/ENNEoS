#ifndef SHELLYBRAIN_H
#define SHELLYBRAIN_H

#include <iomanip>
#include <vector>
#include <math.h>

#include "phenotype.h"
#include "parameters.h"

// A bot wrapper class around the neural network that acts as
// and interface to convert real world inputs into the
// double inputs of the neural net, and take the double
// outputs of the neural network and put them into a form
// you can actually use.
//
// Drew Kirkpatrick
// drew.kirkpatrick@gmail.com
// NopSec

class ShellyBot
{
private:
    CNeuralNet* zeBrain;

    // Incoming stimuls before conversion
    string brainStringInput;

    // Converted output (shellcode)
    vector<unsigned char> convertedBrainOutput;


    // Phenotype inputs/outputs doubles
    // will need to do some conversions
    vector<double> nativeBrainInputs;
    vector<double> nativeBrainOutputs;


public:
    ShellyBot();

    // One brain cycle.
    bool update();

    void insertNewBrain(CNeuralNet* newBrain);
    void setBrainInputs(string newInput);
    void getBrainCharOutputs(vector<unsigned char> &newOutputs);
    void getBrainRawOutputs(vector<double> &rawOutputs);

    // Get the number of neurons in the brain
    int getNumberOfBrainNeurons();

    // Get the number of connectsions in the brain
    int getNumberOfBrainConnections();

    // Get the number of hidden nodes in the brain
    int getNumberOfBrainHiddenNodes();

    // Get the brain genome ID number
    int getBrainGenomeID();

    // Get the brain clone ID number
    int getBrainCloneID();
};

#endif // SHELLYBRAIN_H
