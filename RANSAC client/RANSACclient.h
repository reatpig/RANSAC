#pragma once
#include <QtWidgets/QMainWindow>

#include "ui_RANSACclient.h"
#include <QUdpSocket>
#include <QObject>
#include <QUdpSocket>

class MyUDP : public QObject
{
    Q_OBJECT
public:
    explicit MyUDP(QObject* parent = 0);
signals:
    void giveStraight(double k, double b);
public slots:
    void readyRead();
        
private:
    QUdpSocket* socket;
    
};

class RANSACclient : public QMainWindow
{
    Q_OBJECT
public:
    RANSACclient(QWidget *parent = nullptr);
    ~RANSACclient();
    MyUDP* udp;
public slots: 
    void getStraight(double k, double b);
private:
    Ui::RANSACclientClass ui;
  
    QCPItemStraightLine* infLine;
  
};
