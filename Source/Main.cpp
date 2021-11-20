#include "Physical/Audio.h"
#include "Utils/IOFunctions.hpp"
#include <objbase.h>
#include <bitset>
#include <fstream>
#include <chrono>
#include <iostream>
#include <string>

extern std::ofstream debug_file;
#define PI acos(-1)
std::string debugFile = "C:\\Users\\zengs\\Desktop\\homework\\CS120\\Project\\Athernet\\Input\\debug.out";
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
	std::string Ifile;
	std::string Ofile;
	std::string command;
    Array<uint8_t> input;
    Array<uint8_t> output;

	//std::cout << "Please enter the type of this device.(Sender:1 Receiver:2 Both:3)"<<newLine;
	std::cout << "Please enter the address of input file.\n";
	std::getline(std::cin, Ifile);
	std::cout << "Input file:\n" << Ifile << newLine;

	std::cout << "Please enter the address of output file.\n";
	std::getline(std::cin, Ofile);
	std::cout << "Output file:\n" << Ofile << newLine;

	std::cout << "Please enter command\n";
	std::getline(std::cin, command);
	if (command.find("macping ") == 0) {
		command.erase(0, 8);
		std::cout << command << std::endl;
	}
	else if (command.find("macperf ") == 0) {
		command.erase(0, 8);
		std::cout << command << std::endl;
	}
	
    getInputFromFile(input, Ifile);
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

    std::cout << "Initialization finished.\n";
    while (getchar()) 
    {
        outputfile.open(Ofile);
        debug_file.open(debugFile);

        auto now1 = std::chrono::system_clock::now();
        audioIO.startTransmit();
        auto now2 = std::chrono::system_clock::now();

        std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(now2 - now1).count() << std::endl;
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