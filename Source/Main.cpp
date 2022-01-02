#include "Physical/Audio.h"
#include "Utils/IOFunctions.hpp"
#include "UDP/UDP.h"
#include "ANet/ANet.h"
#include <windows.h>
#include <objbase.h>
#include <bitset>
#include <fstream>
#include <chrono>

extern std::ofstream debug_file;
#define PI acos(-1)
 
void getInputFromFile(Array<uint8_t> &input, const std::string &path) {
    std::ifstream inputFile;
    uint8_t in;
    int idx = 0;

    inputFile.open(path);
    if (!inputFile) {
        std::cerr << "ERROR: Unable to open input file." << newLine;
        exit(1);
    }

    while (inputFile >> in) {
        input.set(idx++, in);
    }

    inputFile.close();
}

int main(int argc, char *argv[]) {
    MessageManager::getInstance();
    std::iostream::sync_with_stdio(false);

    if (argc <= 1) {
        std::cout << "Usage: ./Athernet-cpp [node-num]" << std::endl;
        return -1;
    }

    int node = atoi(argv[1]);
    std::cout << "1 for 1->3, 3 for 3->1, 4 for pinging" << std::endl;
    char ctl;
    std::cin >> ctl;
    strcpy(Config::IP_ETHERNET, "192.168.18.204");

    switch (node) {
        case 1: {
            ANetClient client("192.168.1.2", "4567", true);
            ANetServer server("4566", true);

            switch (ctl) {
                case '1': {
                    std::ifstream inputFile;
                    inputFile.open(
                            R"(C:\Users\16322\Desktop\lessons\2021_1\CS120_Computer_Network\Athernet-cpp\Input\input.in)");
                    std::string message;

                    while (std::getline(inputFile, message)) {
                        client.SendData((const uint8_t *) message.c_str(), message.length() + 1);
                    }
                    break;
                }
                case '4': {
                    while (true)
                        server.ReplyPing();
                }
                default:
                    break;
            }

            getchar();
            getchar();
            break;
        }
        case 2: {
            ANetGateway gateway("4567", "4568");
            gateway.StartForwarding();
            break;
        }
        case 3: {
            switch (ctl) {
                case '1': {
                    ANetServer athernet("4568", false);
                    while (1) {
                        char buffer[512];
                        athernet.RecvData((uint8_t *) buffer, Config::PACKET_PAYLOAD);
                        std::cout << "Payload: " << buffer << std::endl;
                    }
                    break;
                }
                case '3': {
                    ANetClient athernet("10.19.131.103", "4568", false);
                    std::ifstream inputFile;
                    inputFile.open(
                            R"(C:\Users\16322\Desktop\lessons\2021_1\CS120_Computer_Network\Athernet-cpp\Input\input.in)");
                    std::string message;

                    while (std::getline(inputFile, message)) {
                        athernet.SendData((const uint8_t *) message.c_str(), message.length() + 1);
                    }
                    break;
                }
                default:
                    break;
            }

            getchar();
            getchar();
            break;
        }
    }
    return 0;
}