#pragma once
#include "definitions.h"
#include "address.h"
#include "netinterface.h"
#include "netpacket.h"
#include "eth.h"
#include "ipv4.h"

#define ICMP_ECHO_REPLY		0
#define ICMP_ECHO_REQUEST	8

struct ICMP_Header
{
	uint8 type;
	uint8 code;
	uint16 checksum;
	uint16 id;
	uint16 seq;
};

struct ICMP_Packet
{
	ICMP_Header* header;
	uint8* data;
	uint32 data_length;
};

static class ICMP
{
public:
	static void Receive(NetInterface* intf, IPv4_Header* ip_hdr, NetPacket* pkt);
	static bool Decode(ICMP_Packet* ih, NetPacket* pkt);

	static void Send(NetInterface* intf, ICMP_Packet* icmp, IPv4Address tip, uint8 type);
	static void SendReply(NetInterface* intf, ICMP_Packet* icmp, IPv4Address tip);
};