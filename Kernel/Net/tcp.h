#pragma once
#include "tcptypes.h"
#include "ipv4.h"
#include "tcpsession.h"

#define MAX_SESSIONS 32

namespace TCP
{
	TcpSession* CreateSession();
	NetPacket* CreatePacket(NetInterface* intf, IPv4Address dst, uint16 src_port, uint16 dst_port, uint8 flags, uint32 seq, uint32 ack, uint8* data, uint32 data_length);

	void Send(NetInterface* intf, NetPacket* pkt);
	void HandlePacket(NetInterface* intf, IPv4_Header* ip_hdr, NetPacket* pkt);
};
