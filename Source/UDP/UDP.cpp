#include "UDP/UDP.h"
#include <iostream>

UDP::UDP(const char* port)
{
	WORD socketVer = MAKEWORD(2, 2);
	if (WSAStartup(socketVer, &m_wsaData) != 0)
	{
		std::cerr << "Cannot initialize socket, exiting.\n";
		exit(-1);
	}
	m_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	m_sockaddr.sin_family = AF_INET;
	m_sockaddr.sin_port = htons(atoi(port));
	m_sockaddr.sin_addr.S_un.S_addr = INADDR_ANY;
}

UDP::~UDP() 
{
	WSACleanup();
	closesocket(m_socket);
}

UDPServer::UDPServer(const char* port)
	: UDP(port)
{
	if (bind(m_socket, (sockaddr*)&m_sockaddr, sizeof(m_sockaddr)) == SOCKET_ERROR) {
		std::cerr << "Cannot bind to port " << ntohs(m_sockaddr.sin_port)
			<< ", exiting.\n";
		closesocket(m_socket);
		exit(-1);
	}
}

int UDPServer::RecvData(uint8_t* out, int outlen, uint32_t* ipaddr, uint16_t* port)
{
	int remote_size = sizeof(sockaddr);
	int data_size = 0;
	int count = 0;
	while (1)
	{
		data_size = recvfrom(m_socket, (char*)out, outlen, 0, (sockaddr *)&m_sockaddr_remote, &remote_size);
		if (data_size > 0)
		{
			*ipaddr = (uint32_t)m_sockaddr_remote.sin_addr.S_un.S_addr;
			*port = m_sockaddr.sin_port;
			std::cout << "Received UDP packet from " << inet_ntoa(m_sockaddr_remote.sin_addr)
				<< ":" << ntohs(m_sockaddr.sin_port) << " with " << data_size << " bytes.\n";
			break;
		}
		count++;
		if (count >= 10)
		{
			std::cerr << "Received 10 UDP packet with error. \n";
			break;
		}
	}
	return data_size;
}

UDPClient::UDPClient(const char* ip, const char* port)
	: UDP(port)
{
	unsigned long ipaddr = inet_addr(ip);
	m_sockaddr.sin_addr.S_un.S_addr = ipaddr;
}

void UDPClient::SendData(const uint8_t* data, int len)
{
	sendto(m_socket, (char*)data, len, 0, (sockaddr*)&m_sockaddr, sizeof(m_sockaddr));
}