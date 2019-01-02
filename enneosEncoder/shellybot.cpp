#include "shellybot.h"

ShellyBot::ShellyBot()
{
    zeBrain = NULL;
}



bool ShellyBot::update()
{
    nativeBrainOutputs.clear();

    nativeBrainOutputs = zeBrain->Update(nativeBrainInputs, CNeuralNet::active);
}


void ShellyBot::insertNewBrain(CNeuralNet* newBrain)
{
    zeBrain = newBrain;
}


// Takes a string, converts it to a double representation
void ShellyBot::setBrainInputs(string newInput)
{
    // Clear out last inputs
    nativeBrainInputs.clear();

    // Do a quick conversion
    for (unsigned int i = 0; i < newInput.size(); i++)
    {
        nativeBrainInputs.push_back((double)newInput.at(i));
    }
}



void ShellyBot::getBrainCharOutputs(vector<unsigned char> &newOutputs)
{
    newOutputs.clear();
    int hexVal;


    // Convert doubles to hex
    // Neural nets output 0.0->1.0
    for (unsigned int i = 0; i < nativeBrainOutputs.size(); i++)
    {
        hexVal = (int)(nativeBrainOutputs.at(i) * 255.0);
        newOutputs.push_back((unsigned char)hexVal);
    }

    return;
}

void ShellyBot::getBrainRawOutputs(vector<double> &rawOutputs)
{
    rawOutputs.clear();


    // Convert doubles to hex
    // Neural nets output 0.0->1.0
    for (unsigned int i = 0; i < nativeBrainOutputs.size(); i++)
    {
        rawOutputs.push_back(nativeBrainOutputs.at(i));
    }

    return;
}



// Get the number of neurons in the brain
int ShellyBot::getNumberOfBrainNeurons()
{
    if (zeBrain == NULL)
    {
        cout<<"Error in ShellyBrain::getNumberOfBrainNeurons, zeBrain pointer is NULL!"<<endl;
        return -1;
    }
    else
    {
        return zeBrain->getNumberOfNeurons();
    }
}


// Get the number of connections in the brain
int ShellyBot::getNumberOfBrainConnections()
{
    if (zeBrain == NULL)
    {
        cout<<"Error in ShellyBrain::getNumberOfBrainConnections, zeBrain pointer is NULL!"<<endl;
        return -1;
    }
    else
    {
        return zeBrain->getNumberOfConnections();
    }
}

// Get the number of hidden nodes in the brain
int ShellyBot::getNumberOfBrainHiddenNodes()
{
    if (zeBrain == NULL)
    {
        cout<<"Error in ShellyBrain::getNumberOfBrainHiddenNodes, zeBrain pointer is NULL!"<<endl;
        return -1;
    }
    else
    {
        return zeBrain->getNumberOfHiddenNodes();
    }
}

// Get the brain genome ID number
int ShellyBot::getBrainGenomeID()
{
    if (zeBrain == NULL)
    {
        cout<<"Error in ShellyBrain::getBrainGenomeID, zeBrain pointer is NULL!"<<endl;
        return -5;
    }
    else
    {
        return zeBrain->getGenomeID();
    }
}

// Get the brain clone ID number
int ShellyBot::getBrainCloneID()
{
    if (zeBrain == NULL)
    {
        cout<<"Error in ShellyBrain::getBrainCloneID, zeBrain pointer is NULL!"<<endl;
        return -5;
    }
    else
    {
        return zeBrain->getCloneID();
    }
}
