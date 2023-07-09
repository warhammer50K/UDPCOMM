#include "myudpsocket.h"

MyUdpSocket::MyUdpSocket(QObject *parent) : QObject(parent)
{
    connect(&socket, SIGNAL(readyRead()), this, SLOT(udp_receive_callback()));
}

void MyUdpSocket::udp_receive_callback()
{
    if(socket.hasPendingDatagrams())
    {
        QByteArray _buf;
        _buf.resize(socket.pendingDatagramSize());

        QHostAddress _sender_address; quint16 _sender_port;
        socket.readDatagram(_buf.data(), _buf.size(), &_sender_address, &_sender_port);

        senderInfo_with_data sender;
        sender.address = _sender_address;
        sender.port    = _sender_port;
        sender.buf     = _buf;

        buf_que.push(sender);
        if(buf_que.unsafe_size() > 50)
        {
            senderInfo_with_data dummy;
            buf_que.try_pop(dummy);
        }
    }
}
