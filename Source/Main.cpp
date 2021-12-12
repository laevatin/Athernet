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

int main(int argc, char *argv[])
{
    MessageManager::getInstance();

    if (argc <= 1)
    {
        std::cout << "Usage: ./Athernet-cpp [node-num] [dest-ip]" << std::endl;
        return -1;
    }

    int node = atoi(argv[1]);
    std::cout << "1 for 1->3, 3 for 3->1, 4 for pinging" << std::endl;
    char ctl;
    std::cin >> ctl;

    switch (node)
    {
    case 1:
    {
        ANet athernet("192.168.1.1", "4567", "192.168.1.2", "4568", node);

        switch (ctl)
        {
        case '1':
        {
            std::ifstream inputFile;
            inputFile.open("C:\\Users\\zengs\\Desktop\\homework\\CS120\\Project\\Athernet\\Input\\input.in");
            std::string message;

            while (std::getline(inputFile, message))
            {
                athernet.SendData((const uint8_t *)message.c_str(), message.length() + 1, 2);
            }
            break;
        }
        case '3':
        {
            while (1)
            {
                char buffer[512];
                athernet.RecvData((uint8_t *)buffer, Config::PACKET_PAYLOAD, 2);
                std::cout << "Payload: " << buffer << "\n";
            }
            break;
        }
        case '4':
        {
            while (1)
            {
                athernet.SendPing("182.61.200.6");
                Sleep(1000);
            }
        }
        }

        getchar();
        getchar();
        break;
    }
    case 2:
    {
        ANet athernet("192.168.1.2", "4569", argv[2], "4570", node);
        uint8_t buffer[512];
        int from_node = 1, to_node = 3;
        switch (ctl)
        {
        case '1':
        {   
            while (1)
                athernet.Gateway(1, 3);
            break;
        }
        case '3':
        {
            while (1)
                athernet.Gateway(3, 1);
            break;
        }
        case '4':
        {
            while (1)
            {
                athernet.SendPing("182.61.200.6");
                Sleep(1000);
            }
        }
        }
        
        break;
    }
    case 3:
    {
        ANet athernet("0.0.0.0", "4570", argv[2], "4569", node);

        switch (ctl)
        {
        case '1':
        {
            while (1)
            {
                char buffer[512];
                athernet.RecvData((uint8_t *)buffer, Config::PACKET_PAYLOAD, 2);
                std::cout << "Payload: " << buffer << std::endl;
            }
        }
        case '3':
        {
            std::ifstream inputFile;
            inputFile.open("C:\\Users\\zengs\\Desktop\\homework\\CS120\\Project\\Athernet\\Input\\input.in");
            std::string message;

            while (std::getline(inputFile, message))
            {
                athernet.SendData((const uint8_t *)message.c_str(), message.length() + 1, 2);
            }
        }
        }

        getchar();
        getchar();
        break;
    }
    }

    return 0;
}