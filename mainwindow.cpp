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
}


void MainWindow::send_init()
{
    UDPCOMM<STREAM, MOBILE> _joystick(QNetworkInterface::Wifi, qint16(3333));
    _joystick.add_send_address(QHostAddress(QString("10.108.1.1")), 2222);

    STREAM _stream;
    _stream.a = 1.0;

    _joystick.write_dataGram(_stream);
}

void MainWindow::receive_init()
{
    UDPCOMM<MOBILE, STREAM> _mobile(QNetworkInterface::Wifi, qint16(2222));
    MOBILE mobile_status;
    mobile_status.a = 1.0;

    _mobile.write_dataGram(mobile_status);
}
