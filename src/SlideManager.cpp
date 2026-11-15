#include "SlideManager.h"
#include "ui_SlideManager.h"

SlideManager::SlideManager(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::SlideManager)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_StyledBackground, true);
    setAutoFillBackground(true);

    QPalette palette = this->palette();
    palette.setColor(QPalette::Window, Qt::red);
    setPalette(palette);
}

SlideManager::~SlideManager()
{
    delete ui;
}
