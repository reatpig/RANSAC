#include "RANSAC.h"
#include <QtGui>
#include <QFileDialog>
#include <QMessageBox>
#include <map>
bool  isWithinThreshold(const QPointF& a, const QPointF& b, const QPointF& p, double threshold) {
    double numerator = abs((b.y() - a.y()) * p.x() - (b.x() - a.x()) * p.y() + b.x() * a.y() - b.y() * a.x());
    double denominator = sqrt(pow(b.y() - a.y(), 2) + pow(b.x() - a.x(), 2));
    if (numerator / denominator < threshold) {
        return true;
    }
    else return false;
}
QCPColorCurve::QCPColorCurve(QCPAxis* keyAxis, QCPAxis* valueAxis) : QCPCurve(keyAxis, valueAxis) {}

QCPColorCurve::~QCPColorCurve() { }
bool operator <(const QPointF& a, const  QPointF& b) {
    if(a.x() -0.0000005< b.x()|| a.x() +0.0000005 > b.x())
        return a.y() < b.y();
    return a.x() < b.x();
}
void QCPColorCurve::setData(const QVector<double>& keys, const QVector<double>& values) {
    QCPCurve::setData(keys, values);
}

void QCPColorCurve::setInliers(const std::multimap<QPointF, bool>& inliers)
{
    wasRANSAC = true;
    this->inliers = inliers;
}

void QCPColorCurve::drawScatterPlot(QCPPainter* painter, const QVector<QPointF>& points, const QCPScatterStyle& style) const {
    applyScattersAntialiasingHint(painter);
    int nPoints = points.size();
    painter->setPen(QPen(QBrush(Qt::gray), 4));
    if (!wasRANSAC) {
        for (int i = 0; i < nPoints; ++i)
            if (!qIsNaN(points.at(i).x()) && !qIsNaN(points.at(i).y())) {
                style.drawShape(painter, points.at(i));
            }
    }
    else for (int i = 0; i < nPoints; ++i)
        if (!qIsNaN(points.at(i).x()) && !qIsNaN(points.at(i).y())) {
            double x, y;
            pixelsToCoords(points.at(i), x, y);
            QPointF point(x, y);
            if (isWithinThreshold(infLine->point1->coords(), infLine->point2->coords(), point, *p_threshold))
                painter->setPen(Qt::blue);
            else    painter->setPen(Qt::red);
            style.drawShape(painter, points.at(i));
        }
}

RANSAC::RANSAC(QWidget* parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);
    threshold = 0;

    ui.graph->setInteraction(QCP::iRangeDrag, true);
    ui.graph->setInteraction(QCP::iRangeZoom, true);

    infLine = new QCPItemStraightLine(ui.graph);
    infLine->setVisible(false);
    infLine->setPen(QPen(QBrush(Qt::blue), 4));

    allPoints = new QCPColorCurve(ui.graph->xAxis, ui.graph->yAxis);
    allPoints->removeFromLegend();
    allPoints->setLineStyle(QCPCurve::LineStyle::lsNone);
    allPoints->setScatterStyle(QCPScatterStyle::ssCircle);
    allPoints->removeFromLegend();
    allPoints->setScatterStyle(QCPScatterStyle::ssDisc);
    allPoints->p_threshold = &threshold;
    allPoints->infLine = infLine;

    connect(ui.graph, SIGNAL(mousePress( QMouseEvent * )),
        this, SLOT(onMousePress( QMouseEvent * )));

    timer = new QTimer(this);
    ui.timer->setText("00:00:000");
    connect(timer, SIGNAL(timeout()), this, SLOT(timerUpdate()));

    udp = new MyUDP;
    ui.equation->setText("");

    allThread.resize(MAX_THREAD);
    allCalculation.resize(MAX_THREAD); 
    for (int i = 0; i < MAX_THREAD; ++i) {
        allThread[i] = new QThread(this);
        allCalculation[i] = new Calculation(this);
        allCalculation[i]->moveToThread(allThread[i]);
        connect(allCalculation[i], SIGNAL(workDone(unsigned int, double, double)), allThread[i], SLOT(quit()));
        connect(allCalculation[i], SIGNAL(workDone(unsigned int, double, double)),
            this, SLOT(handleResults(unsigned int, double, double)));
    }

}
void RANSAC::timerUpdate()
{
    //Timer's work
    timerTime++;
    QTime time = QTime::fromMSecsSinceStartOfDay(timerTime);
    ui.timer->setText(time.toString("mm:ss:zzz"));
}
RANSAC::~RANSAC()
{

    for (int i = 0; i < MAX_THREAD; ++i) {
        allThread[i]->quit();
        allThread[i]->wait();
    }

}
//Calculate threshold with MAD
  double calcThreshold(QCPCurveDataContainer* data) {
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
     return sqrtf(sortedData[sortedData.size() / 2]) / 12.;
}
 void RANSAC::loadFromFile()
 {
     if (working)
         return;
     QVector <double> keys, values;
     QString filesName = QFileDialog::getOpenFileName(
         this,
         tr("Open file"),
         "C://",
         "Text files (*.txt);; All files (*.*)"
     );
     if (filesName.isEmpty())
         return;
     QFile file(filesName, this);
     if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
         QMessageBox::information(this, tr("Unable to open file"),
             file.errorString());
         return;
     }

     //Clear 
     ui.timer->setText("00:00:000");
     ui.equation->setText("");
     infLine->setVisible(false);
     allPoints->setPen(QPen(QBrush(Qt::gray), 4));
     allPoints->data()->clear();
     allPoints->wasRANSAC = false;

     QDataStream in(&file);
     while (!file.atEnd()) {
         QString line(file.readLine());
         auto splitedLine = QStringRef(&line, 1, line.size() - 3).split(',');
         bool ok;
         double x = splitedLine[0].toDouble(&ok);
         double y = splitedLine[1].toDouble(&ok);
         if (!ok) {
             QMessageBox::information(this, tr("Data corrupted"),
                 file.errorString());
             return;
         }
         keys.push_back(x);
         values.push_back(y);
     }
     file.close();
     allPoints->setData(keys, values);

     //Set data range
     bool ok;
     auto rangeX = allPoints->data()->keyRange(ok);
     auto rangeY = allPoints->data()->valueRange(ok);
     ui.graph->xAxis->setRange(rangeX);
     ui.graph->yAxis->setRange(rangeX);
     ui.graph->replot();
     threshold = calcThreshold(allPoints->data().data());
 }
void RANSAC::saveToFile() {
    if (working)
        return;
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

     auto dataMap =allPoints->data().data();
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
            auto dataMap = allPoints->data().data();
             
            for (auto& point : *dataMap) {//Delete point
                if (abs(point.key - x) < threshold/5 && abs(point.value - y) < threshold / 5) {
                    dataMap->remove(point.t);
                    thereIs = true;
                }
            }
            if (!thereIs) //Add point
                allPoints->addData(x, y);
            ui.graph->replot();
            threshold = calcThreshold(allPoints->data().data());
        }
      
    }
}

void RANSAC::startRANSAC()
{
    if (allPoints->data().data()->isEmpty())
        return;
    if (working)
        return;
    working = true;
    for (int i = 0; i < (multithreading ? MAX_THREAD : 1); ++i) {
            
            connect(this, SIGNAL(operate(int, QCPCurveDataContainer const*, double)),
                allCalculation[i], SLOT(doWork(int, QCPCurveDataContainer const*, double)));
            allThread[i]->start();
    }
    //Number of iterations
    unsigned int k = 300;
    timer->start(1);  
    if (multithreading)
        emit operate(ceil((float)k / MAX_THREAD), allPoints->data().data(), threshold);
    else  
        emit operate(k, allPoints->data().data(), threshold);
}

void RANSAC::handleResults(unsigned int score, double k, double b)
{
    static unsigned int bestScore = 0, count=0;
    static double bestK = 0, bestB = 0;

    if (score > bestScore) {
        bestK = k;
        bestB = b;
    }
    //How threads end calculations
    count++;
    if (count >= (multithreading ? MAX_THREAD : 1)) {
        infLine->setVisible(true);
        infLine->point1->setCoords(0, bestB);  // location of best point 1  coordinate
        infLine->point2->setCoords(100, 100 * bestK + bestB);  // location of best point 2 coordinate

        ui.equation->setText("y=" + QString::number(bestK, 10, 3) + "x + " + QString::number(bestB, 10, 3));
        //Recolor points to blue
        auto data =allPoints->data().data();
        auto totalN = data->size();
        QPointF currPoint, maybeInlierA, maybeInlierB;
        maybeInlierA.setX(0); maybeInlierA.setY(b);
        maybeInlierB.setX(100); maybeInlierB.setY(100 * k + b);
        std:: multimap<QPointF, bool> inliers;
        for (unsigned p = 0; p < totalN; p++) {
           currPoint = { data->at(p)->key , data->at(p)->value };
           inliers.insert({ currPoint,isWithinThreshold(maybeInlierA, maybeInlierB, currPoint, threshold )});
        }
        for (int i = 0; i < MAX_THREAD; ++i)
            disconnect(this, SIGNAL(operate(int, QCPCurveDataContainer const*, double)),
                allCalculation[i], SLOT(doWork(int, QCPCurveDataContainer const*, double)));
        allPoints->setInliers(inliers);
        ui.graph->replot();
        udp->sendStraight(bestK, bestB);
        timer->stop();
        timerTime = 0;
        count = 0;
        bestScore = 0;
        bestK = 0;
        bestB = 0;
        working = false;
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

void Calculation::doWork(int k, QCPCurveDataContainer const * data,double threshold) {
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
