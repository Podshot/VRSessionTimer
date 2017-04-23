#ifndef VRNOTIFICATION_H
#define VRNOTIFICATION_H

#include <QWidget>

namespace Ui {
class VRNotification;
}

class VRNotification : public QWidget
{
    Q_OBJECT

public:
    explicit VRNotification(QWidget *parent = 0);
    ~VRNotification();

private:
    Ui::VRNotification *ui;
};

#endif // VRNOTIFICATION_H
