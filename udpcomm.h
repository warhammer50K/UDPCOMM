#pragma once

// qt
#include <QObject>
#include <QUdpSocket>
#include <QDataStream>
#include <QNetworkInterface>
#include <QtGamepad/QGamepad>
#include <QTimer>

// stl
#include <atomic>
#include <iostream>
#include <QObject>
#include "udp_struct.h"


template <typename SENDER, typename RECEIVER>
class UDPCOMM
{
public:
    explicit UDPCOMM(QHostAddress _receiver_address, qint16 _receiver_port);
    explicit UDPCOMM(qint16 _receiver_port);
    virtual ~UDPCOMM() = default;

    QUdpSocket socket;

    void set_address_port(QHostAddress _address, qint16 _port);
    void write_dataGram(SENDER str);

    void receiver_callback();

private:
    QHostAddress sender_address;
    qint16 sender_port;
    size_t sender_struct_size;
    SENDER sender_struct;

    QHostAddress receiver_address;
    qint16 receiver_port;
    size_t receiver_struct_size;
    RECEIVER receiver_struct;
    QByteArray receiver_buffer;
};

template <typename SENDER, typename RECEIVER>
UDPCOMM<SENDER, RECEIVER>::UDPCOMM(QHostAddress _receiver_address, qint16 _receiver_port) :
    sender_address(_receiver_address),
    sender_port(_receiver_port)
{
    printf("Active write_dataGram, Echo receiver.\n");
    SENDER T;
    sender_struct_size = sizeof(T);

    RECEIVER R;
    receiver_struct_size = sizeof(R);
}

template <typename SENDER, typename RECEIVER>
UDPCOMM<SENDER, RECEIVER>::UDPCOMM(qint16 _port) :
    sender_port(_port)
{
    QString active_address;
    bool flag = false;
    for(auto interface:QNetworkInterface::allInterfaces())
    {
        for(QNetworkAddressEntry address:interface.addressEntries())
        {
            if(address.ip() != QHostAddress::LocalHost
              && address.ip() == QHostAddress::AnyIPv4
              && interface.type() == QNetworkInterface::Wifi)
            {
                active_address = QString(address.ip().toString());
                sender_address = address.ip();
                flag = true;
            }
            if(flag == true)
            {
                break;
            }
        }
        if(flag == true)
        {
            break;
        }
    }

    if(flag == false)
    {
        printf("failed connect wifi.\n");
        return;
    }

    printf("bind wifi address : %s Active Echo sender.\n", active_address.toStdString().c_str());
    SENDER T;
    sender_struct_size = sizeof(T);

    RECEIVER R;
    receiver_struct_size = sizeof(R);
}


template <typename SENDER, typename RECEIVER>
void UDPCOMM<SENDER, RECEIVER>::set_address_port(QHostAddress _address, qint16 _port)
{
    sender_address = _address;
    sender_port = _port;
}

template <typename SENDER, typename RECEIVER>
void UDPCOMM<SENDER, RECEIVER>::write_dataGram(SENDER str)
{
    QByteArray serialization_bytes;

    size_t struct_size = sizeof(str);
    serialization_bytes.resize(struct_size);
    memcpy(serialization_bytes.data(), &str, struct_size);

    socket.writeDatagram(serialization_bytes, sender_address, sender_port);
}

template <typename SENDER, typename RECEIVER>
void UDPCOMM<SENDER, RECEIVER>::receiver_callback()
{
    if(sender_struct_size == 0 || receiver_struct_size == 0)
    {
        printf("struct size is zero.\n");
        return;
    }

    if(socket.hasPendingDatagrams())
    {
        QByteArray _buf;
        _buf.resize(socket.pendingDatagramSize());

        QHostAddress sender; quint16 senderPort;
        socket.readDatagram(_buf.data(), _buf.size(), &sender, &senderPort);

        if(_buf.size() > 0)
        {
            receiver_buffer.append(_buf);

            if(receiver_buffer.size() < 2)
            {
                return;
            }

            bool is_header = false;
            for(int p = 0; p < receiver_buffer.size()-1; p++)
            {
                // header check
                if(receiver_buffer[p] == (char)0xFF && receiver_buffer[p+1] == (char)0xFD)
                {
                    receiver_buffer.remove(0, p);
                    is_header = true;
                    break;
                }
            }

            const int packet_size = int(sender_struct_size);
            if(is_header && receiver_buffer.size() >= packet_size)
            {
                if(receiver_buffer[packet_size-2] == (char)0x01 && receiver_buffer[packet_size-1] == (char)0x02)
                {
                    uchar* body = (uchar*)receiver_buffer.data();
                    memcpy(&sender_struct, &body[2], packet_size);
                    receiver_buffer.remove(0, packet_size);
                }
            }
        }
    }
}

