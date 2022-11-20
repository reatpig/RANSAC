#include "RANSAC.h"
#include <QtWidgets/QApplication>

int main(int argc, char* argv[])
{
    QApplication a(argc, argv);
    RANSAC w;
    qsrand(static_cast<uint>(QTime::currentTime().msec()));
    w.show();
    return a.exec();
}
