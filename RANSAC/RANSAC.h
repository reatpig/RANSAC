#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_RANSAC.h"
#include <qset.h>

#include <QUdpSocket>
#include <QObject>
#include <QUdpSocket>

class MyUDP : public QObject
{
    Q_OBJECT
public:
    explicit MyUDP(QObject* parent = 0);
    void sendStraight(double k, double b);

private:
    QUdpSocket* socket;

};


class Calculation: public QObject {
    Q_OBJECT
public:
    explicit Calculation(QObject* parent = 0) ;
public slots:
  void doWork(int k, QCPDataContainer<QCPGraphData>const* data,double threshold);
signals:
    void workDone(unsigned int bestScore, double k, double b);
private:


};
class RANSAC : public QMainWindow
{
    Q_OBJECT

public:
    RANSAC(QWidget* parent = nullptr);
    ~RANSAC();
    MyUDP* udp;
signals:
    void operate(  int, QCPDataContainer<QCPGraphData>const*,double);
public slots:

    void loadFromFile();
    void saveToFile();
    void onMousePress( QMouseEvent* event);
    void startRANSAC();
    void slotTimerAlarm();
    void toggleThreads();
    void handleResults(unsigned int, double, double);
private:
    bool multithreading =false;
    QCPItemStraightLine* infLine;
    Ui::RANSACClass ui;
    int timerTime = 0;
    QTimer* timer;
    QVector<QThread*> allThread;
    QVector<Calculation*> allCalculation;
    int threshold;
    const int MAX_THREAD = QThreadPool::globalInstance()->maxThreadCount()>1? QThreadPool::globalInstance()->maxThreadCount():2-1;
    bool working=false;
  
};

