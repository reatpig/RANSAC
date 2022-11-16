/********************************************************************************
** Form generated from reading UI file 'RANSACclient.ui'
**
** Created by: Qt User Interface Compiler version 5.12.12
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_RANSACCLIENT_H
#define UI_RANSACCLIENT_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QWidget>
#include "qcustomplot.h"

QT_BEGIN_NAMESPACE

class Ui_RANSACclientClass
{
public:
    QWidget *centralWidget;
    QCustomPlot *Graph;
    QLabel *text;
    QMenuBar *menuBar;
    QToolBar *mainToolBar;
    QStatusBar *statusBar;

    void setupUi(QMainWindow *RANSACclientClass)
    {
        if (RANSACclientClass->objectName().isEmpty())
            RANSACclientClass->setObjectName(QString::fromUtf8("RANSACclientClass"));
        RANSACclientClass->resize(780, 493);
        centralWidget = new QWidget(RANSACclientClass);
        centralWidget->setObjectName(QString::fromUtf8("centralWidget"));
        Graph = new QCustomPlot(centralWidget);
        Graph->setObjectName(QString::fromUtf8("Graph"));
        Graph->setGeometry(QRect(20, 10, 541, 421));
        text = new QLabel(centralWidget);
        text->setObjectName(QString::fromUtf8("text"));
        text->setGeometry(QRect(590, 20, 171, 21));
        RANSACclientClass->setCentralWidget(centralWidget);
        menuBar = new QMenuBar(RANSACclientClass);
        menuBar->setObjectName(QString::fromUtf8("menuBar"));
        menuBar->setGeometry(QRect(0, 0, 780, 26));
        RANSACclientClass->setMenuBar(menuBar);
        mainToolBar = new QToolBar(RANSACclientClass);
        mainToolBar->setObjectName(QString::fromUtf8("mainToolBar"));
        RANSACclientClass->addToolBar(Qt::TopToolBarArea, mainToolBar);
        statusBar = new QStatusBar(RANSACclientClass);
        statusBar->setObjectName(QString::fromUtf8("statusBar"));
        RANSACclientClass->setStatusBar(statusBar);

        retranslateUi(RANSACclientClass);

        QMetaObject::connectSlotsByName(RANSACclientClass);
    } // setupUi

    void retranslateUi(QMainWindow *RANSACclientClass)
    {
        RANSACclientClass->setWindowTitle(QApplication::translate("RANSACclientClass", "RANSACclient", nullptr));
        text->setText(QApplication::translate("RANSACclientClass", "TextLabel", nullptr));
    } // retranslateUi

};

namespace Ui {
    class RANSACclientClass: public Ui_RANSACclientClass {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_RANSACCLIENT_H
