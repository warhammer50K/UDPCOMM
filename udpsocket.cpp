#include "udpsocket.h"

UdpSocket::UdpSocket()
{

}

UdpSocket::~UdpSocket()
{
    if(recv_thread != NULL)
    {
        recv_flag = false;
        recv_thread->join();
        recv_thread = NULL;
    }

    if(send_thread != NULL)
    {
        send_flag = false;
        send_thread->join();
        send_thread = NULL;
    }
}

int UdpSocket::bind(QHostAddress addr, unsigned short port)
{
    // socket create and bind
    sockfd = ::socket(AF_INET, SOCK_DGRAM, 0);

    // set server ip, port (my)
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    //server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    inet_pton(AF_INET, addr.toString().toLocal8Bit().data(), &(server_addr.sin_addr));
    server_addr.sin_port = htons(port);

    // bind server socket
    if(::bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1)
    {
        printf("socket bind failed\n");
        return 0; // err?
    }

    // loop start
    if (recv_thread == NULL)
    {
        recv_flag = true;
        recv_thread = new std::thread(&UdpSocket::recv_loop, this);
    }

    if (send_thread == NULL)
    {
        send_flag = true;
        send_thread = new std::thread(&UdpSocket::send_loop, this);
    }

    return 1;
}

void UdpSocket::recv_loop()
{
    char buf[1500] = {0,};
    while(recv_flag)
    {
        timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(sockfd, &fds);
        int res = ::select(sockfd+1, &fds, 0, 0, &timeout);
        if(res > 0)
        {
            struct sockaddr_in client_addr;
            memset(&client_addr, 0, sizeof(client_addr));
            socklen_t len = sizeof(client_addr);
            int n = ::recvfrom(sockfd, (char*)buf, 1500, MSG_WAITALL, (struct sockaddr*)&client_addr, &len);
            if(n > 0)
            {
                printf("recv_something\n");
                senderInfo_with_data sender;
                sender.address = QHostAddress((struct sockaddr*)&client_addr);
                sender.port    = client_addr.sin_port;
                sender.buf     = QByteArray::fromRawData(buf, n);

                buf_que.push(sender);
                if(buf_que.unsafe_size() > 50)
                {
                    senderInfo_with_data dummy;
                    buf_que.try_pop(dummy);
                }
            }
        }
    }

    printf("recv_thread dead\n");

}

void UdpSocket::writeDatagram(const QByteArray &datagram, const QHostAddress &host, quint16 port)
{
    senderInfo_with_data msg;
    msg.address = host;
    msg.port = port;
    msg.buf = datagram;

    send_que.push(msg);
    if(send_que.unsafe_size() > 50)
    {
        senderInfo_with_data dummy;
        send_que.try_pop(dummy);
    }
}

void UdpSocket::send_loop()
{
    while(send_flag)
    {
        senderInfo_with_data msg;
        if(send_que.try_pop(msg))
        {
            // set client ip, port (remote)
            struct sockaddr_in client_addr;
            memset(&client_addr, 0, sizeof(client_addr));

            client_addr.sin_family = AF_INET;
            client_addr.sin_port = htons(msg.port);
            inet_pton(AF_INET, msg.address.toString().toLocal8Bit().data(), &(client_addr.sin_addr));

            ::sendto(sockfd, msg.buf.data(), msg.buf.length(), MSG_CONFIRM, (const struct sockaddr*)&client_addr, sizeof(client_addr));
            continue;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    printf("send_thread dead\n");
}
