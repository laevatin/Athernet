#include "NAT/Ping.h"

#include <winsock2.h>
#include <iphlpapi.h>
#include <icmpapi.h>
#include <iostream>

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")

IcmpPing::IcmpPing(const char* destip)
{
	m_handle = IcmpCreateFile();
	m_address = inet_addr(destip);
}

int IcmpPing::IcmpSendPing()
{
	char randomdata[] = "Hello World!";
	int success = IcmpSendEcho(m_handle, m_address, randomdata, sizeof(randomdata), NULL, m_recvBuffer, ICMP_BUFFER_SIZE, 1000);
	memcpy(&m_reply, m_recvBuffer, sizeof(m_reply));
	if (success)
		std::cout << "Success! It took " << m_reply.RoundTripTime << " milliseconds.\n";
	return success;
}
