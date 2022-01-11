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

void project2_node1() {
    AudioIO audioIO;
    std::cout << "Project 2 Node 1" << newLine;
    std::cout << "Press any key to continue..." << newLine;

    getchar();
    std::string input;

    while (true) {
        std::cout << "Athernet> ";
        std::getline(std::cin, input);
        if (input == "exit")
            break;

        if (input.length() >= 5 && input.substr(0, 4) == "send") {
            std::ifstream inputFile;
            inputFile.open(input.substr(5));
            Array<uint8_t> data;
            getInputFromFile(data, input.substr(5));
            data = bitToByte(data);
            std::cout << "Sending " << data.size() << " bits" << newLine;
            int size = data.size();
            audioIO.SendData((uint8_t *)&size, sizeof(int));
            for (int i = 0; i < data.size(); i += Config::MACDATA_PER_FRAME) {
                int len = min(Config::MACDATA_PER_FRAME, data.size() - i);
                audioIO.SendData(data.getRawDataPointer() + i, len);
            }
            inputFile.close();
        } else {
            std::cout << "Unknown command: " << input << std::endl;
        }
    }


}

void project2_node2() {
    std::cout << "Project 2 Node 2" << newLine;
    std::cout << "Press any key to continue..." << newLine;

    AudioIO audioIO;
    getchar();

    while (true) {
        std::ofstream outputFile;
        std::string file_path;
        std::cout << "Input your output file path: ";
        std::getline(std::cin, file_path);

        outputFile.open(file_path);
        if (!outputFile) {
            std::cout << "ERROR: Unable to open output file." << newLine;
            continue;
        }

        if (file_path == "exit")
            break;

        uint8_t buffer[Config::MACDATA_PER_FRAME];

        audioIO.RecvData(buffer, sizeof(int));
        int size = *(int *)buffer;
        std::cout << "Start receiving " << size << " bits" << newLine;

        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < size; i += Config::MACDATA_PER_FRAME) {
            int len = min(Config::MACDATA_PER_FRAME, size - i);
            audioIO.RecvData(buffer, len);
            outputFile.write((char *)buffer, len);
        }
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = end - start;

        std::cout << "Received " << size << " bits in "
                  << std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count()
                  << " seconds" << newLine;

        outputFile.close();
    }
}

void project2(int argc, char *argv[]) {
    int node = atoi(argv[1]);
    switch (node) {
        case 1:
            project2_node1();
            break;
        case 2:
            project2_node2();
            break;
        default:
            std::cout << "Invalid node number." << newLine;
            break;
    }
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
