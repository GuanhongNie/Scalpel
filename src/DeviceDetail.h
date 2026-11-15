#pragma once

#include <QWidget>

namespace Ui {
class DeviceDetail;
}

class DeviceDetail : public QWidget
{
    Q_OBJECT

public:
    explicit DeviceDetail(QWidget *parent = nullptr);
    ~DeviceDetail() override;

    void setText(const QString &text);

private:
    Ui::DeviceDetail *ui;
};
