#include "Physical/Audio.h"
#include "Utils/IOFunctions.hpp"
#include "UDP/UDP.h"
#include "NAT/ANet.h"
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

int main(int argc, char* argv[])
{
    MessageManager::getInstance();

    if (argc <= 1)
    {
        std::cout << "Usage: ./Athernet-cpp [node-num] [dest-ip]" << std::endl;
        return -1;
    }

    int node = atoi(argv[1]);
    std::cout << "1 for 1->3, 3 for 3->1" << std::endl;
    char ctl;
    std::cin >> ctl;

    switch (node) 
    {
    case 1:
        {
        ANet athernet("192.168.1.1", "4567", "192.168.1.2", "4568", node);
        
        if (ctl == '1')
        {
            std::ifstream inputFile;
            inputFile.open("C:\\Users\\16322\\Desktop\\lessons\\2021_1\\CS120_Computer_Network\\Athernet-cpp\\Input\\input.in");
            std::string message;

            while (std::getline(inputFile, message)) 
            {
                athernet.SendData((const uint8_t *)message.c_str(), message.length() + 1, 2);
            }
        }
        else 
        {
            char buffer[512];
            athernet.RecvData((uint8_t *)buffer, Config::PACKET_PAYLOAD, 2);
            std::cout << "Payload: " << buffer << "\n";
        }

        getchar();
        getchar();
        break;
        }
    case 2:
        {
        ANet athernet("192.168.1.2", "4569", argv[2], "4570", node);
        uint8_t buffer[512];
        int from_node, to_node;
        if (ctl == '1') 
        {
            from_node = 1;
            to_node = 3;
        }
        else 
        {
            from_node = 3;
            to_node = 1;
        }
        while (1)
        {
            int recv = athernet.RecvData(buffer, Config::PACKET_PAYLOAD, from_node);
            athernet.SendData(buffer, recv, to_node);
        }

        break;
        }
    case 3:
        {
        ANet athernet("0.0.0.0", "4570", argv[2], "4569", node);
        
        if (ctl == '1')
        {
            char buffer[512];
            athernet.RecvData((uint8_t *)buffer, Config::PACKET_PAYLOAD, 2);
            std::cout << "Payload: " << buffer << "\n";
        }
        else 
        {
            std::ifstream inputFile;
            inputFile.open("C:\\Users\\16322\\Desktop\\lessons\\2021_1\\CS120_Computer_Network\\Athernet-cpp\\Input\\input.in");
            std::string message;

            while (std::getline(inputFile, message)) 
            {
                athernet.SendData((const uint8_t *)message.c_str(), message.length() + 1, 2);
            }
        }

        break;
        }
    }

    return 0;
}