#pragma once

#ifndef __UDP_H__
#define __UDP_H__

#include <winsock2.h>
#include <JuceHeader.h>

struct ANetPacket;

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
	int RecvPacket(ANetPacket& out);

private:
	sockaddr_in m_sockaddr_remote;
};

class UDPClient : protected UDP
{
public:
	explicit UDPClient(const char* ip, const char* port);
	void SendPacket(const ANetPacket& packet);
};

#endif