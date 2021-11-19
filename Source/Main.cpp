#include "Physical/Audio.h"
#include "Utils/IOFunctions.hpp"
#include <objbase.h>
#include <bitset>
#include <fstream>


extern std::ofstream debug_file;
#define PI acos(-1)

void getInputFromFile(Array<uint8_t> &input, const std::string &path)
{
    std::ifstream inputFile;
    uint8_t in;
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
    
    Array<uint8_t> input;
    Array<uint8_t> output;
    getInputFromFile(input, "C:\\Users\\16322\\Desktop\\lessons\\2021_1\\CS120_Computer_Network\\Athernet-cpp\\Input\\input10000.in");
    // for (int i = 0; i < input.size(); i++) {
    //     std::cout << (int)input[i];
    //     if (i % (57 * 8) == 0) {
    //         std::cout << "\n";
    //     }
    // }
    // std::cout << "\n";
    input = bitToByte(input);
    std::ofstream outputfile;
    audioIO.write(input);
    while (getchar()) 
    {
        outputfile.open("C:\\Users\\16322\\Desktop\\lessons\\2021_1\\CS120_Computer_Network\\Athernet-cpp\\Input\\output10000.out");
        debug_file.open("C:\\Users\\16322\\Desktop\\lessons\\2021_1\\CS120_Computer_Network\\Athernet-cpp\\Input\\debug.out");

        audioIO.startTransmit();
        audioIO.read(output);
        output = byteToBit(output);
        for (int i = 0; i < output.size(); i++)
            outputfile << (int)output[i];
        outputfile.close();
        debug_file.close();
        audioIO.write(input);
    }

    return 0;
}