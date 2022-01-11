#include "Physical/Audio.h"
#include "Utils/IOFunctions.hpp"
#include "UDP/UDP.h"
#include "ANet/ANet.h"
#include <windows.h>
#include <objbase.h>
#include <bitset>
#include <fstream>
#include <chrono>
#include "ANet/FTP.h"


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

void project3_node1() {
    constexpr char NODE2_IP[] = "192.168.1.2";
    ANetClient client(NODE2_IP, Config::PORT_ATHERNET, true);
    ANetServer server(Config::PORT_ATHERNET, true);

    std::cout << "Athernet Node 1\n";
    std::cout << "Press enter to start";

    getchar();

    std::cout << "Connecting to server..." << std::endl;

    client.SendPing(NODE2_IP);
    std::string input;
    while (true) {
        std::cout << "Athernet> ";
        std::getline(std::cin, input);
        if (input == "exit")
            break;

        if (input.length() >= 5 && input.substr(0, 4) == "ping") {
            client.StartPingReq();
            for (int i = 0; i < Config::PING_NUM; i++) {
                auto ip = input.substr(5);
                client.SendPing(ip.c_str());
            }
            client.StopPingReq();
        } else if (input == "receive") {
            client.StartRecvReq();
            for (int i = 0; i < Config::PING_NUM; i++) {
                server.ReplyPing();
            }
        } else if (input.length() >= 5 && input.substr(0, 4) == "send") {
            std::ifstream inputFile;
            inputFile.open(input.substr(5));
            std::string message;

            while (std::getline(inputFile, message)) {
                client.SendData((const uint8_t*)message.c_str(), message.length() + 1);
            }
        } else {
            std::cout << "Unknown command: " << input << std::endl;
        }
    }
}

void project3_node2() {
    constexpr char NODE1_IP[] = "192.168.1.1";
    std::cout << "Athernet Node 2" << std::endl;

    ANetClient client(NODE1_IP, Config::PORT_ATHERNET, true);
    ANetServer server(Config::PORT_ATHERNET, true);

    std::cout << "Receiving connection..." << std::endl;

    server.ReplyPing();

    ANetGateway gateway(Config::PORT_ATHERNET, Config::PORT_ATHERNET);

    gateway.StartForwarding();
}

void project3(int argc, char* argv[]) {
    int node = atoi(argv[1]);
    strcpy(Config::IP_ETHERNET, "10.20.195.86");

    switch (node) {
        case 1: {
            project3_node1();
            break;
        }
        case 2: {
            project3_node2();
            break;
        }
        case 3: {
/*            switch (ctl) {
                case '1': {
                    ANetServer athernet("4568", false);
                    while (1) {
                        char buffer[512];
                        athernet.RecvData((uint8_t*)buffer, Config::IP_PACKET_PAYLOAD);
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
                        athernet.SendData((const uint8_t*)message.c_str(), message.length() + 1);
                    }
                    break;
                }
                default:
                    break;
            }

            getchar();
            getchar();*/
            break;
        }
    }
}

void project4(int argc, char *argv[]) {
    int node = atoi(argv[1]);
    if (node == 1) {
        std::string addr;
        std::cout << "Input the FTP server address to connect to: ";
        std::cin >> addr;
        ANetClient client(Config::IP_ATHERNET, Config::PORT_ATHERNET, true);
        client.SendString(addr);
        ANetFTP ftp;
        ftp.run();
    } else if (node == 2) {
        std::string addr;
        ANetServer server(Config::PORT_ATHERNET, true);
        server.RecvString(addr);
        FTPGateway ftpGateway(addr);
        ftpGateway.Gateway();
    }
    Sleep(10000);
}

int main(int argc, char *argv[]) {
    MessageManager::getInstance();
    std::iostream::sync_with_stdio(false);

    if (argc <= 1) {
        std::cout << "Usage: ./Athernet-cpp [node-num]" << std::endl;
        return -1;
    }

    project3(argc, argv);

    return 0;
}
