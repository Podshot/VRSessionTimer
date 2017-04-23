#include "vrnotification.h"
#include "ui_vrnotification.h"

VRNotification::VRNotification(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::VRNotification)
{
    ui->setupUi(this);
}

VRNotification::~VRNotification()
{
    delete ui;
}
