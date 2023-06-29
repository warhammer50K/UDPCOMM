#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);


}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::init()
{
    UDPCOMM<STREAM> _udp(SERVER, QHostAddress(QString("10.108.1.22")), qint16(2222));

    STREAM _stream;

    _stream.a = 1.0;
    _udp.write_dataGram(_stream);


}
