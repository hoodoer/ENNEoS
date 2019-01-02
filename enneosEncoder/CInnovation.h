#ifndef CINNOVATION_H
#define CINNOVATION_H

// Based on the code from the book AI Techniques for Game Programming
// by Mat Buckland. 
// Modified by Drew Kirkpatrick, drew.kirkpatrick@gmail.com



#include <vector>
#include <algorithm>

#include "utils.h"
#include "genotype.h"
#include "phenotype.h"

using namespace std;

struct SLinkGene;



//---------------------Innovation related structs/classes----------------
//
//------------------------------------------------------------------------
enum innov_type
{
  new_neuron,
  new_link
};

//------------------------------------------------------------------------
//
//  structure defining an innovation
//------------------------------------------------------------------------
struct	SInnovation
{
  // new neuron or new link?
  innov_type  InnovationType;

  int         InnovationID;
	
  int         NeuronIn;
  int         NeuronOut;

  int         NeuronID;

  neuron_type NeuronType;

  // if the innovation is a neuron we need to keep a record
  // of its position in the tree (for display purposes)
  double      dSplitY, dSplitX;

  SInnovation(int        in,
              int        out, 
              innov_type t, 
              int        inov_id):NeuronIn(in),
                                  NeuronOut(out),
                                  InnovationType(t),
                                  InnovationID(inov_id),
                                  NeuronID(0),
                                  dSplitX(0),
                                  dSplitY(0),
                                  NeuronType(none)
  {}

  SInnovation(SNeuronGene neuron,
              int          innov_id,
              int          neuron_id):InnovationID(innov_id),
                                      NeuronID(neuron_id),
                                      dSplitX(neuron.dSplitX),
                                      dSplitY(neuron.dSplitY),
                                      NeuronType(neuron.NeuronType),
                                      NeuronIn(-1),
                                      NeuronOut(-1)
  {}
  
  SInnovation(int         in,
              int         out, 
              innov_type  t, 
              int         inov_id,
              neuron_type type,
              double      x,
              double      y):NeuronIn(in),
			     NeuronOut(out),
			     InnovationType(t),
			     InnovationID(inov_id),
			     NeuronID(0),
			     NeuronType(type),
			     dSplitX(x),
			     dSplitY(y)
  {}
};

//------------------------------------------------------------------------
//
//  CInnovation class used to keep track of all innovations created during
//  the populations evolution
//------------------------------------------------------------------------
class CInnovation
{

 private:
	
  vector<SInnovation> m_vecInnovs;

  int                 m_NextNeuronID;

  int                 m_NextInnovationNum;


 public:

  CInnovation(vector<SLinkGene>   start_genes,
              vector<SNeuronGene> start_neurons);
		
  // checks to see if this innovation has already occurred. If it has it
  // returns the innovation ID. If not it returns a negative value.
  int CheckInnovation(int in, int out, innov_type type);

  // creates a new innovation and returns its ID
  int CreateNewInnovation(int in, int out, innov_type type);

  // as above but includes adding x/y position of new neuron
  int CreateNewInnovation(int         from,
			  int         to,
			  innov_type  InnovType,
			  neuron_type NeuronType,
			  double      x,
			  double      y);

  // creates a BasicNeuron from the given neuron ID
  SNeuronGene CreateNeuronFromID(int id);


  //-------------accessor methods
  int GetNeuronID(int inv)const{return m_vecInnovs[inv].NeuronID;}

  void Flush(){m_vecInnovs.clear(); return;}
  
  int NextNumber(int num = 0)
  {
    m_NextInnovationNum += num;

    return m_NextInnovationNum;
  } 
};



#endif
