#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "overlaywidget.h"
#include "openvroverlaycontroller.h"
#include "overlaycontroller.h"
#include <iostream>

#define USEVR 0
#define OVERlAYHANDLER 1

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    // Start UI
    ui->setupUi(this);
    QTextStream out(stdout);
    timer = new QTimer(this);
    countingDown = false;
    shouldAutoRestart = true;
    //connect(ui->confirmButton, &QPushButton::clicked, this, &MainWindow::test);
    connect(timer, &QTimer::timeout, this, &MainWindow::updateTimeRemaining);
    connect(ui->confirmButton, &QPushButton::clicked, this, &MainWindow::startCountdown);
    connect(ui->closeNotificationButton, &QPushButton::clicked, this, &MainWindow::onCloseNotification);
    connect(ui->manualPushButton, &QPushButton::clicked, this, &MainWindow::stopCountdown);

    connect(ui->timeLimitField, SIGNAL(valueChanged(int)), this, SLOT(onTimeFieldChange(int)));
    connect(ui->autoRestartCheckBox, SIGNAL(clicked(bool)), this, SLOT(onAutoRestartChange(bool)));
    // End UI

    // Start VR
//#if USEVR == 1
    OverlayWidget *pOverlayWidget = new OverlayWidget;

#if OVERlAYHANDLER == 0
    bool success = COpenVROverlayController::SharedInstance()->Init();
#else
    bool success = OverlayController::Instance()->Init();
#endif
    //std::cout << "Initialized VR: " + success << std::endl;
    std::string s = "Initialized VR: ";
    s.append(success ? "true" : "false");
    QString str = QString::fromStdString(s);
    out << str << endl;
    //std::printf("Initialized VR: %s \n", success ? "true" : "false");
#if OVERlAYHANDLER == 0
    COpenVROverlayController::SharedInstance()->SetWidget( pOverlayWidget );
#endif
//#endif
    // End VR
}

void MainWindow::startCountdown() {
    countingDown = true;
    QSpinBox *pEditor = ui->timeLimitField;
    QTime currentTime = QTime::currentTime();
    timeLimit = currentTime.addSecs(pEditor->value() * 60);
    timer->start(DELAY);
}

void MainWindow::stopCountdown() {
    countingDown = false;
#if OVERlAYHANDLER == 0
    COpenVROverlayController::SharedInstance()->ShowWidget();
#else
    OverlayController::Instance()->ShowWidget();
#endif
    timer->stop();
}

void MainWindow::onCloseNotification() {
#if OVERlAYHANDLER == 0
    COpenVROverlayController::SharedInstance()->HideWidget();
#else
    OverlayController::Instance()->HideWidget();
#endif
    if (shouldAutoRestart) {
        startCountdown();
    } else {
        stopCountdown();
    }
}

void MainWindow::updateTimeRemaining() {
    if (countingDown) {
        QTime currentTime = QTime::currentTime();
        QLabel *remainingLabel = ui->timeRemaining;
        int deltaTime = currentTime.secsTo(timeLimit);
        int remainingMinutes = deltaTime / 60;
        int remainingSeconds = deltaTime % 60;

        if (remainingMinutes == 0 && remainingSeconds == 0) {
            stopCountdown();
        }

        remainingLabel->setText(QString("%1:%2").arg(QString::number(remainingMinutes)).arg(remainingSeconds, 2, 10, QChar('0')));
    }
}

void MainWindow::onTimeFieldChange(int value) {
    if (!countingDown) {
        QLabel *remainingLabel = ui->timeRemaining;
        remainingLabel->setText(QString("%1:00").arg(QString::number(value)));
    }
}

void MainWindow::onAutoRestartChange(bool value) {
    shouldAutoRestart = value;
}

MainWindow::~MainWindow()
{
    delete ui;
}
