#ifndef MYUDPSOCKET_H
#define MYUDPSOCKET_H

// qt
#include <QObject>
#include <QUdpSocket>
#include <QDataStream>
#include <QNetworkInterface>

// stl
#include <tbb/concurrent_queue.h>
#include <tbb/concurrent_vector.h>

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


class MyUdpSocket : public QObject
{
    Q_OBJECT
public:
    explicit MyUdpSocket(QObject *parent = nullptr);

    QUdpSocket socket;
    tbb::concurrent_queue<senderInfo_with_data> buf_que;

signals:

private slots:
    void udp_receive_callback();
};

#endif // MYUDPSOCKET_H
