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
    explicit UDPCOMM(qint16 _my_port);
    explicit UDPCOMM();
    ~UDPCOMM();

    void add_send_address(QHostAddress _address, qint16 _port);
    void set_my_interface(QNetworkInterface type);

    void write_dataGram(SEND_STRUCT str);
    RECEIVE_STRUCT get_received_struct();

    std::vector<senderInfo> senders_info;

private:
    MyUdpSocket udp;
    std::mutex mtx;

    std::atomic<bool> callbackFlag;
    std::thread * callbackThread = NULL;
    void callbackLoop();

    void add_head_tail(QByteArray &data);
    bool find_my_address(QHostAddress &_receive_address, QNetworkInterface _type);

    int usage = USAGE_NONE;

    QHostAddress send_address;
    qint16 send_port;
    size_t send_struct_size;
    SEND_STRUCT send_struct;

    QHostAddress my_address;
    qint16 my_port;
    QNetworkInterface my_interface = QNetworkInterface::Wifi;

    size_t receive_struct_size;
    RECEIVE_STRUCT receive_struct;
    QByteArray receive_buffer;
    RECEIVE_STRUCT real_receive_struct;

    char packet_head[2] = {(char)0xFF, (char)0xFD};
    char packet_tail[2] = {(char)0x01, (char)0x02};
};


////////////////////////////////////////////////////////
///
/// UDPCOMM class definition
/// template class
///
////////////////////////////////////////////////////////
template <typename SEND_STRUCT, typename RECEIVE_STRUCT>
UDPCOMM<SEND_STRUCT, RECEIVE_STRUCT>::UDPCOMM(qint16 _my_port) :
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

    qDebug() << "[UDP] bind address " << my_address << " port " << my_port;
    usage = USAGE_RECEIVER;
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
bool UDPCOMM<SEND_STRUCT, RECEIVE_STRUCT>::find_my_address(QHostAddress &_receive_address, QNetworkInterface _type)
{
    bool flag = false;
    for(auto interface:QNetworkInterface::allInterfaces())
    {
        for(QNetworkAddressEntry address:interface.addressEntries())
        {
            if(interface.type() == _type)
            {
                printf("bind wifi address : %s Active Echo sender.\n", QString(address.ip().toString()).toStdString().c_str());
                _receive_address = address.ip();
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

    qDebug() << "add address " << _address << " " << _port;
}

template <typename SEND_STRUCT, typename RECEIVE_STRUCT>
void UDPCOMM<SEND_STRUCT, RECEIVE_STRUCT>::write_dataGram(SENDER str)
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

    add_head_tail(serialization_bytes);
    for(size_t i=0; i<senders_info.size(); ++i)
    {
        QHostAddress address = senders_info[i].address;
        qint16 port = senders_info[i].port;
        //qDebug() << "[RECEIVER] echo data to " << address  << " port " << port;
        udp.socket.writeDatagram(serialization_bytes, address, port);
    }
}

template <typename SEND_STRUCT, typename RECEIVE_STRUCT>
void UDPCOMM<SEND_STRUCT, RECEIVE_STRUCT>::callbackLoop()
{
    if(send_struct_size == 0 || receive_struct_size == 0)
    {
        printf("struct size is zero.\n");
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
                    if(receive_buffer[p] == (char)0xFF && receive_buffer[p+1] == (char)0xFD)
                    {
                        receive_buffer.remove(0, p);
                        is_header = true;
                        break;
                    }
                }

                const int packet_size = int(receive_struct_size) + 4;
                if(is_header && receive_buffer.size() >= packet_size)
                {
                    if(receive_buffer[packet_size-2] == (char)0x01 && receive_buffer[packet_size-1] == (char)0x02)
                    {
                        uchar* body = (uchar*)receive_buffer.data();
                        memcpy(&receive_struct, &body[2], int(receive_struct_size));
                        receive_buffer.remove(0, packet_size);

                        mtx.lock();
                        real_receive_struct = receive_struct;
                        mtx.unlock();
                    }
                }
            }
        }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
}

template <typename SEND_STRUCT, typename RECEIVE_STRUCT>
RECEIVE_STRUCT UDPCOMM<SEND_STRUCT, RECEIVE_STRUCT>::get_received_struct()
{
    mtx.lock();
    RECEIVE_STRUCT _str = real_receive_struct;
    mtx.unlock();

    return _str;
}
