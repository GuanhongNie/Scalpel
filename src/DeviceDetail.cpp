#include "DeviceDetail.h"
#include "ui_DeviceDetail.h"

DeviceDetail::DeviceDetail(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::DeviceDetail)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_StyledBackground, true);
    setAutoFillBackground(true);

    QPalette palette = this->palette();
    palette.setColor(QPalette::Window, Qt::red);
    setPalette(palette);
}

DeviceDetail::~DeviceDetail()
{
    delete ui;
}

void DeviceDetail::setText(const QString &text)
{
    ui->label->setText(text);
}
