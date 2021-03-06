#include <Net/dhcp.h>
#include <Net/packetmanager.h>
#include <Net/socketmanager.h>
#include <Net/udp.h>
#include <debug.h>
#include <net.h>

namespace UDP {
bool Decode(UDP_Packet *up, NetPacket *pkt)
{
    UDP_Header *header = (UDP_Header *)pkt->start;
    up->header = header;

    header->src_port = ntohs(header->src_port);
    header->dst_port = ntohs(header->dst_port);
    header->length = ntohs(header->length);
    header->checksum = ntohs(header->checksum);

    up->data = (uint8 *)header + sizeof(UDP_Header);
    up->data_length = up->header->length - sizeof(UDP_Header);
    return 1;
}

NetPacket *CreatePacket(NetInterface *intf, const sockaddr_in *dest_addr, uint16 src_port,
                        const void *data, size_t len)
{
    uint32 dst = dest_addr->sin_addr.s_addr;
    NetPacket *pkt = IPv4::CreatePacket(intf, dst, IP_PROTOCOL_UDP, sizeof(UDP_Header) + len);

    if (!pkt)
        return 0;

    UDP_Header *header = (UDP_Header *)pkt->end;
    header->src_port = htons(src_port);
    header->dst_port = dest_addr->sin_port;
    header->length = htons(sizeof(UDP_Header) + len);
    header->checksum = 0;

    memcpy(header + 1, data, len);

    IPv4_PSEUDO_HEADER pseudo;
    pseudo.src = intf->GetIP();
    pseudo.dst = dest_addr->sin_addr.s_addr;
    pseudo.reserved = 0;
    pseudo.protocol = IP_PROTOCOL_UDP;
    pseudo.length = ntohs(sizeof(UDP_Header) + len);

    header->checksum =
        Net::ChecksumDouble(&pseudo, sizeof(IPv4_PSEUDO_HEADER), header, sizeof(UDP_Header) + len);
    pkt->end += sizeof(UDP_Header) + len;
    return pkt;
}

int Send(NetPacket *pkt)
{
    if (!pkt)
        return -1;

    // checksum
    return IPv4::Send(pkt);
}

void HandlePacket(IPv4_Header *ip_hdr, NetPacket *pkt)
{
    UDP_Packet udp;
    if (!Decode(&udp, pkt))
        return;

    pkt->start += udp.header->length;

    uint16 src_port = udp.header->src_port;
    uint16 dst_port = udp.header->dst_port;

    switch (dst_port) {
    case PORT_DHCP_SRC: {
        DHCP::HandlePacket(ip_hdr, &udp, pkt);
    } break;

    default:
        Socket *socket = SocketManager::GetUdpSocket(dst_port);

        if (socket) {
            sockaddr_in *addr = new sockaddr_in();
            addr->sin_family = AF_INET;
            addr->sin_addr.s_addr = ip_hdr->src;
            addr->sin_port = htons(src_port);
            socket->recv_addr = (sockaddr *)addr;
            socket->HandleData(udp.data, udp.data_length);
        }
        break;
    }
}

STATUS Init()
{
    return STATUS_SUCCESS;
}
} // namespace UDP
