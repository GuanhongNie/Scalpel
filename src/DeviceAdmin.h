#pragma once

#include <QDialog>
#include "DeviceManagerMain.h"

namespace Ui {
class DeviceManagerSub;
}

class DeviceManagerSub : public QDialog
{
    Q_OBJECT

public:
    explicit DeviceManagerSub(QWidget *parent = nullptr);
    ~DeviceManagerSub() override;

    void setDevice(const Device &device);
    Device device() const;
    void setExistingDeviceNames(const QStringList &deviceNames);
    void setCurrentDeviceName(const QString &deviceName);

private slots:
    void onOkClicked();

private:
    Ui::DeviceManagerSub *ui;
    QStringList m_existingDeviceNames;
    QString m_currentDeviceName;
};
