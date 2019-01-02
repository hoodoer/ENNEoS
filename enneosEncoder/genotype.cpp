#include "genotype.h"


// Based on the code from the book AI Techniques for Game Programming
// by Mat Buckland. 
// Modified by Drew Kirkpatrick, drew.kirkpatrick@gmail.com

//------------------------------------------------------------------------
//
//  default constructor
//------------------------------------------------------------------------
CGenome::CGenome():m_pPhenotype(NULL),
    m_GenomeID(0),
    m_dFitness(0),
    m_dAdjustedFitness(0),
    m_iNumInputs(0),
    m_iNumOutPuts(0),
    m_dAmountToSpawn(0),
    m_iNetDepth(-1)
{

}






//-----------------------------constructor--------------------------------
//	this constructor creates a minimal genome where there are output +
//	input neurons and each input neuron is connected to each output neuron.
//------------------------------------------------------------------------
CGenome::CGenome(int id, int inputs, int outputs):m_pPhenotype(NULL),
    m_GenomeID(id),
    m_dFitness(0),
    m_dAdjustedFitness(0),
    m_iNumInputs(inputs),
    m_iNumOutPuts(outputs),
    m_dAmountToSpawn(0),
    m_iSpecies(0),
    m_iNetDepth(-1)

{
    // create the input neurons
    double InputRowSlice = 1/(double)(inputs+2);

    for (int i=0; i<inputs; i++)
    {
        m_vecNeurons.push_back(SNeuronGene(input, i, 0, (i+2)*InputRowSlice));
    }

    // create the bias
    m_vecNeurons.push_back(SNeuronGene(bias, inputs, 0, InputRowSlice));

    // create the output neurons
    double OutputRowSlice = 1/(double)(outputs+1);

    for (int i=0; i<outputs; i++)
    {
        m_vecNeurons.push_back(SNeuronGene(output, i+inputs+1, 1, (i+1)*OutputRowSlice));
    }

    // create the link genes, connect each input neuron to each output neuron and
    // assign a random weight -1 < w < 1
    for (int i=0; i<inputs+1; i++)
    {
        for (int j=0; j<outputs; j++)
        {
            m_vecLinks.push_back(SLinkGene(m_vecNeurons[i].iID,
                                           m_vecNeurons[inputs+j+1].iID,
                                 true,
                                 inputs+outputs+1+NumGenes(),
                                 RandomClamped()));
        }
    }
}








//------------------------------------------------------------------------
//
//  this constructor creates a genome from a vector of SLinkGenes, a 
//  vector of SNeuronGenes and an ID number.
//------------------------------------------------------------------------
CGenome::CGenome(int                 id,
                 vector<SNeuronGene> neurons,
                 vector<SLinkGene>   genes,
                 int                 inputs,
                 int                 outputs):m_GenomeID(id),
    m_pPhenotype(NULL),
    m_vecLinks(genes),
    m_vecNeurons(neurons),
    m_dAmountToSpawn(0),
    m_dFitness(0),
    m_dAdjustedFitness(0),
    m_iNumInputs(inputs),
    m_iNumOutPuts(outputs)

{

}








//-------------------------------destructor-----------------------------------------------------
//
//----------------------------------------------------------------------------------------
CGenome::~CGenome()
{  
    if (m_pPhenotype)
    {
        delete m_pPhenotype;

        m_pPhenotype = NULL;
    }
}








//---------------------------------copy constructor--------------------------------------
//
//---------------------------------------------------------------------------------------
CGenome::CGenome(const CGenome& g)
{
    m_GenomeID         = g.m_GenomeID;
    m_vecNeurons       = g.m_vecNeurons;
    m_vecLinks         = g.m_vecLinks;
    m_pPhenotype       = NULL;              //no need to perform a deep copy
    m_dFitness         = g.m_dFitness;
    m_dAdjustedFitness = g.m_dAdjustedFitness;
    m_iNumInputs       = g.m_iNumInputs;
    m_iNumOutPuts      = g.m_iNumOutPuts;
    m_dAmountToSpawn   = g.m_dAmountToSpawn;
}







//---------------------------------assignment operator-----------------------------------
//
//----------------------------------------------------------------------------------------
CGenome& CGenome::operator =(const CGenome& g)
{
    // self assignment guard
    if (this != &g)
    {
        m_GenomeID         = g.m_GenomeID;
        m_vecNeurons       = g.m_vecNeurons;
        m_vecLinks         = g.m_vecLinks;
        m_pPhenotype       = NULL;        //no need to perform a deep copy
        m_dFitness         = g.m_dFitness;
        m_dAdjustedFitness = g.m_dAdjustedFitness;
        m_iNumInputs       = g.m_iNumInputs;
        m_iNumOutPuts      = g.m_iNumOutPuts;
        m_dAmountToSpawn   = g.m_dAmountToSpawn;
    }

    return *this;
}





//-------------------------------CreatePhenotype--------------------------
//
//	Creates a neural network based upon the information in the genome.
//	Returns a pointer to the newly created ANN
//------------------------------------------------------------------------
CNeuralNet* CGenome::CreatePhenotype()
{
    // first make sure there is no existing phenotype for this genome
    DeletePhenotype();

    // this will hold all the neurons required for the phenotype
    vector<SNeuron*>  vecNeurons;

    // first, create all the required neurons
    for (unsigned int i=0; i<m_vecNeurons.size(); i++)
    {
        SNeuron* pNeuron = new SNeuron(m_vecNeurons[i].NeuronType,
                                       m_vecNeurons[i].iID,
                                       m_vecNeurons[i].dSplitY,
                                       m_vecNeurons[i].dSplitX,
                                       m_vecNeurons[i].dActivationResponse);

        vecNeurons.push_back(pNeuron);
    }



    // now to create the links.
    for (unsigned int cGene=0; cGene<m_vecLinks.size(); ++cGene)
    {
        // make sure the link gene is enabled before the connection is created
        if (m_vecLinks[cGene].bEnabled)
        {
            // get the pointers to the relevant neurons
            int element         = GetElementPos(m_vecLinks[cGene].FromNeuron);
            SNeuron* FromNeuron = vecNeurons[element];

            element           = GetElementPos(m_vecLinks[cGene].ToNeuron);
            SNeuron* ToNeuron = vecNeurons[element];

            // create a link between those two neurons and assign the weight stored
            // in the gene
            SLink tmpLink(m_vecLinks[cGene].dWeight,
                          FromNeuron,
                          ToNeuron,
                          m_vecLinks[cGene].bRecurrent);

            // add new links to neuron
            FromNeuron->vecLinksOut.push_back(tmpLink);
            ToNeuron->vecLinksIn.push_back(tmpLink);
        }
    }

    // now the neurons contain all the connectivity information, a neural
    // network may be created from them.
    m_pPhenotype = new CNeuralNet(vecNeurons, m_iNetDepth, m_GenomeID, m_dFitness);

    return m_pPhenotype;
}





//--------------------------- DeletePhenotype ----------------------------
//
//------------------------------------------------------------------------
void CGenome::DeletePhenotype()
{
    if (m_pPhenotype)
    {
        delete m_pPhenotype;
    }

    m_pPhenotype = NULL;
}






//---------------------------- GetElementPos -----------------------------
//
//	given a neuron ID this little function just finds its position in 
//  m_vecNeurons
//------------------------------------------------------------------------
int CGenome::GetElementPos(int neuron_id)
{
    for (unsigned int i=0; i<m_vecNeurons.size(); i++)
    {
        if (m_vecNeurons[i].iID == neuron_id)
        {
            return i;
        }
    }

    cout<<"Error, CGenome::GetElementPos, Shouldn't be here! Looking for NeuronID: "<<neuron_id<<endl;

    cout<<"Dump of all neurons, num neurons: "<<m_vecNeurons.size()<<endl;
    for (unsigned int i = 0; i < m_vecNeurons.size(); i++)
    {
        cout<<"Neuron index: "<<i<<", id: "<<m_vecNeurons[i].iID<<" type: ";
        switch (m_vecNeurons[i].NeuronType)
        {
        case input: cout<<"Input"<<endl; break;
        case hidden: cout<<"Hidden"<<endl; break;
        case output: cout<<"Output"<<endl; break;
        case bias: cout<<"Bias"<<endl; break;
        default:
            cout<<"Invalid type"<<endl;
        }
    }

    exit(1);

    return -1;
}




//------------------------------DuplicateLink-----------------------------
//
// returns true if the link is already part of the genome
//------------------------------------------------------------------------
bool CGenome::DuplicateLink(int NeuronIn, int NeuronOut)
{
    for (unsigned int cGene = 0; cGene < m_vecLinks.size(); ++cGene)
    {
        if ((m_vecLinks[cGene].FromNeuron == NeuronIn) &&
                (m_vecLinks[cGene].ToNeuron == NeuronOut))
        {
            //we already have this link
            return true;
        }
    }

    return false;
}





//--------------------------------AddLink---------------------------------
//
// create a new link with the probability of CParams::dChanceAddLink
//------------------------------------------------------------------------
void CGenome::AddLink(double       MutationRate,
                      double       ChanceOfLooped,
                      CInnovation  &innovation,
                      int          NumTrysToFindLoop,
                      int          NumTrysToAddLink)
{

    // just return dependent on the mutation rate
    if (RandDouble() > MutationRate)
    {
        return;
    }

    // define holders for the two neurons to be linked. If we have find two
    // valid neurons to link these values will become >= 0.
    int ID_neuron1 = -1;
    int ID_neuron2 = -1;

    // flag set if a recurrent link is selected (looped or normal)
    bool bRecurrent = false;

    // first test to see if an attempt should be made to create a
    // link that loops back into the same neuron
    if (RandDouble() < ChanceOfLooped)
    {
        // YES: try NumTrysToFindLoop times to find a neuron that is not an
        // input or bias neuron and that does not already have a loopback
        // connection
        while(NumTrysToFindLoop--)
        {
            // grab a random neuron
            int NeuronPos = RandInt(m_iNumInputs+1, m_vecNeurons.size()-1);

            // check to make sure the neuron does not already have a loopback
            // link and that it is not an input or bias neuron
            if (!m_vecNeurons[NeuronPos].bRecurrent &&
                    (m_vecNeurons[NeuronPos].NeuronType != bias) &&
                    (m_vecNeurons[NeuronPos].NeuronType != input))
            {
                ID_neuron1 = ID_neuron2 = m_vecNeurons[NeuronPos].iID;

                m_vecNeurons[NeuronPos].bRecurrent = true;

                bRecurrent = true;

                NumTrysToFindLoop = 0;
            }
        }
    }
    else
    {
        // No: try to find two unlinked neurons. Make NumTrysToAddLink
        // attempts
        while(NumTrysToAddLink--)
        {
            // choose two neurons, the second must not be an input or a bias
            ID_neuron1 = m_vecNeurons[RandInt(0, m_vecNeurons.size()-1)].iID;

            ID_neuron2 =
                    m_vecNeurons[RandInt(m_iNumInputs+1, m_vecNeurons.size()-1)].iID;

            // make sure these two are not already linked and that they are
            // not the same neuron
            if (DuplicateLink(ID_neuron1, ID_neuron2) ||
                    (ID_neuron1 == ID_neuron2))
            {
                ID_neuron1 = -1;
                ID_neuron2 = -1;
            }
            else
            {
                NumTrysToAddLink = 0;
            }
        }
    }

    // return if unsuccessful in finding a link
    if ( (ID_neuron1 < 0) || (ID_neuron2 < 0) )
    {
        return;
    }

    // check to see if we have already created this innovation
    int id;
    id = innovation.CheckInnovation(ID_neuron1, ID_neuron2, new_link);

    // is this link recurrent?
    if (m_vecNeurons[GetElementPos(ID_neuron1)].dSplitY >
            m_vecNeurons[GetElementPos(ID_neuron2)].dSplitY)
    {
        if (ChanceOfLooped > 0.0)
        {
            // If we have 0 chance of recurrent links,
            // we can't do this.
            bRecurrent = true;
        }
    }

    if ( id < 0)
    {
        // we need to create a new innovation
        innovation.CreateNewInnovation(ID_neuron1, ID_neuron2, new_link);

        // then create the new gene
        int id = innovation.NextNumber() - 1;

        SLinkGene NewGene(ID_neuron1,
                          ID_neuron2,
                          true,
                          id,
                          RandomClamped(),
                          bRecurrent);

        m_vecLinks.push_back(NewGene);
    }
    else
    {
        // the innovation has already been created so all we need to
        // do is create the new gene using the existing innovation ID
        SLinkGene NewGene(ID_neuron1,
                          ID_neuron2,
                          true,
                          id,
                          RandomClamped(),
                          bRecurrent);

        m_vecLinks.push_back(NewGene);
    }

    return;
}






//---------------------------------AddNeuron------------------------------
//
//	this function adds a neuron to the genotype by examining the network, 
//	splitting one of the links and inserting the new neuron.
//------------------------------------------------------------------------
void CGenome::AddNeuron(double       MutationRate,
                        CInnovation &innovations,
                        int          NumTrysToFindOldLink)
{
    // just return dependent on mutation rate
    if (RandDouble() > MutationRate)
    {
        return;
    }

    // if a valid link is found into which to insert the new neuron
    // this value is set to true.
    bool bDone = false;

    // this will hold the index into m_vecLinks of the chosen link gene
    int  ChosenLink = 0;

    // first a link is chosen to split. If the genome is small the code makes
    // sure one of the older links is split to ensure a chaining effect does
    // not occur. Here, if the genome contains less than 5 hidden neurons it
    // is considered to be too small to select a link at random
    const int SizeThreshold = m_iNumInputs + m_iNumOutPuts + 5;

    if (NumNeurons() < SizeThreshold)
    {
        while(NumTrysToFindOldLink--)
        {
            // choose a link with a bias towards the older links in the genome
            ChosenLink = RandInt(0, NumGenes()-1-(int)sqrt(NumGenes()));

            // make sure the link is enabled and that it is not a recurrent link
            // or has a bias input. Also make sure it's not a link between
            // two output neurons, as this would put the hidden
            // neuron on the same "level" of the output neurons,
            // and screws up the network depth calculations.
            int FromNeuron = m_vecLinks[ChosenLink].FromNeuron;
            int ToNeuron   = m_vecLinks[ChosenLink].ToNeuron;


            if ((m_vecLinks[ChosenLink].bEnabled)    &&
                    (!m_vecLinks[ChosenLink].bRecurrent) &&
                    (m_vecNeurons[GetElementPos(FromNeuron)].NeuronType != bias))
            {
                // Check to make sure that the fromNeuron AND toNeuron are not
                // both output neurons. One is fine, but not both
                if ((m_vecNeurons[GetElementPos(FromNeuron)].NeuronType != output) ||
                        (m_vecNeurons[GetElementPos(ToNeuron)].NeuronType != output))
                {
                    bDone = true;
                    NumTrysToFindOldLink = 0;
                }
            }
        }

        if (!bDone)
        {
            // failed to find a decent link
            return;
        }
    }
    else
    {
        // the genome is of sufficient size for any link to be acceptable
        while (!bDone)
        {
            ChosenLink = RandInt(0, NumGenes()-1);

            // make sure the link is enabled and that it is not a recurrent link
            // or has a BIAS input. Also make sure it's not a link between
            // two output neurons, as this would put the hidden
            // neuron on the same "level" of the output neurons,
            // and screws up the network depth calculations.
            int FromNeuron = m_vecLinks[ChosenLink].FromNeuron;
            int ToNeuron   = m_vecLinks[ChosenLink].ToNeuron;

            if ((m_vecLinks[ChosenLink].bEnabled) &&
                    (!m_vecLinks[ChosenLink].bRecurrent) &&
                    (m_vecNeurons[GetElementPos(FromNeuron)].NeuronType != bias))
            {
                // Check to make sure that the fromNeuron AND toNeuron are not
                // both output neurons. One is fine, but not both
                if ((m_vecNeurons[GetElementPos(FromNeuron)].NeuronType != output) ||
                        (m_vecNeurons[GetElementPos(ToNeuron)].NeuronType != output))
                {
                    bDone = true;
                }
            }
        }
    }

    // disable this gene
    m_vecLinks[ChosenLink].bEnabled = false;

    // grab the weight from the gene (we want to use this for the weight of
    // one of the new links so that the split does not disturb anything the
    // NN may have already learned...
    double OriginalWeight = m_vecLinks[ChosenLink].dWeight;

    // identify the neurons this link connects
    int from =  m_vecLinks[ChosenLink].FromNeuron;
    int to   =  m_vecLinks[ChosenLink].ToNeuron;



    // Error check, make sure the neuron isn't being put in a link between
    // two output neurons
    if ((m_vecNeurons[GetElementPos(from)].NeuronType == output) &&
            (m_vecNeurons[GetElementPos(to)].NeuronType == output))
    {
        cout<<"Error in CGenome::AddNeuron, trying to add a neuron in a link between two output neurons!"<<endl;
        exit(1);
    }


    // calculate the depth and width of the new neuron. We can use the depth
    // to see if the link feeds backwards or forwards
    double NewDepth = (m_vecNeurons[GetElementPos(from)].dSplitY +
            m_vecNeurons[GetElementPos(to)].dSplitY) /2;

    double NewWidth = (m_vecNeurons[GetElementPos(from)].dSplitX +
            m_vecNeurons[GetElementPos(to)].dSplitX) /2;


    // Error check, this function is for adding hidden nodes.
    // No hidden node should have a depth of 1.0, which is
    // the layer for output nodes.
    if (NewDepth == 1.0)
    {
        cout<<"Error in CGenome::AddNeuron, just calculated a depth (splitY) of 1.0 for a hidden node. Impossible!"<<endl;
        exit(1);
    }

    // Now to see if this innovation has been created previously by
    // another member of the population
    int id = innovations.CheckInnovation(from,
                                         to,
                                         new_neuron);



    /*it is possible for NEAT to repeatedly do the following:

  1. Find a link. Lets say we choose link 1 to 5
  2. Disable the link,
  3. Add a new neuron and two new links
  4. The link disabled in Step 2 maybe re-enabled when this genome
  is recombined with a genome that has that link enabled.
  5  etc etc

  Therefore, this function must check to see if a neuron ID is already
  being used. If it is then the function creates a new innovation
  for the neuron. */
    if (id >= 0)
    {
        int NeuronID = innovations.GetNeuronID(id);

        if (AlreadyHaveThisNeuronID(NeuronID))
        {
            id = -1;
        }
    }

    if (id < 0)
    {
        // add the innovation for the new neuron
        int NewNeuronID = innovations.CreateNewInnovation(from,
                                                          to,
                                                          new_neuron,
                                                          hidden,
                                                          NewWidth,
                                                          NewDepth);

        // create the new neuron gene and add it.
        m_vecNeurons.push_back(SNeuronGene(hidden,
                                           NewNeuronID,
                                           NewDepth,
                                           NewWidth));

        if (NewNeuronID == 0)
        {
            cout<<"~~~ Adding new hidden neuron with ID: "<<NewNeuronID<<endl;
            cout<<"~~~~~~ Depth: "<<NewDepth<<endl;
            cout<<"~~~~~~ Width: "<<NewWidth<<endl<<endl;
            exit(1);
        }

        // Two new link innovations are required, one for each of the
        // new links created when this gene is split.

        //------first link

        // get the next innovation ID
        int idLink1 = innovations.NextNumber();

        // create the new innovation
        innovations.CreateNewInnovation(from,
                                        NewNeuronID,
                                        new_link);

        // create the new link gene
        SLinkGene link1(from,
                        NewNeuronID,
                        true,
                        idLink1,
                        1.0);

        m_vecLinks.push_back(link1);

        //----------second link

        // get the next innovation ID
        int idLink2 = innovations.NextNumber();

        // create the new innovation
        innovations.CreateNewInnovation(NewNeuronID,
                                        to,
                                        new_link);

        // create the new gene
        SLinkGene link2(NewNeuronID,
                        to,
                        true,
                        idLink2,
                        OriginalWeight);

        m_vecLinks.push_back(link2);
    }
    else
    {
        // this innovation has already been created so grab the relevant neuron
        // and link info from the innovation database
        int NewNeuronID = innovations.GetNeuronID(id);

        // get the innovation IDs for the two new link genes.
        int idLink1 = innovations.CheckInnovation(from, NewNeuronID, new_link);
        int idLink2 = innovations.CheckInnovation(NewNeuronID, to, new_link);

        // this should never happen because the innovations *should* have already
        // occurred
        if ( (idLink1 < 0) || (idLink2 < 0) )
        {
            cout<<"ERROR, CGenome::AddNeuron, Shouldn't be here!"<<endl;

            return;
        }

        // now we need to create 2 new genes to represent the new links
        SLinkGene link1(from, NewNeuronID, true, idLink1, 1.0);
        SLinkGene link2(NewNeuronID, to, true, idLink2, OriginalWeight);

        m_vecLinks.push_back(link1);
        m_vecLinks.push_back(link2);

        // create the new neuron
        SNeuronGene NewNeuron(hidden, NewNeuronID, NewDepth, NewWidth);

        // and add it
        m_vecNeurons.push_back(NewNeuron);
        if (NewNeuronID == 0)
        {
            cout<<"~~~ Adding new hidden neuron with ID: "<<NewNeuronID<<endl;
            cout<<"~~~~~~ Depth: "<<NewDepth<<endl;
            cout<<"~~~~~~ Width: "<<NewWidth<<endl<<endl;
            exit(1);
        }
    }

    return;
}







//--------------------------- AlreadyHaveThisNeuronID ----------------------
// 
// tests to see if the parameter is equal to any existing neuron ID's. 
// Returns true if this is the case.
//------------------------------------------------------------------------
bool CGenome::AlreadyHaveThisNeuronID(const int ID)
{
    for (unsigned int n=0; n<m_vecNeurons.size(); ++n)
    {
        if (ID == m_vecNeurons[n].iID)
        {
            return true;
        }
    }

    return false;
}







//------------------------------- MutateWeights---------------------------
//	Iterates through the genes and purturbs the weights given a 
//  probability mut_rate.
//
//	prob_new_mut is the chance that a weight may get replaced by a
//  completely new weight.
//
//	dMaxPertubation is the maximum perturbation to be applied.
//------------------------------------------------------------------------
void CGenome::MutateWeights(double mut_rate,
                            double prob_new_mut,
                            double MaxPertubation)
{
    for (unsigned int cGen=0; cGen<m_vecLinks.size(); ++cGen)
    {
        // do we mutate this gene?
        if (RandDouble() < mut_rate)
        {
            // do we change the weight to a completely new weight?
            if (RandDouble() < prob_new_mut)
            {
                // change the weight using the random distribtion defined by 'type'
                m_vecLinks[cGen].dWeight = RandomClamped();
            }
            else
            {
                // perturb the weight
                m_vecLinks[cGen].dWeight += RandomClamped() * MaxPertubation;
            }
        }
    }

    return;
}





void CGenome::MutateActivationResponse(double mut_rate,
                                       double MaxPertubation)
{
    for (unsigned int cGen=0; cGen<m_vecNeurons.size(); ++cGen)
    {
        if (RandDouble() < mut_rate)
        {
            m_vecNeurons[cGen].dActivationResponse += RandomClamped() * MaxPertubation;
        }
    }
}





//------------------------- GetCompatibilityScore ------------------------
//
//  this function returns a score based on the compatibility of this
//  genome with the passed genome
//------------------------------------------------------------------------
double CGenome::GetCompatibilityScore(const CGenome &genome)
{
    // travel down the length of each genome counting the number of
    // disjoint genes, the number of excess genes and the number of
    // matched genes
    double	NumDisjoint = 0;
    double	NumExcess   = 0;
    double	NumMatched  = 0;

    // this records the summed difference of weights in matched genes
    double	WeightDifference = 0;

    // position holders for each genome. They are incremented as we
    // step down each genomes length.
    unsigned int g1 = 0;
    unsigned int g2 = 0;

    while ( (g1 < m_vecLinks.size()-1) || (g2 < genome.m_vecLinks.size()-1) )
    {
        // we've reached the end of genome1 but not genome2 so increment
        // the excess score
        if (g1 == m_vecLinks.size()-1)
        {
            ++g2;
            ++NumExcess;

            continue;
        }

        // and vice versa
        if (g2 == genome.m_vecLinks.size()-1)
        {
            ++g1;
            ++NumExcess;

            continue;
        }

        // get innovation numbers for each gene at this point
        int id1 = m_vecLinks[g1].InnovationID;
        int id2 = genome.m_vecLinks[g2].InnovationID;

        // innovation numbers are identical so increase the matched score
        if (id1 == id2)
        {
            ++g1;
            ++g2;
            ++NumMatched;

            // get the weight difference between these two genes
            WeightDifference += fabs(m_vecLinks[g1].dWeight - genome.m_vecLinks[g2].dWeight);
        }

        // innovation numbers are different so increment the disjoint score
        if (id1 < id2)
        {
            ++NumDisjoint;
            ++g1;
        }

        if (id1 > id2)
        {
            ++NumDisjoint;
            ++g2;
        }
    }

    // get the length of the longest genome
    int longest = genome.NumGenes();

    if (NumGenes() > longest)
    {
        longest = NumGenes();
    }


    // mMatched replaced with a parameter, as you might
    // want to change it for particular setups. Larger mMatched
    // values would probably want to be used for very
    // large populations.

    // finally calculate the scores
    double score = (param_mExcess * NumExcess/(double)longest) +
            (param_mDisjoint * NumDisjoint/(double)longest) +
            (param_mMatched * WeightDifference / NumMatched);

    
    return score;
}






//--------------------------- SortGenes ----------------------------------
//
//  does exactly that
//------------------------------------------------------------------------
void CGenome::SortGenes()
{
    sort (m_vecLinks.begin(), m_vecLinks.end());
}







//--------------------------- Tokenize ----------------------------------
//
//  Tokenizes a string based on a delimiter. Used for processing
//  text files, such as saved genome dna files
//------------------------------------------------------------------------
void CGenome::Tokenize(const string &str, vector<string> &tokens, const string &delimiters)
{
    // Skip delimiters at beginning.
    string::size_type lastPos = str.find_first_not_of(delimiters, 0);

    // Find first "non-delimiter".
    string::size_type pos     = str.find_first_of(delimiters, lastPos);

    while (string::npos != pos || string::npos != lastPos)
    {
        // Found a token, add it to the vector.
        tokens.push_back(str.substr(lastPos, pos - lastPos));

        // Skip delimiters.  Note the "not_of"
        lastPos = str.find_first_not_of(delimiters, pos);

        // Find next "non-delimiter"
        pos = str.find_first_of(delimiters, lastPos);
    }
}



//--------------------------- WriteGenomeToFile --------------------------
//
//  Saves this genome to a file for later use.
//  The file will have the extension .dna
//  Returns true if it worked, false if it failed
//------------------------------------------------------------------------
bool CGenome::WriteGenomeToFile(string fileName)
{
    const string genomeFileExtension = ".dna";
    ofstream     genomeFileStream;

    string fullGenomeFileName = fileName + genomeFileExtension;
    genomeFileStream.open(fullGenomeFileName.c_str(), ios::out);

    if (genomeFileStream.fail())
    {
        cout<<"CGenome::WriteGenomeToFile, Error opening genome dna file "<<fullGenomeFileName<<" for writing"<<endl;
        return false;
    }


    // Increase the precision for doubles with setprecision call
    genomeFileStream<<setprecision(numeric_limits<double>::digits10 + 2);
    //    genomeFileStream<<setprecision(50);

    // Write non-vector data
    genomeFileStream<<"GenomeID: "        <<m_GenomeID         <<endl;



    // Fitness and amountToSpawn values don't make any sense when loading
    // a genome back into different software. Just set these to zero, so
    // only the relevant values are written. This way md5sums will work
    // to double check if genomes are being stored in memory correctly
    //    genomeFileStream<<"RawFitness: 0"      <<endl;
    //    genomeFileStream<<"AdjustedFitness: 0" <<endl;
    //    genomeFileStream<<"AmountToSpawn: 0"   <<endl;
    //    genomeFileStream<<"RawFitness: "      <<m_dFitness         <<endl;
    //    genomeFileStream<<"AdjustedFitness: " <<m_dAdjustedFitness <<endl;
    //    genomeFileStream<<"AmountToSpawn: "   <<m_dAmountToSpawn   <<endl;


    genomeFileStream<<"NumInputs: "       <<m_iNumInputs       <<endl;
    genomeFileStream<<"NumOutputs: "      <<m_iNumOutPuts      <<endl;
    genomeFileStream<<"NetDepth: "        <<m_iNetDepth        <<endl;

    // Species index doesn't make sense when loading this back later, and isn't always
    // set if a genome isn't in the working population. Just set it to 0, this way
    // checksums will make sense.
    //    genomeFileStream<<"Species: 0" <<endl;
    //    genomeFileStream<<"Species: "         <<m_iSpecies         <<endl;

    // Write out the neuron gene data...
    genomeFileStream<<"NumNeuronGenes: "<<m_vecNeurons.size()<<endl;

    for (unsigned int i = 0; i < m_vecNeurons.size(); i++)
    {
        genomeFileStream<<"NeuronGeneID: "<<m_vecNeurons[i].iID<<endl;

        genomeFileStream<<"NeuronGeneType: ";
        switch (m_vecNeurons[i].NeuronType)
        {
        case input:
            genomeFileStream<<"Input"<<endl;
            break;

        case hidden:
            genomeFileStream<<"Hidden"<<endl;
            break;

        case output:
            genomeFileStream<<"Output"<<endl;
            break;

        case bias:
            genomeFileStream<<"Bias"<<endl;
            break;

        default:
            cout<<"CGenome::WriteGenomeToFile, invalid NeuronType"<<endl;
            genomeFileStream.close();
            return false;
        }

        genomeFileStream<<"Recurrent: ";
        if (m_vecNeurons[i].bRecurrent)
        {
            genomeFileStream<<"True"<<endl;
        }
        else
        {
            genomeFileStream<<"False"<<endl;
        }

        genomeFileStream<<"ActivationResponse: " <<m_vecNeurons[i].dActivationResponse <<endl;
        genomeFileStream<<"SplitX: "             <<m_vecNeurons[i].dSplitX             <<endl;
        genomeFileStream<<"SplitY: "             <<m_vecNeurons[i].dSplitY             <<endl;
    }



    // Write out the link gene data
    genomeFileStream<<"NumLinkGenes: "<<m_vecLinks.size()<<endl;

    for (unsigned int i = 0; i < m_vecLinks.size(); i++)
    {
        genomeFileStream<<"LinkGeneInnovationID: " <<m_vecLinks[i].InnovationID <<endl;
        genomeFileStream<<"FromNeuronID: "         <<m_vecLinks[i].FromNeuron   <<endl;
        genomeFileStream<<"ToNeuronID: "           <<m_vecLinks[i].ToNeuron     <<endl;
        genomeFileStream<<"Weight: "               <<m_vecLinks[i].dWeight      <<endl;

        genomeFileStream<<"Enabled: ";
        if (m_vecLinks[i].bEnabled)
        {
            genomeFileStream<<"True"<<endl;
        }
        else
        {
            genomeFileStream<<"False"<<endl;
        }

        genomeFileStream<<"Recurrent: ";
        if (m_vecLinks[i].bRecurrent)
        {
            genomeFileStream<<"True"<<endl;
        }
        else
        {
            genomeFileStream<<"False"<<endl;
        }
    }

    genomeFileStream.close();
    return true;
}








//--------------------------- ReadGenomeFromFile --------------------------
//
//  Reads the genome data from a saved text file
//  Returns true if it worked, or false if it failed
//------------------------------------------------------------------------
bool CGenome::ReadGenomeFromFile(string fileName)
{
    ifstream       genomeFileStream;
    string         inBuffer;
    vector<string> tokens;

    // Number of genes, used for looped reading
    int numNeuronGenes = 0;
    int numLinkGenes   = 0;


    genomeFileStream.open(fileName.c_str(), ios::in);

    if (genomeFileStream.fail())
    {
        cout<<"CGenome::ReadGenomeFromFile, Error opening genome dna file "<<fileName<<" for reading"<<endl;
        return false;
    }


    // Clear the current neuron and link genes
    m_vecNeurons.clear();
    m_vecLinks.clear();



    // Read in the genome ID
    getline(genomeFileStream, inBuffer);
    Tokenize(inBuffer, tokens, ": ");
    m_GenomeID = atoi(tokens.at(1).c_str());
    tokens.clear();

    // Read in the raw fitness score
    // Removed from dna files
    m_dFitness = 0.0;


    // Read in the adjusted fitness score
    // Removed from dna files
    m_dAdjustedFitness = 0.0;


    // Read in the amount to spawn
    // Removed from dna files
    m_dAmountToSpawn = 0.0;


    // Read in the number of inputs
    getline(genomeFileStream, inBuffer);
    Tokenize(inBuffer, tokens, ": ");
    m_iNumInputs = atoi(tokens.at(1).c_str());
    tokens.clear();


    // Read in the number of outputs
    getline(genomeFileStream, inBuffer);
    Tokenize(inBuffer, tokens, ": ");
    m_iNumOutPuts = atoi(tokens.at(1).c_str());
    tokens.clear();


    // Read in the depth of the genome network
    getline(genomeFileStream, inBuffer);
    Tokenize(inBuffer, tokens, ": ");
    m_iNetDepth = atoi(tokens.at(1).c_str());
    tokens.clear();


    // Read in the species
    // Removed from dna files
    m_iSpecies = 0;


    // Get the number of neuron genes
    getline(genomeFileStream, inBuffer);
    Tokenize(inBuffer, tokens, ": ");
    numNeuronGenes = atoi(tokens.at(1).c_str());
    tokens.clear();

    // Looped read of all neuron gene data
    for (int i = 0; i < numNeuronGenes; i++)
    {
        int         geneID;
        neuron_type neuronType;
        bool        recurrent;
        double      activationResponse;
        double      splitX;
        double      splitY;
        string      comparisonString;

        // Get the neuron gene identification
        getline(genomeFileStream, inBuffer);
        Tokenize(inBuffer, tokens, ": ");
        geneID = atoi(tokens.at(1).c_str());
        tokens.clear();


        // Get the neuron gene type
        getline(genomeFileStream, inBuffer);
        Tokenize(inBuffer, tokens, ": ");
        comparisonString = tokens.at(1);
        tokens.clear();

        if (!strcmp(comparisonString.c_str(), "Input"))
        {
            neuronType = input;
        }
        else if (!strcmp(comparisonString.c_str(), "Hidden"))
        {
            neuronType = hidden;
        }
        else if (!strcmp(comparisonString.c_str(), "Output"))
        {
            neuronType = output;
        }
        else if (!strcmp(comparisonString.c_str(), "Bias"))
        {
            neuronType = bias;
        }
        else
        {
            cout<<"CGenome::ReadGenomeFromFile, Error reading genome dna file "<<fileName<<endl;
            genomeFileStream.close();
            return false;
        }



        // Get whether or not the neuron is recurrent
        getline(genomeFileStream, inBuffer);
        Tokenize(inBuffer, tokens, ": ");
        comparisonString = tokens.at(1);
        tokens.clear();

        if (!strcmp(comparisonString.c_str(), "True"))
        {
            recurrent = true;
        }
        else if (!strcmp(comparisonString.c_str(), "False"))
        {
            recurrent = false;
        }
        else
        {
            cout<<"CGenome::ReadGenomeFromFile, Error reading genome dna file "<<fileName<<endl;
            genomeFileStream.close();
            return false;
        }


        // Get the activation response
        getline(genomeFileStream, inBuffer);
        Tokenize(inBuffer, tokens, ": ");
        activationResponse = atof(tokens.at(1).c_str());
        tokens.clear();


        // Get the split X value
        getline(genomeFileStream, inBuffer);
        Tokenize(inBuffer, tokens, ": ");
        splitX = atof(tokens.at(1).c_str());
        tokens.clear();

        // Get the split Y value
        getline(genomeFileStream, inBuffer);
        Tokenize(inBuffer, tokens, ": ");
        splitY = atof(tokens.at(1).c_str());
        tokens.clear();


        // Now, create the neuron gene from this data
        SNeuronGene neuronGene(neuronType, geneID, splitY, splitX, recurrent);
        neuronGene.dActivationResponse = activationResponse;
        m_vecNeurons.push_back(neuronGene);
    }


    // Get the number of link genes
    getline(genomeFileStream, inBuffer);
    Tokenize(inBuffer, tokens, ": ");
    numLinkGenes = atoi(tokens.at(1).c_str());
    tokens.clear();

    // Looped read of all link gene data
    for (int i = 0; i < numLinkGenes; i++)
    {
        int    innovationID;
        int    fromNeuron;
        int    toNeuron;
        double weight;
        bool   enabled;
        bool   recurrent;
        string comparisonString;

        // Get the link gene innovation id number
        getline(genomeFileStream, inBuffer);
        Tokenize(inBuffer, tokens, ": ");
        innovationID = atoi(tokens.at(1).c_str());
        tokens.clear();


        // Get the from neuron id number
        getline(genomeFileStream, inBuffer);
        Tokenize(inBuffer, tokens, ": ");
        fromNeuron = atoi(tokens.at(1).c_str());
        tokens.clear();


        // Get the to neuron id number
        getline(genomeFileStream, inBuffer);
        Tokenize(inBuffer, tokens, ": ");
        toNeuron = atoi(tokens.at(1).c_str());
        tokens.clear();


        // Get the weight of the link gene
        getline(genomeFileStream, inBuffer);
        Tokenize(inBuffer, tokens, ": ");
        weight = atof(tokens.at(1).c_str());
        tokens.clear();


        // Get whether or not the link is enabled
        getline(genomeFileStream, inBuffer);
        Tokenize(inBuffer, tokens, ": ");
        comparisonString = tokens.at(1);
        tokens.clear();

        if (!strcmp(comparisonString.c_str(), "True"))
        {
            enabled = true;
        }
        else if (!strcmp(comparisonString.c_str(), "False"))
        {
            enabled = false;
        }
        else
        {
            cout<<"CGenome::ReadGenomeFromFile, Error reading genome dna file "<<fileName<<endl;
            genomeFileStream.close();
            return false;
        }


        // Get whether or not the link is recurrent
        getline(genomeFileStream, inBuffer);
        Tokenize(inBuffer, tokens, ": ");
        comparisonString = tokens.at(1);
        tokens.clear();

        if (!strcmp(comparisonString.c_str(), "True"))
        {
            recurrent = true;
        }
        else if (!strcmp(comparisonString.c_str(), "False"))
        {
            recurrent = false;
        }
        else
        {
            cout<<"CGenome::ReadGenomeFromFile, Error reading genome dna file "<<fileName<<endl;
            genomeFileStream.close();
            return false;
        }

        // Now, create the link gene from this data
        SLinkGene linkGene(fromNeuron, toNeuron, enabled, innovationID, weight, recurrent);
        m_vecLinks.push_back(linkGene);
    }


    genomeFileStream.close();
    return true;
}



