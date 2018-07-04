#include "dhcp.h"
#include "Lib/debug.h"

#define BOOTREQUEST 	1
#define BOOTREPLY 		2
#define DHCP_MAGIC 		0x63825363

#define TYPE_DISCOVER	1
#define TYPE_OFFER		2
#define TYPE_REQUEST	3
#define TYPE_ACK		5
#define TYPE_NAK		6

#define OPT_SUBNET_MASK		1
#define OPT_ROUTER			3
#define OPT_DNS				6
#define OPT_HOSTNAME		12
#define OPT_REQUESTED		50
#define OPT_LEASE_TIME		51
#define OPT_MESSAGE_TYPE	53
#define OPT_DHCP_SERVER		54
#define OPT_PARAM_LIST		55
#define OPT_END				255


namespace DHCP
{
	struct DHCP_Options
	{
		IPv4Address* subnet_mask = 0;
		IPv4Address* router = 0;
		IPv4Address* dns = 0;
		uint32 lease = 0;
		uint8 type = 0;
	};

	uint32 xid = 0x6AC9D9B2;

	DHCP_Header* CreateHeader(NetInterface* intf, uint8** options)
	{
		DHCP_Header* header = new DHCP_Header;
		header->op = BOOTREQUEST;
		header->htype = 1;
		header->hlen = 6;
		header->hops = 0;
		header->xid = htonl(xid++); //transaction id
		header->secs = 0;
		header->flags = htons(0x8000); //broadcast
		header->ciaddr = IPv4Address();
		header->yiaddr = IPv4Address();
		header->siaddr = IPv4Address();
		header->giaddr = IPv4Address();
		header->chaddr = intf->GetMac();
		header->magic = htonl(DHCP_MAGIC);

		//Options
		char* hostname = "AndyOS";
		int hostlen = strlen(hostname);

		int i = 0;
		header->options[i++] = OPT_HOSTNAME;
		header->options[i++] = hostlen;
		memcpy(&header->options[i], hostname, hostlen);
		i += hostlen;

		*options = header->options + i;
		return header;
	}

	void Discover(NetInterface* intf)
	{
		uint8* options;
		DHCP_Header* header = CreateHeader(intf, &options);
		int i = 0;

		options[i++] = OPT_MESSAGE_TYPE;
		options[i++] = 1;
		options[i++] = TYPE_DISCOVER;

		options[i++] = OPT_PARAM_LIST;
		options[i++] = 0x03;
		options[i++] = OPT_SUBNET_MASK;
		options[i++] = OPT_ROUTER;
		options[i++] = OPT_DNS;

		options[i++] = OPT_END;
		int length = (int)&options[i] - (int)header;

		NetPacket* pkt = UDP::CreatePacket(intf, Net::BroadcastIPv4, PORT_DHCP_DST, PORT_DHCP_SRC, (uint8*)header, length);
		intf->Send(pkt);
	}

	void Request(NetInterface* intf, IPv4Address* requested, IPv4Address* server)
	{
		uint8* options;
		DHCP_Header* header = CreateHeader(intf, &options);
		int i = 0;

		options[i++] = OPT_MESSAGE_TYPE;
		options[i++] = 1;
		options[i++] = TYPE_REQUEST;

		options[i++] = OPT_REQUESTED;
		options[i++] = 4;
		*(uint32*)&options[i] = requested->ToIntNet();
		i += 4;

		options[i++] = OPT_DHCP_SERVER;
		options[i++] = 4;
		*(uint32*)&options[i] = server->ToIntNet();
		i += 4;

		options[i++] = OPT_END;
		int length = (int)&options[i] - (int)header;

		NetPacket* pkt = UDP::CreatePacket(intf, Net::BroadcastIPv4, PORT_DHCP_DST, PORT_DHCP_SRC, (uint8*)header, length);
		intf->Send(pkt);
	}

	void ParseOptions(DHCP_Header* offer, DHCP_Options& options)
	{
		uint8* data = (uint8*)offer->options;
		int i = 0;

		while (data[i] != 0xFF && i < 308)
		{
			uint8 type = data[i++];
			uint8 length = data[i++];

			switch (type)
			{
			case OPT_SUBNET_MASK:
				options.subnet_mask = (IPv4Address*)&data[i];
				break;

			case OPT_ROUTER:
				options.router = (IPv4Address*)&data[i];
				break;

			case OPT_DNS:
				options.dns = (IPv4Address*)&data[i];
				break;

			case OPT_LEASE_TIME:
				options.lease = htonl(*(uint32*)&data[i]);
				break;

			case OPT_MESSAGE_TYPE:
				options.type = data[i];
				break;
			}

			i += length;
		}
	}

	void Receive(NetInterface* intf, IPv4_Header* ip_hdr, UDP_Packet* udp, NetPacket* pkt)
	{
		DHCP_Header* offer = (DHCP_Header*)udp->data;
		IPv4Address addr = offer->yiaddr;

		DHCP_Options options;
		ParseOptions(offer, options);

		if (options.type == TYPE_OFFER)
		{
			debug_print("---------DHCP OFFER-----------\n");
			Net::PrintIP("Address:		", addr);
			Net::PrintIP("Subnet mask:	", *options.subnet_mask);
			Net::PrintIP("Router:			", *options.router);
			Net::PrintIP("DNS server:		", *options.dns);
			debug_print("Lease time:		%ui", options.lease);
			debug_print("\n------------------------------\n");

			Request(intf, &addr, options.router);
		}
		else if (options.type == TYPE_ACK)
		{
			debug_print("dhcp: ack\n");
		}
		else if (options.type == TYPE_NAK)
		{
			debug_print("dhcp: nak\n");
		}
	}
}