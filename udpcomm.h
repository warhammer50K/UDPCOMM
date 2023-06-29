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

#include "udp_struct.h"

enum UDP_USAGE
{
    NONE=-1,
    SERVER=0,
    CLIENT
};

template <typename STRUCT>
class UDPCOMM
{
public:
    explicit UDPCOMM(int _usage, QHostAddress _address, qint16 _port);
    virtual ~UDPCOMM() = default;

    /*UDPCOMM(const UDPCOMM& src) = default;
    UDPCOMM<STRUCT>& operator=(const UDPCOMM& rhs) = default;

    UDPCOMM(UDPCOMM&& src) = default;
    UDPCOMM<STRUCT>& operator=(UDPCOMM&& rhs) = default;*/

    QUdpSocket socket;

    void set_address_port(QHostAddress _address, qint16 _port);
    void write_dataGram(STRUCT str);

    void client_callback();


private:
    int udp_usage = NONE;
    QHostAddress udp_address;
    qint16 udp_port;
    STRUCT _struct;
    size_t struct_size;

    // client
    QByteArray client_buf;
};

template <typename STRUCT>
UDPCOMM<STRUCT>::UDPCOMM(int _usage, QHostAddress _address, qint16 _port) :
    udp_usage(_usage),
    udp_address(_address),
    udp_port(_port)
{
    STRUCT t;
    struct_size = sizeof(t);

    if(udp_usage == SERVER)
    {

    }
    else if(udp_usage == CLIENT)
    {

    }
}

template <typename STRUCT>
void UDPCOMM<STRUCT>::set_address_port(QHostAddress _address, qint16 _port)
{
    udp_address = _address;
    udp_port = _port;
}

template <typename STRUCT>
void UDPCOMM<STRUCT>::write_dataGram(STRUCT str)
{
    if(udp_usage == CLIENT)
    {
        printf("this function activate at [SERVER].\n");
        return;
    }

    QByteArray serialization_bytes;

    size_t struct_size = sizeof(str);
    serialization_bytes.resize(struct_size);
    memcpy(serialization_bytes.data(), &str, struct_size);

    socket.writeDatagram(serialization_bytes, udp_address, udp_port);
}


// client

template <typename STRUCT>
void UDPCOMM<STRUCT>::client_callback()
{
    if(udp_usage == SERVER)
    {
        printf("this function activate at [CLIENT].\n");
        return;
    }

    if(struct_size == 0)
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
            client_buf.append(_buf);

            if(client_buf.size() < 2)
            {
                return;
            }

            bool is_header = false;
            for(int p = 0; p < client_buf.size()-1; p++)
            {
                // header check
                if(client_buf[p] == (char)0xFF && client_buf[p+1] == (char)0xFD)
                {
                    client_buf.remove(0, p);
                    is_header = true;
                    break;
                }
            }

            const int packet_size = int(struct_size);
            if(is_header && client_buf.size() >= packet_size)
            {
                if(client_buf[packet_size-2] == (char)0x01 && client_buf[packet_size-1] == (char)0x02)
                {
                    uchar* body = (uchar*)client_buf.data();
                    memcpy(&_struct, &body[2], packet_size);
                    client_buf.remove(0, packet_size);
                }
            }
        }
    }
}

