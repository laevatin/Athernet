#pragma once

#ifndef __UDP_H__
#define __UDP_H__

#include <winsock2.h>

class UDP
{
protected:
	UDP(const char* port);
	WSADATA m_wsaData;
	SOCKET m_socket;
	sockaddr_in m_sockaddr;
};

class UDPServer : protected UDP
{
public:
	UDPServer(const char* port);
	void RecvData(char* out, int outlen);

private:
	sockaddr_in m_sockaddr_remote;

};

class UDPClient : protected UDP
{
public:
	UDPClient(const char* ip, const char* port);
	void SendData(const char* data, int len);
};

#endif