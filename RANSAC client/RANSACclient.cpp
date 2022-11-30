#include "RANSACclient.h"

MyUDP::MyUDP(QObject* parent) :
    QObject(parent)
{
    socket = new QUdpSocket(this);
    socket->bind(QHostAddress::LocalHost,4320);
    connect(socket, SIGNAL(readyRead()), this, SLOT(readyRead()));
}

void MyUDP::readyRead()
{
    QByteArray buffer;
    buffer.resize(socket->pendingDatagramSize());

    QHostAddress sender;
    quint16 senderPort;

    socket->readDatagram(buffer.data(), buffer.size(),
        &sender, &senderPort);

    double k = buffer.split(' ')[0].toDouble();
    double b = buffer.split(' ')[1].toDouble();
    emit  giveStraight(k, b);
}
RANSACclient::RANSACclient(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);
    udp = new MyUDP(this);
    ui.text->setText("");
    ui.Graph->setInteraction(QCP::iRangeDrag, true);
    ui.Graph->setInteraction(QCP::iRangeZoom, true);

    infLine = new QCPItemStraightLine(ui.Graph);
    infLine->setVisible(false);
    connect(udp, SIGNAL(giveStraight(double, double)), this, SLOT(getStraight(double, double)));

}
void RANSACclient::getStraight(double k, double b) {

    infLine->setVisible(true);
    infLine->point1->setCoords(0, b);  // location of point 1 in plot coordinate
    infLine->point2->setCoords(100, 100*k+b);  // location of point 2 in plot coordinate
    infLine->setPen(QPen(QBrush(Qt::blue), 4));
    ui.text->setText("y=" + QString::number(k, 10, 3) + "x + " + QString::number(b, 10, 3));
    ui.Graph->xAxis->setRange(-(100 * k + b), 100 * k + b);
    ui.Graph->yAxis->setRange(-(100 * k + b), 100 * k + b);
    ui.text->update();
    ui.Graph->replot();
}

RANSACclient::~RANSACclient()
{}
