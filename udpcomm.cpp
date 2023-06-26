#include "udpcomm.h"

UDPCOMM::UDPCOMM(QObject *parent) : QObject(parent)
{
    connect(&socket, SIGNAL(readyRead()), this, SLOT(readyRead()));
    connect(&udp_timer, SIGNAL(timeout()), this, SLOT(udp_loop()));

    init();
}


void UDPCOMM::init()
{
    // set address
    const QHostAddress &localhost = QHostAddress(QHostAddress::LocalHost);
    for (const QHostAddress &_address: QNetworkInterface::allAddresses())
    {
        if (_address.protocol() == QAbstractSocket::IPv4Protocol && _address != localhost)
        {
            std::cout << "Address : " << _address.toString().toStdString() << std::endl;
            address = _address;
            break;
        }
    }

    // set gamepad
    auto gamepads = QGamepadManager::instance()->connectedGamepads();
    if(!gamepads.isEmpty())
    {
        gamepad = new QGamepad(*gamepads.begin(), this);
    }
}

void UDPCOMM::init(QHostAddress _address, quint16 _port)
{
    // set address and port
    address = _address;
    port = _port;

    // set gamepad
    auto gamepads = QGamepadManager::instance()->connectedGamepads();
    if(!gamepads.isEmpty())
    {
        gamepad = new QGamepad(*gamepads.begin(), this);
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
    if(address.isNull())
    {
        printf("address is empty\n");
        return;
    }

    stream _stream;
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

    data.append((char)0xFF);
    data.append((char)0xFD);

    _stream.serialization(buf);

    data.append((char)0x01);
    data.append((char)0x02);

    socket.writeDatagram(data, address, port);

    //QString str;
    //str.sprintf("lx:%.2f, ly:%.2f, rx:%.2f, ry:%.2f\n"
    //            "x:%d, y:%d, a:%d, b:%d\n"
    //            "l1:%d, l2:%d, r1:%d, r2:%d", lx, ly, rx, ry,
    //            x, y, a, b,
    //            l1, l2, r1, r2);
    //std::cout << str.toStdString() << std::endl;
}

void UDPCOMM::readyRead()
{
    // echo
    if(socket.hasPendingDatagrams())
    {
        QByteArray _buf;
        _buf.resize(socket.pendingDatagramSize());

        QHostAddress sender; quint16 senderPort;
        socket.readDatagram(_buf.data(), _buf.size(), &sender, &senderPort);

        if(_buf.size() > 0)
        {
            buf.append(_buf);

            if(buf.size() < 2)
            {
                return;
            }

            bool is_header = false;
            for(int p = 0; p < buf.size()-1; p++)
            {
                // header check
                if(buf[p] == (char)0xFF && buf[p+1] == (char)0xFD)
                {
                    buf.remove(0, p);
                    is_header = true;
                    break;
                }
            }

            // 11 3*4 +8 20 25
            const int packet_size = 0;
            if(is_header && buf.size() >= packet_size)
            {
                if(buf[packet_size-2] == (char)0x01 && buf[packet_size-1] == (char)0x02)
                {
                    // do something
                }
            }
        }
    }}
