#include "phenotype.h"

// Based on the code from the book AI Techniques for Game Programming
// by Mat Buckland. 
// Modified by Drew Kirkpatrick, drew.kirkpatrick@gmail.com




//------------------------------------Sigmoid function------------------------
//
//----------------------------------------------------------------------------

float Sigmoid(float netinput, float response)
{
    return ( 1 / ( 1 + exp(-netinput / response)));
}




//----------------------------- TidyXSplits -----------------------------
//
//  This is a fix to prevent neurons overlapping when they are displayed
//-----------------------------------------------------------------------
void TidyXSplits(vector<SNeuron*> &neurons)
{
    // stores the index of any neurons with identical splitY values
    vector<int>    SameLevelNeurons;

    // stores all the splitY values already checked
    vector<double> DepthsChecked;


    // for each neuron find all neurons of identical ySplit level
    for (int n=0; n<neurons.size(); ++n)
    {
        double ThisDepth = neurons[n]->dSplitY;

        // check to see if we have already adjusted the neurons at this depth
        bool bAlreadyChecked = false;

        for (int i=0; i<DepthsChecked.size(); ++i)
        {
            if (DepthsChecked[i] == ThisDepth)
            {
                bAlreadyChecked = true;

                break;
            }
        }

        // add this depth to the depths checked.
        DepthsChecked.push_back(ThisDepth);

        // if this depth has not already been adjusted
        if (!bAlreadyChecked)
        {
            // clear this storage and add the neuron's index we are checking
            SameLevelNeurons.clear();
            SameLevelNeurons.push_back(n);

            // find all the neurons with this splitY depth
            for (int i=n+1; i<neurons.size(); ++i)
            {
                if (neurons[i]->dSplitY == ThisDepth)
                {
                    // add the index to this neuron
                    SameLevelNeurons.push_back(i);
                }
            }

            // calculate the distance between each neuron
            double slice = 1.0/(SameLevelNeurons.size()+1);


            // separate all neurons at this level
            for (int i=0; i<SameLevelNeurons.size(); ++i)
            {
                int idx = SameLevelNeurons[i];

                neurons[idx]->dSplitX = (i+1) * slice;
            }
        }
    }// next neuron to check
}







//--------------------------------- constructor ------------------------------
//
//------------------------------------------------------------------------
CNeuralNet::CNeuralNet(vector<SNeuron*> neurons,
                       int              depth,
                       int              genomeID,
                       double           rawFitness):m_vecpNeurons(neurons),
    m_iDepth(depth),
    m_genomeID(genomeID),
    m_dFitness(rawFitness)
{
    m_cloneID = -1;
}





//--------------------------------- destructor ---------------------------------
//
//------------------------------------------------------------------------
CNeuralNet::~CNeuralNet()
{
    // delete any live neurons
    for (int i=0; i<m_vecpNeurons.size(); ++i)
    {
        if (m_vecpNeurons[i])
	{
            delete m_vecpNeurons[i];

            m_vecpNeurons[i] = NULL;
	}
    }
}







//----------------------------------Update--------------------------------
//	takes a list of doubles as inputs into the network then steps through 
//  the neurons calculating each neurons next output.
//
//	finally returns a std::vector of doubles as the output from the net.
//------------------------------------------------------------------------
vector<double> CNeuralNet::Update(const vector<double> &inputs,
                                  const run_type        type)
{
    // if the mode is snapshot then we require all the neurons to be
    // iterated through as many times as the network is deep. If the
    // mode is set to active the method can return an output after
    // just one iteration
    int FlushCount = 0;

    if (type == snapshot)
    {
        FlushCount = m_iDepth;
    }
    else
    {
        FlushCount = 1;
    }

    // iterate through the network FlushCount times
    for (int i=0; i<FlushCount; ++i)
    {
        // clear the output vector
        outputs.clear();

        // this is an index into the current neuron
        int cNeuron = 0;

        // first set the outputs of the 'input' neurons to be equal
        // to the values passed into the function in inputs
        while (m_vecpNeurons[cNeuron]->NeuronType == input)
	{
            m_vecpNeurons[cNeuron]->dOutput = inputs[cNeuron];

            ++cNeuron;
	}

        // set the output of the bias to 1
        m_vecpNeurons[cNeuron++]->dOutput = 1;

        // then we step through the network a neuron at a time
        while (cNeuron < m_vecpNeurons.size())
	{
            // this will hold the sum of all the inputs x weights
            double sum = 0;

            // sum this neuron's inputs by iterating through all the links into
            // the neuron
            for (int lnk=0; lnk<m_vecpNeurons[cNeuron]->vecLinksIn.size(); ++lnk)
	    {
                // get this link's weight
                double Weight = m_vecpNeurons[cNeuron]->vecLinksIn[lnk].dWeight;

                // get the output from the neuron this link is coming from
                double NeuronOutput =
                        m_vecpNeurons[cNeuron]->vecLinksIn[lnk].pIn->dOutput;

                // add to sum
                sum += Weight * NeuronOutput;
	    }


            // now put the sum through the activation function and assign the
            // value to this neuron's output
            m_vecpNeurons[cNeuron]->dOutput =
                    Sigmoid(sum, m_vecpNeurons[cNeuron]->dActivationResponse);

            if (m_vecpNeurons[cNeuron]->NeuronType == output)
	    {
                // add to our outputs
                outputs.push_back(m_vecpNeurons[cNeuron]->dOutput);
	    }

            // next neuron
            ++cNeuron;
	}

    }// next iteration through the network



    // the network needs to be flushed if this type of update is performed
    // otherwise it is possible for dependencies to be built on the order
    // the training data is presented
    if (type == snapshot)
    {
        for (int n=0; n<m_vecpNeurons.size(); ++n)
	{
            m_vecpNeurons[n]->dOutput = 0;
	}
    }

    // return the outputs
    return outputs;
}





// Returns the number of neurons
// in the neural network.
int CNeuralNet::getNumberOfNeurons()
{
    return m_vecpNeurons.size();
}



// Counts the number of connections
// in the neural network. The number
// of links in will be the same number
// of links out. Those added together is twice
// the number of connections in the network,
// so just count the links in
int CNeuralNet::getNumberOfConnections()
{
    int linksIn = 0;

    for (int i = 0; i < m_vecpNeurons.size(); i++)
    {
        linksIn += m_vecpNeurons[i]->vecLinksIn.size();
    }

    return linksIn;
}



// Test all neurons to see if they're of type
// hidden, and count them
int CNeuralNet::getNumberOfHiddenNodes()
{
    int numHiddenNodes = 0;

    for (int i = 0; i < m_vecpNeurons.size(); i++)
    {
        if (m_vecpNeurons[i]->NeuronType == hidden)
        {
            numHiddenNodes++;
        }
    }

    return numHiddenNodes;
}



// Returns the number of recurrent connections
// the neural network has
int CNeuralNet::getNumberOfRecurrentConnections()
{
    int numRecurrentLinks = 0;

    for (int i = 0; i < m_vecpNeurons.size(); i++)
    {
        for (int j = 0; j < m_vecpNeurons[i]->vecLinksIn.size(); j++)
        {
            if (m_vecpNeurons[i]->vecLinksIn[j].bRecurrent)
            {
                numRecurrentLinks++;
            }
        }
    }

    return numRecurrentLinks;
}



// This function returns the genome ID
// for the neural network. This ID number
// is identical to the genome identification
// number of the genome that this neural
// network was created from
int CNeuralNet::getGenomeID()
{
    return m_genomeID;
}




// This function is for setting the Clone
// ID number of the neural network. This
// number is initialized in the constructor
// to -1, but can be changed here in
// order to help differentiate various
// copies of neural networks.
void CNeuralNet::setCloneID(int newCloneID)
{
    m_cloneID = newCloneID;
}




// This function returns the Clone ID
// number the neural network currently has.
// This value is set to -1 in the constructor,
// but can changed with setCloneID
int CNeuralNet::getCloneID()
{
    return m_cloneID;
}



// Returns the rawfitness score of the
// genome that this neural network was
// created from. Not always valid, as
// this value will only be meaningful
// if the genome it's created from has
// already been assigned a score
double CNeuralNet::getRawFitness()
{
    return m_dFitness;
}




// Returns a pointer to the neuron referenced by the neuron index
// Useful for rendering the neural network.
SNeuron* CNeuralNet::getPointerToNeuron(int neuronIndex)
{
    if ((neuronIndex >= m_vecpNeurons.size()) || (neuronIndex < 0))
    {
        cout<<"Error in CNeuralNet::getPointerToNeuron, invalid index: "<<neuronIndex<<endl;
        exit(1);
    }


    return m_vecpNeurons.at(neuronIndex);
}






// Makes sure the positioning data for neurons is correct, should
// be called once before getPointerToNeuron is called for rendering
// purposes
void CNeuralNet::tidyXSPlitsForNeuralNetwork()
{
    TidyXSplits(m_vecpNeurons);
}




