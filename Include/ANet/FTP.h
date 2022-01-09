#pragma once

#ifndef __FTP_H__
#define __FTP_H__

#include <map>
#include <functional>
#include "FtpClient.h"
#include "Directory.h"
#include "Log.h"
#include "ANet/ANet.h"

class FTPGateway {
public:
    FTPGateway(const std::string& ftp_server);

    void Gateway();

    ~FTPGateway() = default;

private:
    bool FTPGateway::GetPWD(CFtpResponse &response, std::string &path);

    void DataTransfer();

    ANetClient m_client;
    ANetServer m_server;
    CFtpClient m_clsFtp;
    std::string m_ftpServer;
};

class ANetFTP {
public:
    ANetFTP();
    ~ANetFTP() = default;

    void run();

private:

    static bool checkCmd(const std::string &cmd);

    static void displayCode(int retCode);

    ANetClient m_client;
    ANetServer m_server;
};

#endif
