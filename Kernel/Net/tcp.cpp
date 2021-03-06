#include <Net/packetmanager.h>
#include <Net/tcp.h>
#include <Net/tcpsession.h>
#include <debug.h>
#include <hal.h>
#include <list.h>
#include <math.h>
#include <sync.h>

namespace TCP {
List<TcpSession *> sessions;
Mutex sessions_mutex;

TcpSession *GetSession(uint16 src_port, uint16 dst_port, uint32 dst_addr)
{
    sessions_mutex.Aquire();

    for (int i = 0; i < sessions.Count(); i++) {
        TcpSession *session = sessions[i];
        Socket *socket = session->socket;
        sockaddr_in *addr = (sockaddr_in *)socket->addr;

        bool valid_port = addr->sin_port == htons(dst_port) && socket->src_port == src_port;
        bool valid_ip = addr->sin_addr.s_addr == dst_addr;

        if (valid_port && valid_ip) {
            sessions_mutex.Release();
            return session;
        }
    }

    for (int i = 0; i < sessions.Count(); i++) {
        TcpSession *session = sessions[i];
        sockaddr_in *addr = (sockaddr_in *)session->socket->addr;

        bool valid_port = addr->sin_port == htons(src_port);
        bool valid_ip = addr->sin_addr.s_addr == INADDR_ANY;

        if (valid_port && valid_ip) {
            sessions_mutex.Release();
            return session;
        }
    }

    sessions_mutex.Release();
    return 0;
}

int CreateSession(Socket *socket)
{
    if (!socket)
        return -1;

    sessions_mutex.Aquire();
    socket->tcp_session = new TcpSession();
    socket->tcp_session->socket = socket;
    sessions.Add(socket->tcp_session);
    sessions_mutex.Release();

    return 0;
}

Socket *SessionAccept(Socket *socket, const sockaddr_in *addr, int flags)
{
    if (!socket->tcp_session)
        return 0;

    return socket->tcp_session->Accept(addr, flags);
}

int SessionShutdown(Socket *socket, int how)
{
    if (!socket->tcp_session)
        return -1;

    return socket->tcp_session->Shutdown(how);
}

int SessionClose(Socket *socket)
{
    if (!socket->tcp_session)
        return -1;

    socket->tcp_session->Shutdown(SHUT_RDWR);

    sessions_mutex.Aquire();
    sessions.Remove(socket->tcp_session);
    sessions_mutex.Release();
    return 0;
}

int SessionListen(Socket *socket, int backlog)
{
    if (!socket->tcp_session)
        return -1;

    return socket->tcp_session->Listen(backlog);
}

int SessionConnect(Socket *socket, const sockaddr_in *addr)
{
    if (!socket->tcp_session)
        return -1;

    return socket->tcp_session->Connect(addr);
}

int SessionSend(Socket *socket, const void *buf, size_t len, int flags)
{
    if (!socket->tcp_session)
        return -1;

    return socket->tcp_session->SendData(buf, len, flags);
}

int SessionRecv(Socket *socket, void *buf, size_t len, int flags)
{
    if (!socket->tcp_session)
        return -1;

    return socket->tcp_session->RecvData(buf, len, flags);
}

bool Decode(TCP_Packet *tp, NetPacket *pkt)
{
    TCP_Header *header = (TCP_Header *)pkt->start;
    tp->header = header;

    tp->header->src_port = ntohs(header->src_port);
    tp->header->dst_port = ntohs(header->dst_port);
    tp->header->seq = ntohl(header->seq);
    tp->header->ack = ntohl(header->ack);
    tp->header->offset = header->offset;
    tp->header->flags = header->flags;
    tp->header->window_size = htons(header->window_size);
    tp->header->checksum = htons(header->checksum);
    tp->header->urg_ptr = htons(header->urg_ptr);

    tp->data = (uint8 *)header + header->offset / 4;
    tp->data_length = pkt->end - tp->data;
    return true;
}

NetPacket *CreatePacket(NetInterface *intf, const sockaddr_in *dest_addr, uint16 src_port,
                        uint8 flags, uint32 seq, uint32 ack, const void *data, uint32 data_length)
{
    uint32 dst = dest_addr->sin_addr.s_addr;
    NetPacket *pkt =
        IPv4::CreatePacket(intf, dst, IP_PROTOCOL_TCP, sizeof(TCP_Header) + data_length);

    if (!pkt)
        return 0;

    TCP_Header *header = (TCP_Header *)pkt->end;
    header->src_port = htons(src_port);
    header->dst_port = dest_addr->sin_port;
    header->seq = htonl(seq);
    header->ack = htonl(ack);

    header->offset = sizeof(TCP_Header) * 4;
    header->flags = flags;
    header->window_size = ntohs(TCP_WINDOW_SIZE);
    header->checksum = ntohs(0);
    header->urg_ptr = ntohs(0);

    memcpy((uint8 *)header + sizeof(TCP_Header), data, data_length);

    IPv4_PSEUDO_HEADER pseudo;
    pseudo.src = intf->GetIP();
    pseudo.dst = dst;
    pseudo.reserved = 0;
    pseudo.protocol = IP_PROTOCOL_TCP;
    pseudo.length = ntohs(sizeof(TCP_Header) + data_length);

    header->checksum = Net::ChecksumDouble(&pseudo, sizeof(IPv4_PSEUDO_HEADER), header,
                                           sizeof(TCP_Header) + data_length);
    pkt->end += sizeof(TCP_Header) + data_length;
    return pkt;
}

void Send(NetPacket *pkt)
{
    // checksum
    IPv4::Send(pkt);
}

void HandlePacket(IPv4_Header *ip_hdr, NetPacket *pkt)
{
    TCP_Packet tcp;

    if (!Decode(&tcp, pkt))
        return;

    TcpSession *session = GetSession(tcp.header->dst_port, tcp.header->src_port, ip_hdr->src);

    bool handled = false;

    if (session)
        handled = session->HandlePacket(ip_hdr, &tcp);

    if (!handled) {
        NetInterface *intf = PacketManager::GetInterface(ip_hdr->src);
        sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(tcp.header->src_port);
        addr.sin_addr.s_addr = ip_hdr->src;
        NetPacket *rst_packet = CreatePacket(intf, &addr, tcp.header->dst_port, TCP_RST | TCP_ACK,
                                             tcp.header->ack, tcp.header->seq + 1, 0, 0);
        Send(rst_packet);
    }
}
} // namespace TCP
