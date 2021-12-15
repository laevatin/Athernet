#pragma once

#ifndef __PING_H__
#define __PING_H__

#include <winsock2.h>
#include <iphlpapi.h>
#include <icmpapi.h>
#include <JuceHeader.h>

#define ICMP_BUFFER_SIZE 128

class IcmpPing
{
public: 
	explicit IcmpPing(uint32_t destip);
	int IcmpSendPing();

private:
	HANDLE m_handle;
	ICMP_ECHO_REPLY m_reply; //ICMP_ECHO_REPLY data structure
	IPAddr m_address;
	char m_recvBuffer[ICMP_BUFFER_SIZE];
};


#endif
