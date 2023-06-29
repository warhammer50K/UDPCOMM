#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    //UDPCOMM qwer;
    //qwer.setStruct(STREAM a);
    UDPCOMM<STREAM> _udp(SERVER, QHostAddress(QString("10.108.1.22")), qint16(2222));
}

MainWindow::~MainWindow()
{
    delete ui;
}

