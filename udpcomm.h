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

    UDPCOMM(const UDPCOMM& src) = default;
    UDPCOMM<STRUCT>& operator=(const UDPCOMM& rhs) = default;

    UDPCOMM(UDPCOMM&& src) = default;
    UDPCOMM<STRUCT>& operator=(UDPCOMM&& rhs) = default;

    QUdpSocket socket;

    void set_address_port(QHostAddress _address, qint16 _port);
    void write_dataGram(STRUCT str);


private:
    int udp_usage = NONE;
    QHostAddress udp_address;
    qint16 udp_port;
    STRUCT _struct;
    size_t struct_size;
};

template <typename STRUCT>
UDPCOMM<STRUCT>::UDPCOMM(int _usage, QHostAddress _address, qint16 _port) :
    udp_usage(_usage),
    udp_address(_address),
    udp_port(_port)
{
    STRUCT t;
    struct_size = sizeof(t);
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
    QByteArray serialization_bytes;

    size_t struct_size = sizeof(str);
    serialization_bytes.resize(struct_size);
    memcpy(serialization_bytes.data(), &str, struct_size);

    socket.writeDatagram(serialization_bytes, udp_address, udp_port);
}

