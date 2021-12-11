#include "Physical/Audio.h"
#include "Utils/IOFunctions.hpp"
#include "UDP/UDP.h"
#include <windows.h>
#include <objbase.h>
#include <bitset>
#include <fstream>
#include <chrono>

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
        input.set(idx++, in);
    }

    inputFile.close();
}


/*
-------   ANet  -------  Wifi  -------
|Node1| <------>|Node2|<------>|Node3|
-------         -------        -------
*/

int main(int argc, char* argv[])
{
    MessageManager::getInstance();
    AudioIO audioIO;
    Array<uint8_t> input;
    Array<uint8_t> output;
    std::ifstream inputFile;
    inputFile.open("C:\\Users\\16322\\Desktop\\lessons\\2021_1\\CS120_Computer_Network\\Athernet-cpp\\Input\\input.in");

    // getInputFromFile(input, "C:\\Users\\16322\\Desktop\\lessons\\2021_1\\CS120_Computer_Network\\Athernet-cpp\\Input\\input1000.in");
    std::string message;
    
    // input = bitToByte(input);
    std::ofstream outputfile;
    // audioIO.write(input);

    std::cout << "Initialization finished.\n";
    while (getchar()) 
    {
        char recvbuffer[1024];
        while (std::getline(inputFile, message)) 
        {
            //for (int i = 0; i < message.length(); i++)
                std::cout << message << std::endl;
            auto now1 = std::chrono::system_clock::now();
            audioIO.SendData((const uint8_t *)message.c_str(), message.length() + 1);

            int rec = audioIO.RecvData((uint8_t *)recvbuffer, message.length() + 1);
            auto now2 = std::chrono::system_clock::now();
            std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(now2 - now1).count() << std::endl;
            //for (int i = 0; i < rec; i++)
                std::cout << recvbuffer << std::endl;
            //std::cout << "\n";
        }
        // outputfile.open("C:\\Users\\16322\\Desktop\\lessons\\2021_1\\CS120_Computer_Network\\Athernet-cpp\\Input\\output1000.out");
        // debug_file.open("C:\\Users\\16322\\Desktop\\lessons\\2021_1\\CS120_Computer_Network\\Athernet-cpp\\Input\\debug.out");

        // audioIO.read(output);
        // output = byteToBit(output);
        // for (int i = 0; i < output.size(); i++)
        //     outputfile << (int)output[i];
        // outputfile.close();
        // // debug_file.close();
        // audioIO.write(input);
    }

    // if (argc <= 2) {
    //     return -1;
    // }

    // if (!strcmp(argv[1], "send"))
    // {
    //     char data[] = "hello world!";
    //     UDPClient client("10.19.126.236", "1000");
    //     while (true)
    //     {
    //         client.SendData(data, sizeof(data));
    //         Sleep(1000);
    //     }
    // }
    // else {
    //     char buffer[1024];
    //     UDPServer server("1000");
    //     while (true)
    //     {
    //         server.RecvData(buffer, 1024);
    //         std::cout << buffer << std::endl;
    //     }
    // }


    return 0;
}