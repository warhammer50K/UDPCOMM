#include "udpcomm.h"

UDPCOMM::UDPCOMM(QObject *parent) : QObject(parent)
{
}

void UDPCOMM::init(int usage, QHostAddress _address, quint16 _port)
{
    if(_address.isNull())
    {
        printf("address is incorrect.\n");
        return;
    }

    address = _address;
    port = _port;

    if(usage == SERVER)
    {
        printf("udp server start.\n");

        connect(&udp_timer, SIGNAL(timeout()), this, SLOT(udp_loop()));
        udp_timer.start(100);

        // set gamepad
        auto gamepads = QGamepadManager::instance()->connectedGamepads();
        if(!gamepads.isEmpty())
        {
            gamepad = new QGamepad(*gamepads.begin(), this);
        }
    }

    else if(usage == CLIENT)
    {
        printf("udp client start.\n");

        connect(&socket, SIGNAL(readyRead()), this, SLOT(readyRead()));
        socket.bind(address, port);
    }

    else
    {
        printf("set udp server or client.\n");
    }
}

void UDPCOMM::set_address_port(int usage, QHostAddress _address, quint16 _port)
{
    address = _address;
    port = _port;

    if(usage == CLIENT)
    {
        socket.bind(address, port);
    }
}

void UDPCOMM::start()
{
    udp_timer.start(100);
}

void UDPCOMM::stop()
{
    udp_timer.stop();
}

void UDPCOMM::udp_loop()
{
    STREAM _stream;
    _stream.lx = gamepad->axisLeftX();
    _stream.ly = gamepad->axisLeftY();
    _stream.rx = gamepad->axisRightX();
    _stream.ry = gamepad->axisRightY();
    _stream.x = gamepad->buttonX();
    _stream.y = gamepad->buttonY();
    _stream.a = gamepad->buttonB();
    _stream.b = gamepad->buttonA();
    _stream.l1 = gamepad->buttonL1();
    _stream.l2 = gamepad->buttonL2();
    _stream.r1 = gamepad->buttonR1();
    _stream.r2 = gamepad->buttonR2();

    QByteArray data;
    QDataStream buf(&data, QIODevice::ReadWrite);
    buf << _stream;

    // set head & tail
    data.push_front(char(0xFF));
    data.push_front(char(0xFD));
    data.push_back(char(0x01));
    data.push_back(char(0x02));

    socket.writeDatagram(data, address, port);
}

void UDPCOMM::readyRead()
{
    // parsing data
    if(socket.hasPendingDatagrams())
    {
        QByteArray _data;
        _data.resize(socket.pendingDatagramSize());

        QHostAddress sender; quint16 senderPort;
        socket.readDatagram(_data.data(), _data.size(), &sender, &senderPort);

        if(_data.size() > 0)
        {
            data.append(_data);

            if(data.size() < 2)
            {
                return;
            }

            bool is_header = false;
            for(int p = 0; p < data.size()-1; p++)
            {
                // header check
                if(data[p] == (char)0xFF && data[p+1] == (char)0xFD)
                {
                    data.remove(0, p);
                    is_header = true;
                    break;
                }
            }

            const int packet_size = STREAM_SIZE + 4;
            if(is_header && data.size() >= packet_size)
            {
                if(data[packet_size-2] == (char)0x01 && data[packet_size-1] == (char)0x02)
                {
                    STREAM _stream;
                    QDataStream _buf(&data, QIODevice::ReadWrite);
                    _buf >> _stream;
                }
            }
        }
    }
}
