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
    UDPCOMM<STREAM, MOBILE> _joystick(QHostAddress(QString("10.108.1.22")), qint16(2222));
    UDPCOMM<MOBILE, STREAM> _mobile(qint16(2222));

    //STREAM _stream;

    //_stream.a = 1.0;
    //_joystick.write_dataGram(_stream);


}
