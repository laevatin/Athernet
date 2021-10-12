/*
  ==============================================================================
    This file contains the basic startup code for a JUCE application.
  ==============================================================================
*/

#include "Audio.h"
#include <objbase.h>
#include <bitset>
#include <fstream>

#define PI acos(-1)

void getInputFromFile(Array<int8_t> &input, const std::string &path)
{
    std::ifstream inputFile;
    int8_t in;
    int idx = 0;

    inputFile.open(path);
    if (!inputFile)
    {
        std::cerr << "ERROR: Unable to open input file." << newLine;
        exit(1);
    }
    
    while (inputFile >> in) 
    {
        input.set(idx++, in - '0');
    }

    inputFile.close();
}

//==============================================================================
int main(int argc, char* argv[])
{
    MessageManager::getInstance();
    AudioIO audioIO;
    
    Array<int8_t> input;
    getInputFromFile(input, "C:\\Users\\16322\\Desktop\\lessons\\2021_1\\CS120_Computer_Network\\Athernet-cpp\\Source\\input.in");
    for (auto elem : input)
        std::cout << (int)elem << " ";
    std::cout << newLine;

    audioIO.write(input);
    while (getchar()) 
    {
        //std::cout << "output: ";
        //for (int i = 0; i < input.size(); i++)
        //    std::cout << audioIO.getOutput(i) << " ";
        //std::cout << newLine;
        audioIO.startTransmit();
    }

    return 0;
}