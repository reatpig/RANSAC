#include "RANSAC.h"
#include <QtGui>
#include <QFileDialog>
#include <QMessageBox>
#include <map>



RANSAC::RANSAC(QWidget* parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);

    ui.graph->setInteraction(QCP::iRangeDrag, true);
    ui.graph->setInteraction(QCP::iRangeZoom, true);


    ui.graph->addGraph();
    ui.graph->graph(0)->setAdaptiveSampling(false);
    ui.graph->graph(0)->removeFromLegend();
    ui.graph->graph(0)->setLineStyle(QCPGraph::lsNone);
    ui.graph->graph(0)->setScatterStyle(QCPScatterStyle::ssDisc);
    ui.graph->graph(0)->setPen(QPen(QBrush(Qt::gray), 4));

    ui.graph->addGraph();
    ui.graph->graph(1)->setAdaptiveSampling(false);
    ui.graph->graph(1)->removeFromLegend();
    ui.graph->graph(1)->setLineStyle(QCPGraph::lsNone);
    ui.graph->graph(1)->setScatterStyle(QCPScatterStyle::ssDisc);
    ui.graph->graph(1)->setPen(QPen(QBrush(Qt::blue), 4));

    infLine = new QCPItemStraightLine(ui.graph);
    infLine->setVisible(false);
    infLine->setPen(QPen(QBrush(Qt::blue), 4));

    connect(ui.graph, SIGNAL(mousePress( QMouseEvent * )),
        this, SLOT(onMousePress( QMouseEvent * )));

    timer = new QTimer(this);
    ui.timer->setText("00:00:000");
    connect(timer, SIGNAL(timeout()), this, SLOT(slotTimerAlarm()));

    udp = new MyUDP;

    allThread.resize(MAX_THREAD);
    allCalculation.resize(MAX_THREAD);

    ui.equation->setText("");
    
    for (int i = 0; i < MAX_THREAD; ++i) {
        allThread[i] = new QThread(this);
        allCalculation[i] = new Calculation(this);
        allCalculation[i]->moveToThread(allThread[i]);
        connect(allThread[i], SIGNAL(QThread::finished), allThread[i], SLOT(deleteLater()));

        connect(allCalculation[i], SIGNAL( workDone(unsigned int,double, double)),
            this,SLOT(handleResults(unsigned int, double ,double)));
        connect(this,SIGNAL( operate( int , QCPDataContainer<QCPGraphData>const*,double)), 
            allCalculation[i], SLOT( doWork(int, QCPDataContainer<QCPGraphData>const*, double)));
      
    }
}
void RANSAC::slotTimerAlarm()
{
 //Timer's work
    timerTime++;
    QTime time = QTime::fromMSecsSinceStartOfDay(timerTime); 
    ui.timer->setText(time.toString("mm:ss:zzz"));
}
RANSAC::~RANSAC()
{
    
}
//Calculate threshold with MAD
 long double calcThreshold(QCPGraphDataContainer* data) {
    //Median
    std::vector< long double> sortedData;
    sortedData.reserve(data->size());
    for (auto& point : *data) {
         long double x = point.key, y = point.value;
        sortedData.push_back(x * x + y * y);
    }
    sort(sortedData.begin(), sortedData.end());

     long double mid = sortedData[sortedData.size() / 2];
    for (auto& number : sortedData) {
        number =abs(number- mid);
    }
    sort(sortedData.begin(), sortedData.end());
     return sqrt(sortedData[sortedData.size() / 2]) / 15;;
}
void RANSAC::loadFromFile()
{ 
    QString filesName = QFileDialog::getOpenFileName(
        this,
        tr("Open file"),
        "C://",
        "Text files (*.txt);; All files (*.*)"
    );
    
    QFile file(filesName, this);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::information(this, tr("Unable to open file"),
            file.errorString());
        return;
    }
    //Clear 
    ui.timer->setText("00:00:000");
    ui.equation->setText("");
    ui.graph->graph(1)->data().data()->clear();
    infLine->setVisible(false);
    ui.graph->graph(0)->setPen(QPen(QBrush(Qt::gray), 4));
    ui.graph->graph(0)->data()->clear();

    QDataStream in(&file);
    while (!file.atEnd()) {
        QString line ( file.readLine());
        auto splitedLine= QStringRef (&line, 1, line.size() - 3).split(',');
        bool ok;
       int x= splitedLine[0].toDouble(&ok);
       int y = splitedLine[1].toDouble(&ok);
       if (!ok) {
           QMessageBox::information(this, tr("Data corrupted"),
               file.errorString());
           return;
       }
       ui.graph->graph(0)->addData(x, y);
    }
    file.close();

    //Set data range
    bool ok;
    auto rangeX = ui.graph->graph(0)->data()->keyRange(ok);
    auto rangeY = ui.graph->graph(0)->data()->valueRange(ok);
    ui.graph->xAxis->setRange(rangeX);
    ui.graph->yAxis->setRange(rangeX);

    ui.graph->replot();
    threshold = calcThreshold(ui.graph->graph(0)->data().data());

}
void RANSAC::saveToFile() {
    QString filesName = QFileDialog::getSaveFileName(
        this,
        tr("Save file"),
        "C://",
        "Text files (*.txt);; All files (*.*)"
    );

    QFile file(filesName, this);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::information(this, tr("Unable to open file"),
            file.errorString());
        return;
    }

    const auto dataMap = ui.graph->graph(0)->data().data();
    QTextStream in(&file);
    for (auto& iter : *dataMap) {
        in << '(' << iter.key << ", " << iter.value << ')' << '\n';
    }
    file.close();
}

void RANSAC::onMousePress(QMouseEvent* event)
{
    if (event->button() == Qt::RightButton)
    {
        if (ui.graph->axisRect()->rect().contains(event->pos()))
        {
            //Delete point
            double x = ui.graph->xAxis->pixelToCoord(event->x());
            double y = ui.graph->yAxis->pixelToCoord(event->y());
            bool thereIs = false;
            auto dataMap = ui.graph->graph(0)->data().data();
            auto closetPoint = dataMap->findBegin(x);

            if (abs(closetPoint->key - x) < 12 && abs(closetPoint->value - y) < 12) {
                dataMap->remove(closetPoint->key);
                thereIs = true;
            }
            dataMap = ui.graph->graph(1)->data().data();
            closetPoint = dataMap->findBegin(x);
            if (abs(closetPoint->key - x) < 12 && abs(closetPoint->value - y) < 12) {
                dataMap->remove(closetPoint->key);
                thereIs = true;
            }
            if(!thereIs) //Add point
                ui.graph->graph(0)->addData(x, y);
        }
        ui.graph->replot();
    }
}

bool  isWithinThreshold(const QPointF& a, const QPointF& b, const QPointF& p,double threshold) {
    double numerator = abs((b.y() - a.y()) * p.x() - (b.x() - a.x()) * p.y() + b.x() * a.y() - b.y() * a.x());
    double denominator = sqrt(pow(b.y() - a.y(), 2) + pow(b.x() - a.x(), 2));
    if (numerator / denominator < threshold) {
        return true;
    }
    else return false;
}

void RANSAC::startRANSAC()
{
    if (ui.graph->graph(0)->data()->isEmpty())
        return;
    if (working)
        return;

    for (int i = 0; i <( multithreading ? MAX_THREAD : 1); ++i)
        allThread[i]->start();

    working = true;
    //Number of iterations
    unsigned int k = log(1 - 0.99) / log(1 - (1 - 0.6) * (1 - 0.6));
    ui.graph->graph(1)->data()->clear();
    timer->start(1.75);
    if (multithreading)
        emit operate(ceil((float)k / MAX_THREAD+5), ui.graph->graph(0)->data().data(), threshold);
    else  
        emit operate(k, ui.graph->graph(0)->data().data(), threshold);
}


void RANSAC::handleResults(unsigned int score, double k, double b)
{
    static unsigned int bestScore = 0, count = 0;
    static double bestK = 0, bestB = 0;

    if (score > bestScore) {
        bestK = k;
        bestB = b;
    }
    //How threads end calculations
    count++;
    if (count >= (multithreading ? MAX_THREAD : 1)) {
        working = false;
        infLine->setVisible(true);
        infLine->point1->setCoords(0, bestB);  // location of best point 1  coordinate
        infLine->point2->setCoords(100, 100 * bestK + bestB);  // location of best point 2 coordinate

        ui.graph->graph(0)->setPen(QPen(QBrush(Qt::red), 4));
        ui.equation->setText("y=" + QString::number(bestK, 10, 3) + "x + " + QString::number(bestB, 10, 3));
        //Recolor points to blue
        auto data = ui.graph->graph(0)->data().data();
        auto totalN = data->size();
        QPointF currPoint, maybeInlierA, maybeInlierB;
        maybeInlierA.setX(0); maybeInlierA.setY(b);
        maybeInlierB.setX(100); maybeInlierB.setY(100 * k + b);
        for (unsigned p = 0; p <= totalN; p++) {
            currPoint = { data->at(p)->key , data->at(p)->value };
            if (isWithinThreshold(maybeInlierA, maybeInlierB, currPoint, threshold))
                ui.graph->graph(1)->addData(data->at(p)->key, data->at(p)->value);
        }
        for (int i = 0; i < (multithreading ? MAX_THREAD : 1); ++i)
            allThread[i]->quit();

        ui.graph->replot();
        udp->sendStraight(bestK, bestB);
        timer->stop();
        timerTime = 0;
        count = 0;

    }
}
void RANSAC::toggleThreads()
{
    if (!working) {
        multithreading = !multithreading;
    }
    else {
        ui.MultTreadChesk->setChecked(multithreading);
    }
}

void Calculation::doWork(int k, QCPDataContainer<QCPGraphData>const * data,double threshold) {
    QPointF maybeInlierA, maybeInlierB, currPoint, bestFitPointA, bestFitPointB;
    auto totalN = data->size();
    unsigned nInliersGlobalMax = 0;
    unsigned    nInliersCurr = 0;

    for (unsigned i = 0; i <= k; i++) {
        // randomly select n values from data
        unsigned n0 = QRandomGenerator::global()->generate()% totalN;
        unsigned n1 = QRandomGenerator::global()->generate() % totalN;
   
        maybeInlierA = { data->at(n0)->key , data->at(n0)->value };
        maybeInlierB = { data->at(n1)->key, data->at(n1)->value };
        nInliersCurr = 0;
      
        // iterate over the data points to find the number of points that are within threshold epsilon distance of the line fit for maybeInlierA and maybeInlierB
        for (unsigned p = 0; p <= totalN; p++) {
            currPoint = { data->at(p)->key , data->at(p)->value };
            if (isWithinThreshold(maybeInlierA, maybeInlierB, currPoint, threshold))
                // increase curr inliers count and set true in the inliers vector
                nInliersCurr += 1;
        }
        
        if (nInliersGlobalMax < nInliersCurr) {
            nInliersGlobalMax = nInliersCurr;
            // update best fit points A and B
            bestFitPointA = { data->at(n0)->key ,data->at(n0)->value };
            bestFitPointB = { data->at(n1)->key , data->at(n1)->value };
        }  
    }
    double cofK = (bestFitPointB.y() - bestFitPointA.y()) / (bestFitPointB.x() - bestFitPointA.x());
    double cofB = bestFitPointB.y() - cofK * bestFitPointB.x();
    emit workDone(nInliersGlobalMax, cofK, cofB);
}

MyUDP::MyUDP(QObject* parent) :
    QObject(parent)
{
    socket = new QUdpSocket(this);
   socket->bind(QHostAddress::LocalHost);
    connect(socket, SIGNAL(readyRead()), this, SLOT(readyRead()));
}
#
void MyUDP::sendStraight(double k,double b)
{
    QByteArray Data;
    Data.append(QString::number(k) + ' '+QString::number(b));  
    socket->writeDatagram(Data, QHostAddress::LocalHost, 4320);
}

Calculation::Calculation(QObject* parent)
{
}
