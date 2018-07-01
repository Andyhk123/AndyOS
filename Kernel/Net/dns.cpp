#include "dns.h"
#include "udpsocket.h"
#include "Lib/debug.h"

IPv4Address DNS::dns_server;
DNS_TABLE_ENTRY DNS::dns_cache[DNS_CACHE_SIZE];

STATUS DNS::Init()
{
	for (int i = 0; i < DNS_CACHE_SIZE; i++)
	{
		dns_cache[i].name = new char[256];
		dns_cache[i].name[0] = 0;
		dns_cache[i].addr = Net::NullIPv4;
	}

	return STATUS_SUCCESS;
}

void DNS::Receive(NetInterface* intf, IPv4_Header* ip_hdr, UDP_Packet* udp, NetPacket* pkt)
{
	debug_print("Received DNS\n");

	DNS_Packet dns;
	if (!Decode(&dns, udp))
		return;

	char buf[256];
	DNS_Answer ans;

	uint8* ptr = dns.data;
	ptr = ParseDomain(buf, ptr);
	ptr = ParseAnswer(&ans, ptr);

	AddEntry(buf, ans.addr);
}

bool DNS::Decode(DNS_Packet* dns, UDP_Packet* udp)
{
	DNS_Header* header = (DNS_Header*)udp->data;
	dns->hdr = header;

	header->id = ntohs(header->id);
	header->flags = ntohs(header->flags);
	header->quest_count = ntohs(header->quest_count);
	header->ans_count = ntohs(header->ans_count);
	header->auth_count = ntohs(header->auth_count);
	header->add_count = ntohs(header->add_count);

	dns->data = (uint8*)header + sizeof(DNS_Header);
	dns->data_length = udp->data_length - sizeof(DNS_Header);
	return 1;
}

void DNS::Query(NetInterface* intf, char* name)
{
	int dom_len = strlen(name);
	uint8* data = new uint8[sizeof(DNS_Header) + dom_len + 4];

	DNS_Header* dns = (DNS_Header*)data;
	dns->id = htons(42);
	dns->flags = htons(0x0100);
	dns->quest_count = htons(1);
	dns->ans_count = htons(0);
	dns->auth_count = htons(0);
	dns->add_count = htons(0);

	AppendDomain(data + sizeof(DNS_Header), name);

	NetPacket* pkt = UDP::CreatePacket(intf, Net::GatewayIPv4, 53, 53, data, sizeof(DNS_Header) + dom_len + 4);
	intf->Send(pkt);

	delete[] data;
}

uint8* DNS::AppendDomain(uint8* ptr, char* name)
{
	uint8* labelHead = ptr++;
	char* p = name;

	while (1)
	{
		char c = *p++;

		if (c == '.' || c == '\0')
		{
			int labelLen = ptr - labelHead - 1;
			*labelHead = labelLen;
			labelHead = ptr;
		}

		*ptr++ = c;

		if (!c)
			break;
	};

	uint16* end = (uint16*)ptr;
	*end++ = htons(1);
	*end++ = htons(1);

	return (uint8*)end;
}

uint8* DNS::ParseDomain(char* buf, uint8* ptr)
{
	int len = *ptr++;
	while (len--)
	{
		*buf++ = *ptr++;

		if (!len)
		{
			if (*ptr)
			{
				*buf++ = '.';
				len = *ptr++;
			}
			else
			{
				*buf++ = *ptr++;
			}
		}
	}

	uint16* end = (uint16*)ptr;
	uint16 type = *end++;
	uint16 clas = *end++;

	return (uint8*)end;
}

uint8* DNS::ParseAnswer(DNS_Answer* ans, uint8* ptr)
{
	DNS_Answer* da = (DNS_Answer*)ptr;

	ans->name = ntohs(da->name);
	ans->type = ntohs(da->type);
	ans->clas = ntohs(da->clas);
	ans->ttl = ntohl(da->ttl);
	ans->data_length = ntohs(da->data_length);
	ans->addr = da->addr;

	return ptr + sizeof(DNS_Answer);
}

void DNS::AddEntry(char* name, IPv4Address addr)
{
	for (int i = 0; i < DNS_CACHE_SIZE; i++)
	{
		if (!dns_cache[i].name[0] || !strcmp(dns_cache[i].name, name))
		{
			if (!dns_cache[i].name[0])
			{
				debug_print("DNS added entry: %s ", name);
				Net::PrintIP("", addr);
			}

			strcpy(dns_cache[i].name, name);
			dns_cache[i].addr = addr;
			break;
		}
	}
}

IPv4Address DNS::LookupAddress(char* name)
{
	for (int i = 0; i < DNS_CACHE_SIZE; i++)
	{
		if (!strcmp(dns_cache[i].name, name))
		{
			return dns_cache[i].addr;
		}
	}

	return Net::NullIPv4;
}