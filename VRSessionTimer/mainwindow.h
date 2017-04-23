#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QTextStream>
#include <QTime>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    void test();
    void updateTimeRemaining();
    void startCountdown();
    void stopCountdown();
    void onCloseNotification();

public slots:
    void onTimeFieldChange(int);
    void onAutoRestartChange(bool);

private:
    static const int DELAY = 500;

    Ui::MainWindow *ui;
    QTimer *timer;
    QTime timeLimit;
    QTextStream *out;
    bool countingDown;
    bool shouldAutoRestart;
};

#endif // MAINWINDOW_H
