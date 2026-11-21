#pragma once

#include <QDialog>
#include <QTableWidgetItem>
#include <QVector>

struct Device {
    QString strDeviceName;
    QString strDeviceIP;
    QString strDevicePort;
    bool bEnabled = true;
};

namespace Ui {
class DeviceManager;
}

class DeviceManager : public QDialog
{
    Q_OBJECT

public:
    explicit DeviceManager(QWidget *parent = nullptr);
    ~DeviceManager() override;

    void setDevices(const QVector<Device> &devices);

private slots:
    void onAddDevice();
    void onEditDevice();
    void onDeleteDevice();
    void onTableItemDoubleClicked(QTableWidgetItem *item);

private:
    Ui::DeviceManager *ui;
    QVector<Device> m_vecDevice;

    void refreshTable();
};
