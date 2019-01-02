#ifndef GENES_H
#define GENES_H

// Based on the code from the book AI Techniques for Game Programming
// by Mat Buckland. 
// Modified by Drew Kirkpatrick, drew.kirkpatrick@gmail.com


class CNeuron;

//------------------------------------------------------------------------
//
//  enumeration for all the available neuron types
//------------------------------------------------------------------------
enum neuron_type
{
    input,
    hidden,
    output,
    bias,
    none
};



//------------------------------------------------------------------------
//
//  this structure defines a neuron gene
//------------------------------------------------------------------------
struct SNeuronGene
{  
    // its identification
    int iID;

    // The neuron type. Can be input, hidden, bias, output,
    // or none. None is used in innovations.
    neuron_type NeuronType;

    // Whether or not it's recurrent.
    // A recurrent neuron in NEAT has a connection that loops back
    // on itself.
    bool bRecurrent;


    // position in network grid
    // Can be useful for visualization of the network.
    // Also useful for calculating network depth.
    double dSplitY, dSplitX;

    // sets the curvature of the sigmoid function
    double dActivationResponse;

    SNeuronGene(neuron_type type,
                int         id,
                double      y,
                double      x,
                bool        r = false):iID(id),
        NeuronType(type),
        bRecurrent(r),
        dSplitY(y),
        dSplitX(x),
        dActivationResponse(1)
    {}
};



//------------------------------------------------------------------------
//
//  this structure defines a link gene
//------------------------------------------------------------------------
struct SLinkGene
{
    // flag to indicate if this link is currently enabled or not
    bool bEnabled;

    int InnovationID;

    // the IDs of the two neurons this link connects
    int FromNeuron, ToNeuron;

    double dWeight;


    // flag to indicate if this link is recurrent or not
    bool bRecurrent;



    SLinkGene(){}

    SLinkGene(int    in,
              int    out,
              bool   enable,
              int    tag,
              double w,
              bool   rec = false):
        bEnabled(enable),
        InnovationID(tag),
        FromNeuron(in),
        ToNeuron(out),
        dWeight(w),
        bRecurrent(rec)
    {}

    //overload '<' used for sorting(we use the innovation ID as the criteria)
    friend bool operator<(const SLinkGene& lhs, const SLinkGene& rhs)
    {
        return (lhs.InnovationID < rhs.InnovationID);
    }
};



#endif 
