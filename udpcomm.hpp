#pragma once

// qt
#include <QtGamepad/QGamepad>
#include <QTimer>

// stl
#include <thread>
#include <mutex>
#include <atomic>
#include <iostream>

//my
#include "global_defines.h"
#include "myudpsocket.h"

template <typename SEND_STRUCT, typename RECEIVE_STRUCT>
class UDPCOMM
{
public:
    explicit UDPCOMM(QNetworkInterface::InterfaceType _my_interface, qint16 _my_port);
    explicit UDPCOMM();
    ~UDPCOMM();

    void add_send_address(QHostAddress _address, qint16 _port);
    void write_dataGram(SEND_STRUCT str);
    RECEIVE_STRUCT get_received_struct();

private:

    MyUdpSocket udp;
    std::mutex mtx;

    QHostAddress my_address;
    qint16 my_port;
    QNetworkInterface::InterfaceType my_interface;

    std::vector<senderInfo> senders_info;
    size_t send_struct_size;

    size_t receive_struct_size;
    QByteArray receive_buffer;
    RECEIVE_STRUCT receive_struct;

    const char packet_head[2] = {(char)0xFF, (char)0xFD};
    const char packet_tail[2] = {(char)0xFB, (char)0xFA};

    void add_head_tail(QByteArray &data);
    bool find_my_address(QHostAddress &_receive_address, QNetworkInterface::InterfaceType _type);

    std::atomic<bool> callbackFlag;
    std::thread * callbackThread = NULL;
    void callbackLoop();
};


////////////////////////////////////////////////////////
///
/// UDPCOMM class definition
/// template class
///
////////////////////////////////////////////////////////
template <typename SEND_STRUCT, typename RECEIVE_STRUCT>
UDPCOMM<SEND_STRUCT, RECEIVE_STRUCT>::UDPCOMM(QNetworkInterface::InterfaceType _my_interface, qint16 _my_port) :
    my_interface(_my_interface),
    my_port(_my_port)
{
    // set bind address to cur Wifi network address
    bool flag = find_my_address(my_address, my_interface);
    if(flag == false)
    {
        printf("[FAILED] failed bind ip.\n");
        return;
    }

    // socket biding wifi address and port
    if(!udp.socket.bind(my_address, my_port))
    {
        printf("[FAILED] failed socket binding.\n");
        return;
    }

    SEND_STRUCT T;
    send_struct_size = sizeof(T);

    RECEIVE_STRUCT R;
    receive_struct_size = sizeof(R);

    // start callback loop
    if (callbackThread == NULL)
    {
        callbackFlag = true;
        callbackThread = new std::thread(&UDPCOMM::callbackLoop, this);
    }
}

template <typename SEND_STRUCT, typename RECEIVE_STRUCT>
UDPCOMM<SEND_STRUCT, RECEIVE_STRUCT>::~UDPCOMM()
{
    if(callbackThread != NULL)
    {
        callbackFlag = false;
        callbackThread->join();
        callbackThread = NULL;
    }
}

template <typename SEND_STRUCT, typename RECEIVE_STRUCT>
bool UDPCOMM<SEND_STRUCT, RECEIVE_STRUCT>::find_my_address(QHostAddress &_receive_address, QNetworkInterface::InterfaceType _type)
{
    bool flag = false;
    for(auto interface:QNetworkInterface::allInterfaces())
    {
        for(QNetworkAddressEntry address:interface.addressEntries())
        {
            if(interface.type() == _type)
            {
                _receive_address = address.ip();

                printf("[REGISTRATION] bind address: %s\n", address.ip().toString().toStdString().c_str());
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

    return flag;
}

template <typename SEND_STRUCT, typename RECEIVE_STRUCT>
void UDPCOMM<SEND_STRUCT, RECEIVE_STRUCT>::add_send_address(QHostAddress _address, qint16 _port)
{
    senderInfo _send;
    _send.address = _address;
    _send.port = _port;
    senders_info.push_back(_send);

    printf("[REGISTRATION] add address: %s, port: %d\n", _address.toString().toStdString().c_str(), (int)_port);
}

template <typename SEND_STRUCT, typename RECEIVE_STRUCT>
void UDPCOMM<SEND_STRUCT, RECEIVE_STRUCT>::write_dataGram(SEND_STRUCT str)
{
    if(senders_info.size() == 0)
    {
        printf("[FAILED] senders_info size is zero.\n");
        return;
    }

    QByteArray serialization_bytes;
    size_t struct_size = send_struct_size;
    serialization_bytes.resize(struct_size);
    memcpy(serialization_bytes.data(), &str, struct_size);

    QByteArray serialization_bytes_split;
    serialization_bytes_split.resize(struct_size*2);

    for(int i = 0; i < serialization_bytes.count(); ++i)
    {
        char lowByte = (serialization_bytes[i] & (char)0x0F);
        char highByte = (serialization_bytes[i] &(char)0xF0);

        serialization_bytes_split[2*i] = lowByte;
        serialization_bytes_split[2*i+1] = highByte;
    }

    add_head_tail(serialization_bytes_split);

    for(size_t i=0; i<senders_info.size(); ++i)
    {
        QHostAddress address = senders_info[i].address;
        qint16 port = senders_info[i].port;
        udp.socket.writeDatagram(serialization_bytes_split, address, port);
    }
}

template <typename SEND_STRUCT, typename RECEIVE_STRUCT>
void UDPCOMM<SEND_STRUCT, RECEIVE_STRUCT>::callbackLoop()
{
    if(send_struct_size == 0 || receive_struct_size == 0)
    {
        printf("[FAILED] base struct size is zero.\n");
        return;
    }

    while(callbackFlag)
    {
        senderInfo_with_data _sender;
        if(udp.buf_que.try_pop(_sender))
        {
            senderInfo _send;
            _send.address = _sender.address;
            _send.port= _sender.port;

            if(senders_info.size() == 0)
            {
                senders_info.push_back(_send);
            }
            else
            {
                bool flag = true;
                for(size_t i = 0; i < senders_info.size(); ++i)
                {
                    if(senders_info[i].address == _sender.address)
                    {
                        flag = false;
                    }
                }

                if(flag == true)
                {
                    senders_info.push_back(_send);
                }
            }

            QByteArray _buf = _sender.buf;
            if(_buf.size() > 0)
            {
                receive_buffer.append(_buf);
                if(receive_buffer.size() < 2)
                {
                    return;
                }

                bool is_header = false;
                for(int p = 0; p < receive_buffer.size()-1; p++)
                {
                    // header check
                    if(receive_buffer[p] == packet_head[0] && receive_buffer[p+1] == packet_head[1])
                    {
                        receive_buffer.remove(0, p);
                        is_header = true;
                        break;
                    }
                }

                const int packet_size = int(receive_struct_size * 2) + 4;
                if(is_header && receive_buffer.size() >= packet_size)
                {
                    if(receive_buffer[packet_size-2] == packet_tail[0] && receive_buffer[packet_size-1] == packet_tail[1])
                    {
                        QByteArray combine_buffer;
                        combine_buffer.resize(receive_struct_size);
                        for(int i=0; i<combine_buffer.count(); ++i)
                        {
                            char lowByte = (receive_buffer[2*i]);
                            char highByte = (receive_buffer[2*i+1]);

                            char val = lowByte + highByte;
                            combine_buffer[i] = val;
                        }

                        char* body = (char*)combine_buffer.data();
                        memcpy(&receive_struct, &body[0], int(receive_struct_size));
                        receive_buffer.remove(0, packet_size);
                    }
                }
            }
        }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
}

template <typename SEND_STRUCT, typename RECEIVE_STRUCT>
void UDPCOMM<SEND_STRUCT, RECEIVE_STRUCT>::add_head_tail(QByteArray &data)
{
    data.push_front(packet_head[1]);
    data.push_front(packet_head[0]);

    data.push_back(packet_tail[0]);
    data.push_back(packet_tail[1]);
}

template <typename SEND_STRUCT, typename RECEIVE_STRUCT>
RECEIVE_STRUCT UDPCOMM<SEND_STRUCT, RECEIVE_STRUCT>::get_received_struct()
{
    mtx.lock();
    RECEIVE_STRUCT _str = receive_struct;
    mtx.unlock();

    return _str;
}
