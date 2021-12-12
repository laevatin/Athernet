#pragma once

#ifndef __UDP_H__
#define __UDP_H__

#include <winsock2.h>
#include <JuceHeader.h>

typedef unsigned char uint8_t;

class UDP
{
public:
	UDP(const UDP&) = delete;
    UDP& operator=(const UDP&) = delete;
	
protected:
	UDP(const char* port);
	UDP() = default;
	~UDP();

	WSADATA m_wsaData;
	SOCKET m_socket;
	sockaddr_in m_sockaddr;
};

class UDPServer : protected UDP
{
public:
	explicit UDPServer(const char* port);
	int RecvData(uint8_t* out, int outlen, uint32_t* ipaddr, uint16_t* port);

private:
	sockaddr_in m_sockaddr_remote;
};

class UDPClient : protected UDP
{
public:
	explicit UDPClient(const char* ip, const char* port);
	void SendData(const uint8_t* data, int len);
};

#endif