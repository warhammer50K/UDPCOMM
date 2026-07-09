#pragma once

// Uncomment to split every payload byte into two half-bytes on the wire
// (for links/parsers that cannot handle arbitrary binary data).
//#define USE_BYTE_SPLIT

// POSIX
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>

// stl
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

// which local interface to bind to
enum class NetInterface
{
    Any,      // 0.0.0.0
    Ethernet, // first wired, non-loopback interface
    Wifi,     // first wireless interface (detected via /sys/class/net/<if>/wireless)
    Loopback  // 127.0.0.1
};

struct senderInfo
{
    std::string address;
    uint16_t port = 0;
};

template <typename SEND_STRUCT, typename RECEIVE_STRUCT>
class UDPCOMM
{
public:
    /* if you use wifi network and want set port 3333, set this
       UDPCOMM<(struct you want to send),(struct you want to receive)>(NetInterface::Wifi, 3333); */
    explicit UDPCOMM(NetInterface _my_interface, uint16_t _my_port);

    // bind to an explicit local address, e.g. UDPCOMM<S,R>("127.0.0.1", 3333)
    explicit UDPCOMM(const std::string &_bind_ip, uint16_t _my_port);

    ~UDPCOMM();

    /* use this function */
    // add address(ip & port) you want send struct
    void add_send_address(const std::string &_address, uint16_t _port);

    // you send struct to registrated address (add_send_address or auto-registered peer)
    void write_dataGram(const SEND_STRUCT &str);

    // get received struct (latest value, filled by the background thread)
    RECEIVE_STRUCT get_received_struct();

    // false if socket creation/binding failed in the constructor
    bool is_bound() const { return sock >= 0; }

private:
    int sock = -1;
    std::string my_address;
    uint16_t my_port = 0;

    // registrated address (ip, port); guarded by senders_mtx
    std::vector<senderInfo> senders_info;
    std::mutex senders_mtx;

    // latest received struct; guarded by recv_mtx
    RECEIVE_STRUCT receive_struct;
    std::mutex recv_mtx;

    // partial-datagram reassembly buffer (receive thread only)
    std::vector<char> receive_buffer;

    // data packet's head & tail
    const char packet_head[2] = {(char)0xFF, (char)0xFD};
    const char packet_tail[2] = {(char)0xFB, (char)0xFA};

    // data receive thread
    std::atomic<bool> callbackFlag{false};
    std::thread callbackThread;

    void init(const std::string &_bind_ip, uint16_t _my_port);
    void callbackLoop();
    void parse_frames();
    void register_sender(const std::string &_address, uint16_t _port);

    // Network information found according to the interface type
    static bool find_my_address(std::string &_receive_address, NetInterface _type);
};

////////////////////////////////////////////////////////
///
/// UDPCOMM class definition
/// template class
///
////////////////////////////////////////////////////////

template <typename SEND_STRUCT, typename RECEIVE_STRUCT>
UDPCOMM<SEND_STRUCT, RECEIVE_STRUCT>::UDPCOMM(NetInterface _my_interface, uint16_t _my_port)
{
    std::string bind_ip;
    if (!find_my_address(bind_ip, _my_interface))
    {
        printf("[FAILED] failed bind ip.\n");
        return;
    }
    init(bind_ip, _my_port);
}

template <typename SEND_STRUCT, typename RECEIVE_STRUCT>
UDPCOMM<SEND_STRUCT, RECEIVE_STRUCT>::UDPCOMM(const std::string &_bind_ip, uint16_t _my_port)
{
    init(_bind_ip, _my_port);
}

template <typename SEND_STRUCT, typename RECEIVE_STRUCT>
void UDPCOMM<SEND_STRUCT, RECEIVE_STRUCT>::init(const std::string &_bind_ip, uint16_t _my_port)
{
    my_address = _bind_ip;
    my_port = _my_port;

    sock = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0)
    {
        printf("[FAILED] failed socket creation.\n");
        return;
    }

    sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(my_port);
    if (inet_pton(AF_INET, my_address.c_str(), &server_addr.sin_addr) != 1)
    {
        printf("[FAILED] invalid bind ip: %s\n", my_address.c_str());
        ::close(sock);
        sock = -1;
        return;
    }

    if (::bind(sock, (sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        printf("[FAILED] failed socket binding.\n");
        ::close(sock);
        sock = -1;
        return;
    }

    // recv timeout so the receive thread can check the stop flag periodically
    timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 100 * 1000;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    printf("[UDPCOMM] bind, ip:%s, port:%d\n", my_address.c_str(), (int)my_port);

    // start callback loop
    callbackFlag = true;
    callbackThread = std::thread(&UDPCOMM::callbackLoop, this);
}

template <typename SEND_STRUCT, typename RECEIVE_STRUCT>
UDPCOMM<SEND_STRUCT, RECEIVE_STRUCT>::~UDPCOMM()
{
    if (callbackThread.joinable())
    {
        callbackFlag = false;
        callbackThread.join();
    }

    if (sock >= 0)
    {
        ::close(sock);
    }
}

template <typename SEND_STRUCT, typename RECEIVE_STRUCT>
bool UDPCOMM<SEND_STRUCT, RECEIVE_STRUCT>::find_my_address(std::string &_receive_address, NetInterface _type)
{
    if (_type == NetInterface::Any)
    {
        _receive_address = "0.0.0.0";
        return true;
    }

    ifaddrs *ifaddr = nullptr;
    if (getifaddrs(&ifaddr) == -1)
    {
        return false;
    }

    bool flag = false;
    for (ifaddrs *ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr == nullptr || ifa->ifa_addr->sa_family != AF_INET)
        {
            continue;
        }

        const bool is_loopback = (ifa->ifa_flags & IFF_LOOPBACK) != 0;

        // wireless interfaces expose /sys/class/net/<name>/wireless
        struct stat st;
        const std::string wireless_path = std::string("/sys/class/net/") + ifa->ifa_name + "/wireless";
        const bool is_wireless = (stat(wireless_path.c_str(), &st) == 0);

        bool match = false;
        switch (_type)
        {
        case NetInterface::Loopback: match = is_loopback; break;
        case NetInterface::Wifi:     match = is_wireless; break;
        case NetInterface::Ethernet: match = !is_loopback && !is_wireless; break;
        default: break;
        }

        if (match)
        {
            char ip[INET_ADDRSTRLEN];
            sockaddr_in *addr = (sockaddr_in *)ifa->ifa_addr;
            inet_ntop(AF_INET, &addr->sin_addr, ip, sizeof(ip));
            _receive_address = ip;

            printf("[REGISTRATION] bind address: %s (%s)\n", ip, ifa->ifa_name);
            flag = true;
            break;
        }
    }

    freeifaddrs(ifaddr);
    return flag;
}

template <typename SEND_STRUCT, typename RECEIVE_STRUCT>
void UDPCOMM<SEND_STRUCT, RECEIVE_STRUCT>::add_send_address(const std::string &_address, uint16_t _port)
{
    register_sender(_address, _port);
    printf("[REGISTRATION] add address: %s, port: %d\n", _address.c_str(), (int)_port);
}

template <typename SEND_STRUCT, typename RECEIVE_STRUCT>
void UDPCOMM<SEND_STRUCT, RECEIVE_STRUCT>::register_sender(const std::string &_address, uint16_t _port)
{
    std::lock_guard<std::mutex> lock(senders_mtx);
    for (size_t i = 0; i < senders_info.size(); ++i)
    {
        if (senders_info[i].address == _address && senders_info[i].port == _port)
        {
            return;
        }
    }

    senderInfo _send;
    _send.address = _address;
    _send.port = _port;
    senders_info.push_back(_send);
}

template <typename SEND_STRUCT, typename RECEIVE_STRUCT>
void UDPCOMM<SEND_STRUCT, RECEIVE_STRUCT>::write_dataGram(const SEND_STRUCT &str)
{
    if (sock < 0)
    {
        return;
    }

    std::vector<senderInfo> targets;
    {
        std::lock_guard<std::mutex> lock(senders_mtx);
        targets = senders_info;
    }

    if (targets.empty())
    {
        printf("[FAILED] senders_info size is zero.\n");
        return;
    }

    std::vector<char> payload(sizeof(SEND_STRUCT));
    memcpy(payload.data(), &str, sizeof(SEND_STRUCT));

#ifdef USE_BYTE_SPLIT
    std::vector<char> split(payload.size() * 2);
    for (size_t i = 0; i < payload.size(); ++i)
    {
        split[2 * i] = payload[i] & (char)0x0F;
        split[2 * i + 1] = (payload[i] >> 4) & (char)0x0F;
    }
    payload.swap(split);
#endif

    // add head & tail to serialized data
    std::vector<char> packet;
    packet.reserve(payload.size() + 4);
    packet.push_back(packet_head[0]);
    packet.push_back(packet_head[1]);
    packet.insert(packet.end(), payload.begin(), payload.end());
    packet.push_back(packet_tail[0]);
    packet.push_back(packet_tail[1]);

    for (size_t i = 0; i < targets.size(); ++i)
    {
        sockaddr_in client_addr;
        memset(&client_addr, 0, sizeof(client_addr));
        client_addr.sin_family = AF_INET;
        client_addr.sin_port = htons(targets[i].port);
        if (inet_pton(AF_INET, targets[i].address.c_str(), &client_addr.sin_addr) != 1)
        {
            continue;
        }

        ::sendto(sock, packet.data(), packet.size(), 0, (const sockaddr *)&client_addr, sizeof(client_addr));
    }
}

template <typename SEND_STRUCT, typename RECEIVE_STRUCT>
void UDPCOMM<SEND_STRUCT, RECEIVE_STRUCT>::callbackLoop()
{
    std::vector<char> buf(2048);

    while (callbackFlag)
    {
        sockaddr_in client_addr;
        memset(&client_addr, 0, sizeof(client_addr));
        socklen_t len = sizeof(client_addr);

        ssize_t n = ::recvfrom(sock, buf.data(), buf.size(), 0, (sockaddr *)&client_addr, &len);
        if (n <= 0)
        {
            continue; // timeout (SO_RCVTIMEO) or error — re-check stop flag
        }

        // auto-register the peer so replies work without add_send_address()
        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, ip, sizeof(ip));
        register_sender(ip, ntohs(client_addr.sin_port));

        receive_buffer.insert(receive_buffer.end(), buf.data(), buf.data() + n);
        parse_frames();
    }
}

template <typename SEND_STRUCT, typename RECEIVE_STRUCT>
void UDPCOMM<SEND_STRUCT, RECEIVE_STRUCT>::parse_frames()
{
#ifdef USE_BYTE_SPLIT
    const size_t body_size = sizeof(RECEIVE_STRUCT) * 2;
#else
    const size_t body_size = sizeof(RECEIVE_STRUCT);
#endif
    const size_t packet_size = body_size + 4;

    while (true)
    {
        // header check — drop garbage before the first head marker
        size_t head_pos = std::string::npos;
        for (size_t p = 0; p + 1 < receive_buffer.size(); ++p)
        {
            if (receive_buffer[p] == packet_head[0] && receive_buffer[p + 1] == packet_head[1])
            {
                head_pos = p;
                break;
            }
        }

        if (head_pos == std::string::npos)
        {
            receive_buffer.clear();
            return;
        }

        if (head_pos > 0)
        {
            receive_buffer.erase(receive_buffer.begin(), receive_buffer.begin() + head_pos);
        }

        if (receive_buffer.size() < packet_size)
        {
            return; // wait for the rest of the packet
        }

        if (receive_buffer[packet_size - 2] == packet_tail[0] && receive_buffer[packet_size - 1] == packet_tail[1])
        {
            const char *body = receive_buffer.data() + 2;

#ifdef USE_BYTE_SPLIT
            std::vector<char> combined(sizeof(RECEIVE_STRUCT));
            for (size_t i = 0; i < combined.size(); ++i)
            {
                const char low = body[2 * i] & (char)0x0F;
                const char high = (body[2 * i + 1] & (char)0x0F) << 4;
                combined[i] = low | high;
            }
            body = combined.data();

            std::lock_guard<std::mutex> lock(recv_mtx);
            memcpy(&receive_struct, body, sizeof(RECEIVE_STRUCT));
#else
            std::lock_guard<std::mutex> lock(recv_mtx);
            memcpy(&receive_struct, body, sizeof(RECEIVE_STRUCT));
#endif

            receive_buffer.erase(receive_buffer.begin(), receive_buffer.begin() + packet_size);
        }
        else
        {
            // corrupt frame — skip this head marker and resync
            receive_buffer.erase(receive_buffer.begin(), receive_buffer.begin() + 2);
        }
    }
}

template <typename SEND_STRUCT, typename RECEIVE_STRUCT>
RECEIVE_STRUCT UDPCOMM<SEND_STRUCT, RECEIVE_STRUCT>::get_received_struct()
{
    std::lock_guard<std::mutex> lock(recv_mtx);
    return receive_struct;
}
