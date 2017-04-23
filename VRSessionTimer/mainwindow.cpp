#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "overlaywidget.h"
#include "openvroverlaycontroller.h"

#define USEVR 0

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    // Start UI
    ui->setupUi(this);
    timer = new QTimer(this);
    out = new QTextStream(stdout);
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
#if USEVR == 1
    OverlayWidget *pOverlayWidget = new OverlayWidget;

    COpenVROverlayController::SharedInstance()->Init();
    COpenVROverlayController::SharedInstance()->SetWidget( pOverlayWidget );
#endif
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
#if USEVR == 1
    COpenVROverlayController::SharedInstance()->ShowWidget();
#endif
    timer->stop();
}

void MainWindow::onCloseNotification() {
#if USEVR == 1
    COpenVROverlayController::SharedInstance()->HideWidget();
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

        remainingLabel->setText(QString("%1:%2").arg(QString::number(remainingMinutes), QString::number(remainingSeconds)));
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
