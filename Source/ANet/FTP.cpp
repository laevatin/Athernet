#include "ANet/FTP.h"

#include <string>
#include <iostream>
#include <fstream>

FTPGateway::FTPGateway(const std::string& ftp_server)
        : m_client(Config::IP_ATHERNET, Config::PORT_ATHERNET, true),
          m_server(Config::PORT_ATHERNET, true) {

    InitNetwork();
    CLog::SetLevel(LOG_DEBUG | LOG_NETWORK);

    if (!m_clsFtp.Connect(ftp_server.c_str(), 21, true)) {
        std::cout << "FTP: Failed to connect to " << ftp_server << std::endl;
        return;
    }
}

void FTPGateway::Gateway() {
    std::string curCommand;

    while (curCommand != "EXIT") {
        m_server.RecvString(curCommand);
        m_clsFtp.Send("%s", curCommand.c_str());

        std::istringstream ss(curCommand);
        std::string cmd;
        int ret;
        ss >> cmd;

        if (cmd == "PWD") {
            CFtpResponse response;
            std::string pwd;
            ret = m_clsFtp.Recv(response, 257);
            m_client.SendInt(ret);
            if (ret == 257) {
                GetPWD(response, pwd);
            }
            m_client.SendString(pwd);
        }
        else if (cmd == "RETR" || cmd == "LIST") {
            DataTransfer(m_clsFtp.m_strDataIp, m_clsFtp.m_iDataPort);
            ret = m_clsFtp.Recv(1);
            m_client.SendInt(ret);
        } else if (cmd == "PASV") {
            CFtpResponse response;
            ret = m_clsFtp.Recv(response, 1);
            response.GetIpPort(m_clsFtp.m_strDataIp, m_clsFtp.m_iDataPort);
            m_client.SendInt(ret);
        } else {
            ret = m_clsFtp.Recv(1);
            m_client.SendInt(ret);
        }
    }
}

void FTPGateway::DataTransfer(std::string &dataIP, int dataPort) {
    Socket hSocket = TcpConnect(dataIP.c_str(), dataPort, 1000);
    if (hSocket == INVALID_SOCKET) {
        m_client.SendInt(0);
        return;
    }

    int iRead;
    char buf[Config::IP_PACKET_PAYLOAD];

    while (true) {
        iRead = TcpRecv(hSocket, buf, sizeof(buf), 1000);
        if (iRead <= 0) break;

        m_client.SendData((uint8_t *) buf, iRead);
    }

    closesocket(hSocket);
    int finish = m_clsFtp.Recv(1);
    m_client.SendInt(finish);
}

bool FTPGateway::GetPWD(CFtpResponse &response, std::string &path) {
    auto it = response.m_clsReplyList.begin();
    if (it == response.m_clsReplyList.end()) {
        return false;
    }

    const char *pszLine = it->c_str();
    int iLineLen = (int)it->length();
    int iStartPos = -1;

    for (int i = 0; i < iLineLen; ++i) {
        if (pszLine[i] == '"') {
            if (iStartPos == -1) {
                iStartPos = i + 1;
            } else {
                path.append(pszLine + iStartPos, i - iStartPos);
                return true;
            }
        }
    }

    return false;
}

ANetFTP::ANetFTP()
    : m_client(Config::IP_ATHERNET, Config::PORT_ATHERNET, true),
      m_server(Config::PORT_ATHERNET, true) {}

void ANetFTP::run() {
    std::string curCommand;

    while (curCommand != "EXIT") {
        std::cout << "> ";
        std::getline(std::cin, curCommand);
        std::ofstream outfile;
        std::istringstream ss(curCommand);
        std::string cmd;
        ss >> cmd;

        if (cmd == "RETR") {
            std::string arg1;

            ss >> arg1;
            outfile.open(arg1, std::ios::trunc | std::ios::binary);
        }

        if (!checkCmd(cmd)) {
            std::cout << "Invalid command" << std::endl;
            continue;
        }

        m_client.SendString(curCommand);
        if (cmd != "RETR" && cmd != "LIST") {
            int ret = m_server.RecvInt();
            displayCode(ret);
        }

        if (cmd == "PWD") {
            std::string pwd;
            m_server.RecvString(pwd);
            std::cout << pwd << std::endl;
        } else if (cmd == "RETR") {
            while (true) {
                uint8_t buf[Config::IP_PACKET_PAYLOAD];
                int read = m_server.RecvData(buf, Config::IP_PACKET_PAYLOAD);

                if (outfile.is_open()) {
                    outfile.write((char *) buf, read);
                }

                if (read < Config::IP_PACKET_PAYLOAD)
                    break;
            }

            int ret = m_server.RecvInt();
            displayCode(ret);
            ret = m_server.RecvInt();
            displayCode(ret);
        } else if (cmd == "LIST") {
            std::string listStr;
            uint8_t buf[Config::IP_PACKET_PAYLOAD];

            while (true) {
                int read = m_server.RecvData(buf, Config::IP_PACKET_PAYLOAD);

                listStr.append((char*)buf, read);

                if (read < Config::IP_PACKET_PAYLOAD) break;
            }
            std::cout << listStr << std::endl;

            int ret = m_server.RecvInt();
            displayCode(ret);
            ret = m_server.RecvInt();
            displayCode(ret);
        }
    }
}

bool ANetFTP::checkCmd(const std::string &cmd) {
    return cmd == "PWD" || cmd == "RETR" || cmd == "LIST" || cmd == "EXIT" || cmd == "USER" || cmd == "PASS" || cmd == "PASV" || cmd == "CWD";
}

void ANetFTP::displayCode(int retCode) {
    switch (retCode) {
        case 220:
            std::cout << "FTP server ready" << std::endl;
            break;
        case 221:
            std::cout << "FTP server closing connection" << std::endl;
            break;
        case 227:
            std::cout << "Entering Passive Mode" << std::endl;
            break;
        case 230:
            std::cout << "User logged in" << std::endl;
            break;
        case 250:
            std::cout << "Requested file action okay, completed" << std::endl;
            break;
        case 257:
            std::cout << "PWD command successful" << std::endl;
            break;
        case 331:
            std::cout << "User name okay, need password" << std::endl;
            break;
        case 332:
            std::cout << "Need account for login" << std::endl;
            break;
        case 421:
            std::cout << "Service not available, closing control connection" << std::endl;
            break;
        case 425:
            std::cout << "Can't open data connection" << std::endl;
            break;
        case 426:
            std::cout << "Connection closed; transfer aborted" << std::endl;
            break;
        case 450:
            std::cout << "Requested file action not taken" << std::endl;
            break;
        case 451:
            std::cout << "Requested action aborted" << std::endl;
            break;
        case 500:
            std::cout << "Syntax error, command unrecognized" << std::endl;
            break;
        case 501:
            std::cout << "Syntax error in parameters or arguments" << std::endl;
            break;
        case 502:
            std::cout << "Command not implemented" << std::endl;
            break;
        case 503:
            std::cout << "Bad sequence of commands" << std::endl;
            break;
        case 504:
            std::cout << "Command not implemented for that parameter" << std::endl;
            break;
        case 530:
            std::cout << "Not logged in" << std::endl;
            break;
        case 532:
            std::cout << "Need account for storing files" << std::endl;
            break;
        case 550:
            std::cout << "Failed to change directory or open file" << std::endl;
            break;
        default:
            std::cout << "Code " << retCode << std::endl;
            break;
    }
}