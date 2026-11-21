#pragma once

#include <QDialog>
#include "DeviceManager.h"

namespace Ui {
class DeviceAdmin;
}

class DeviceAdmin : public QDialog
{
    Q_OBJECT

public:
    explicit DeviceAdmin(QWidget *parent = nullptr);
    ~DeviceAdmin() override;

    void setDevice(const Device &device);
    Device device() const;
    void setExistingDeviceNames(const QStringList &deviceNames);
    void setCurrentDeviceName(const QString &deviceName);

private slots:
    void onOkClicked();

private:
    Ui::DeviceAdmin *ui;
    QStringList m_existingDeviceNames;
    QString m_currentDeviceName;
};
