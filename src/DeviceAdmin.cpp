#include "DeviceAdmin.h"
#include "DeviceManager.h"
#include "ui_DeviceAdmin.h"

#include <QMessageBox>
#include <QRegularExpression>

namespace {

bool isValidIpAddress(const QString &ip)
{
    const QRegularExpression regex(
        "^(?:(?:25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)\\.){3}(?:25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)$");
    return regex.match(ip).hasMatch();
}

bool isValidPort(const QString &portText)
{
    bool ok = false;
    const int port = portText.toInt(&ok);
    return ok && port >= 0 && port <= 65535;
}

} // namespace

DeviceAdmin::DeviceAdmin(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DeviceAdmin)
{
    ui->setupUi(this);
    setWindowTitle("Device Detail");

    connect(ui->pushButtonOK, &QPushButton::clicked, this, &DeviceAdmin::onOkClicked);
    connect(ui->pushButtonCancel, &QPushButton::clicked, this, &QDialog::reject);
}

DeviceAdmin::~DeviceAdmin()
{
    delete ui;
}

void DeviceAdmin::setDevice(const Device &device)
{
    ui->lineEditDeviceName->setText(device.strDeviceName);
    ui->lineEditDeviceIP->setText(device.strDeviceIP);
    ui->lineEditDevicePort->setText(device.strDevicePort);
    ui->checkBoxDeviceEnabled->setChecked(device.bEnabled);
}

Device DeviceAdmin::device() const
{
    Device device;
    device.strDeviceName = ui->lineEditDeviceName->text();
    device.strDeviceIP = ui->lineEditDeviceIP->text();
    device.strDevicePort = ui->lineEditDevicePort->text();
    device.bEnabled = ui->checkBoxDeviceEnabled->isChecked();
    return device;
}

void DeviceAdmin::setExistingDeviceNames(const QStringList &deviceNames)
{
    m_existingDeviceNames = deviceNames;
}

void DeviceAdmin::setCurrentDeviceName(const QString &deviceName)
{
    m_currentDeviceName = deviceName;
}

void DeviceAdmin::onOkClicked()
{
    const QString name = ui->lineEditDeviceName->text().trimmed();
    const QString ip = ui->lineEditDeviceIP->text().trimmed();
    const QString port = ui->lineEditDevicePort->text().trimmed();
    if (name.isEmpty() || ip.isEmpty() || port.isEmpty())
    {
        QMessageBox::warning(this, "Invalid input", "All three fields cannot be empty.");
        return;
    }

    if (!isValidIpAddress(ip))
    {
        QMessageBox::warning(this, "Invalid IP", "Please enter a valid IPv4 address.");
        return;
    }

    if (!isValidPort(port))
    {
        QMessageBox::warning(this, "Invalid Port", "Port must be a number between 0 and 65535.");
        return;
    }

    const bool isDuplicate = m_existingDeviceNames.contains(name, Qt::CaseInsensitive)
        && name.compare(m_currentDeviceName, Qt::CaseInsensitive) != 0;
    if (isDuplicate)
    {
        QMessageBox::warning(this, "Duplicate device", "A device with the same name already exists.");
        return;
    }

    accept();
}
