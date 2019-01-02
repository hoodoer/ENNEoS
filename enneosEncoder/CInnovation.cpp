#include "CInnovation.h"

// Based on the code from the book AI Techniques for Game Programming
// by Mat Buckland. 
// Modified by Drew Kirkpatrick, drew.kirkpatrick@gmail.com





//---------------------------------- ctor --------------------------------
//
//  given a series of start genes and start neurons this ctor adds
//  all the appropriate innovations.
//------------------------------------------------------------------------
CInnovation::CInnovation(vector<SLinkGene>   start_genes,
                         vector<SNeuronGene> start_neurons)
{
    m_NextNeuronID      = 0;
    m_NextInnovationNum = 0;

    // add the neurons
    for (int nd=0; nd<start_neurons.size(); ++nd)
    {
        m_vecInnovs.push_back(SInnovation(start_neurons[nd],
                                          m_NextInnovationNum++,
                                          m_NextNeuronID++));
    }

    // add the links
    for (int cGen = 0; cGen<start_genes.size(); ++cGen)
    {
        SInnovation NewInnov(start_genes[cGen].FromNeuron,
                             start_genes[cGen].ToNeuron,
                             new_link,
                             m_NextInnovationNum);

        m_vecInnovs.push_back(NewInnov);

        ++m_NextInnovationNum;
    }
}





//---------------------------CheckInnovation------------------------------
//
//	checks to see if this innovation has already occurred. If it has it
//	returns the innovation ID. If not it returns a negative value. 
//------------------------------------------------------------------------
int CInnovation::CheckInnovation(int in, int out, innov_type type)
{
    // iterate through the innovations looking for a match on all
    // three parameters
    for (int inv=0; inv<m_vecInnovs.size(); ++inv)
    {
        if ((m_vecInnovs[inv].NeuronIn == in)   &&
                (m_vecInnovs[inv].NeuronOut == out) &&
                (m_vecInnovs[inv].InnovationType == type))
        {
            // found a match so assign this innovation number to id
            return m_vecInnovs[inv].InnovationID;
        }
    }

    // if no match return a negative value
    return -1;
}





//--------------------------CreateNewInnovation---------------------------
//
//	creates a new innovation and returns its ID
//------------------------------------------------------------------------
int CInnovation::CreateNewInnovation(int in, int out, innov_type type)
{
    SInnovation new_innov(in, out, type, m_NextInnovationNum);

    if (type == new_neuron)
    {
        new_innov.NeuronID = m_NextNeuronID;

        ++m_NextNeuronID;
    }

    m_vecInnovs.push_back(new_innov);

    ++m_NextInnovationNum;

    return (m_NextNeuronID-1);
}





//------------------------------------------------------------------------
//
//  as above but includes adding x/y position of new neuron
//------------------------------------------------------------------------
int CInnovation::CreateNewInnovation(int          from,
                                     int          to,
                                     innov_type   InnovType,
                                     neuron_type  NeuronType,
                                     double       x,
                                     double       y)
{ 

    SInnovation new_innov(from, to, InnovType, m_NextInnovationNum, NeuronType, x, y);

    if (InnovType == new_neuron)
    {
        new_innov.NeuronID = m_NextNeuronID;

        ++m_NextNeuronID;
    }

    m_vecInnovs.push_back(new_innov);

    ++m_NextInnovationNum;

    return (m_NextNeuronID-1);
}






//------------------------------- CreateNeuronFromID -----------------------
//
//  given a neuron ID this function returns a clone of that neuron
//------------------------------------------------------------------------
SNeuronGene CInnovation::CreateNeuronFromID(int NeuronID)
{
    SNeuronGene temp(hidden,0,0,0);

    for (int inv=0; inv<m_vecInnovs.size(); ++inv)
    {
        if (m_vecInnovs[inv].NeuronID == NeuronID)
        {
            temp.NeuronType = m_vecInnovs[inv].NeuronType;
            temp.iID      = m_vecInnovs[inv].NeuronID;
            temp.dSplitY  = m_vecInnovs[inv].dSplitY;
            temp.dSplitX  = m_vecInnovs[inv].dSplitX;

            return temp;
        }
    }

    return temp;
}



