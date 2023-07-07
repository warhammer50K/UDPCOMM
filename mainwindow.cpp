#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    init();



}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::init()
{
    // set sender (write datagram to receiver)
    UDPCOMM<STREAM, MOBILE> _joystick(qint16(3333));
    _joystick.set_my_interface(QNetworkInterface::Ethernet);

    UDPCOMM<MOBILE, STREAM> _mobile(qint16(2222));

    STREAM _stream;

    qDebug() << sizeof(_stream);
    _stream.a = 1.0;
    _joystick.write_dataGram(_stream);

    // set receiver (echo datagram to sender)

    MOBILE _m;
    _mobile.set_echo_dataGram(_m);
}
