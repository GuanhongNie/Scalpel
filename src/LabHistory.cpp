#include "LabHistory.h"
#include "ui_LabHistory.h"

LabHistory::LabHistory(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::LabHistory)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_StyledBackground, true);
    setAutoFillBackground(true);

    QPalette palette = this->palette();
    palette.setColor(QPalette::Window, Qt::green);
    setPalette(palette);
}

LabHistory::~LabHistory()
{
    delete ui;
}
