#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    //OverlayWidget *pOverlayWidget = new OverlayWidget;
    //OverlayWidget *pOverlayWidget2 = new OverlayWidget;
    MainWindow *pMainWindow = new MainWindow;

    pMainWindow->show();

    //COpenVROverlayController::SharedInstance()->Init();

    //COpenVROverlayController::SharedInstance()->SetWidget( pOverlayWidget );

    //pOverlayWidget->show();

    // don't show widgets that you're going display in an overlay
    //w.show();

    return a.exec();
}
