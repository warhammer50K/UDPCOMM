#ifndef UDPCOMM_H
#define UDPCOMM_H

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

class UDPCOMM : public QObject
{
    Q_OBJECT
public:
    explicit UDPCOMM(QObject *parent = nullptr);

    QTimer udp_timer;

    void init();
    void start();
    void stop();

signals:


private:
    QGamepad *gamepad = NULL;
    QUdpSocket socket;
    QHostAddress address;
    quint16 port = 9999;

private slots:
    void readyRead();
    void udp_loop();
};

#endif // UDPCOMM_H
