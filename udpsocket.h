#ifndef UDPSOCKET_H
#define UDPSOCKET_H

// linux native
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>

// stl
#include <tbb/concurrent_queue.h>
#include <tbb/concurrent_vector.h>
#include <thread>

// qt
#include <QByteArray>
#include <QHostAddress>

struct senderInfo_with_data
{
    QByteArray buf;
    QHostAddress address;
    qint16 port;
    senderInfo_with_data() {}
};

struct senderInfo
{
    QHostAddress address;
    qint16 port;
    senderInfo() {}
};

class UdpSocket
{
public:
    UdpSocket();
    ~UdpSocket();

    int bind(QHostAddress addr, unsigned short port);
    void writeDatagram(const QByteArray &datagram, const QHostAddress &host, quint16 port);

    int sockfd = 0;

    tbb::concurrent_queue<senderInfo_with_data> buf_que;
    tbb::concurrent_queue<senderInfo_with_data> send_que;

    std::atomic<bool> recv_flag;
    std::thread* recv_thread = NULL;
    void recv_loop();

    std::atomic<bool> send_flag;
    std::thread* send_thread = NULL;
    void send_loop();
};

#endif // UDPSOCKET_H
