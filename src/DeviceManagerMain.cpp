#include "DeviceManagerMain.h"
#include "DeviceManagerSub.h"
#include "ui_DeviceManagerMain.h"

#include <QMessageBox>
#include <QTableWidgetItem>

#include "DBManager.h"

DeviceManagerMain::DeviceManagerMain(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DeviceManagerMain)
{
    ui->setupUi(this);
   // setWindowTitle("Device Manager");

    ui->tableWidgetDevices->setColumnCount(4);
    ui->tableWidgetDevices->setHorizontalHeaderLabels({"设备名称", "设备 IP", "设备端口", "启用设备"});
    ui->tableWidgetDevices->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableWidgetDevices->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidgetDevices->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tableWidgetDevices->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableWidgetDevices->horizontalHeader()->setStyleSheet(
        "QHeaderView::section {"
        "background-color: #C8DFF0;"
        "color: #17324D;"
        "font-weight: bold;"
        "padding: 5px;"
        "border: 1px solid #A9C3D8;"
        "}");
    ui->tableWidgetDevices->setAlternatingRowColors(true);

    ui->pushButtonEdit->setEnabled(false);
    ui->pushButtonDelete->setEnabled(false);

    connect(ui->pushButtonNew, &QPushButton::clicked, this, &DeviceManagerMain::onAddDevice);
    connect(ui->pushButtonEdit, &QPushButton::clicked, this, &DeviceManagerMain::onEditDevice);
    connect(ui->pushButtonDelete, &QPushButton::clicked, this, &DeviceManagerMain::onDeleteDevice);
    connect(ui->tableWidgetDevices, &QTableWidget::itemSelectionChanged, this, [this]() {
        const bool hasSelection = ui->tableWidgetDevices->currentRow() >= 0
            && ui->tableWidgetDevices->currentRow() < ui->tableWidgetDevices->rowCount();
        ui->pushButtonEdit->setEnabled(hasSelection);
        ui->pushButtonDelete->setEnabled(hasSelection);
    });
    connect(ui->tableWidgetDevices, &QTableWidget::itemDoubleClicked,
            this, &DeviceManagerMain::onTableItemDoubleClicked);

    m_vecDevice.clear();

    QVector<DBManager::DeviceInfo> devices;
    if (DBManager::instance().getDeviceList(devices))
    {
        m_vecDevice.reserve(devices.size());
        for (const DBManager::DeviceInfo &record : devices)
        {
            m_vecDevice.append({
                record.strDeviceName,
                record.strDeviceIP,
                record.strDevicePort,
                record.bDeviceEnabled
            });
        }
    }

    refreshTable();
}

DeviceManagerMain::~DeviceManagerMain()
{
    delete ui;
}

void DeviceManagerMain::setDevices(const QVector<Device> &devices)
{
    m_vecDevice = devices;
    refreshTable();
}

void DeviceManagerMain::refreshTable()
{
    ui->tableWidgetDevices->clearContents();
    ui->tableWidgetDevices->setRowCount(static_cast<int>(m_vecDevice.size()));
    ui->tableWidgetDevices->clearSelection();

    for (int row = 0; row < m_vecDevice.size(); ++row)
    {
        const Device &device = m_vecDevice.at(row);
        ui->tableWidgetDevices->setItem(row, 0, new QTableWidgetItem(device.strDeviceName));
        ui->tableWidgetDevices->setItem(row, 1, new QTableWidgetItem(device.strDeviceIP));
        ui->tableWidgetDevices->setItem(row, 2, new QTableWidgetItem(device.strDevicePort));
        ui->tableWidgetDevices->setItem(row, 3, new QTableWidgetItem(device.bEnabled ? "是" : "否"));
    }

    ui->pushButtonEdit->setEnabled(ui->tableWidgetDevices->currentRow() >= 0 && ui->tableWidgetDevices->currentRow() < ui->tableWidgetDevices->rowCount());
    ui->pushButtonDelete->setEnabled(ui->tableWidgetDevices->currentRow() >= 0 && ui->tableWidgetDevices->currentRow() < ui->tableWidgetDevices->rowCount());
}

void DeviceManagerMain::onAddDevice()
{
    QStringList existingNames;
    for (const Device &device : m_vecDevice)
    {
        existingNames << device.strDeviceName;
    }

    DeviceManagerSub dialog(this);
    dialog.setWindowTitle("新建设备");
    dialog.setExistingDeviceNames(existingNames);

    if (dialog.exec() == QDialog::Accepted)
    {
        const Device device = dialog.device();
        if (!DBManager::instance().addDevice(device.strDeviceName, device.strDeviceIP, device.strDevicePort, device.bEnabled))
        {
            QMessageBox::critical(this, "新建设备", "保存设备到数据库失败。设备名称可能已存在。");
            return;
        }

        m_vecDevice.append(device);
        refreshTable();
    }
}

void DeviceManagerMain::onEditDevice()
{
    const int currentRow = ui->tableWidgetDevices->currentRow();
    if (currentRow < 0 || currentRow >= m_vecDevice.size())
    {
        QMessageBox::warning(this, "Edit Device", "Please select a device to edit.");
        return;
    }

    QStringList existingNames;
    for (int i = 0; i < m_vecDevice.size(); ++i)
    {
        if (i != currentRow)
        {
            existingNames << m_vecDevice.at(i).strDeviceName;
        }
    }

    DeviceManagerSub dialog(this);
    dialog.setWindowTitle("编辑设备");
    dialog.setDevice(m_vecDevice.at(currentRow));
    dialog.setExistingDeviceNames(existingNames);
    dialog.setCurrentDeviceName(m_vecDevice.at(currentRow).strDeviceName);

    if (dialog.exec() == QDialog::Accepted)
    {
        const Device updatedDevice = dialog.device();
        const Device &oldDevice = m_vecDevice.at(currentRow);
        if (!DBManager::instance().editDevice(oldDevice.strDeviceName,
                                              updatedDevice.strDeviceName,
                                              updatedDevice.strDeviceIP,
                                              updatedDevice.strDevicePort,
                                              updatedDevice.bEnabled))
        {
            QMessageBox::critical(this, "编辑设备", "更新设备数据库记录失败。");
            return;
        }

        m_vecDevice[currentRow] = updatedDevice;
        refreshTable();
    }
}

void DeviceManagerMain::onDeleteDevice()
{
    const int currentRow = ui->tableWidgetDevices->currentRow();
    if (currentRow < 0 || currentRow >= m_vecDevice.size())
    {
        QMessageBox::warning(this, "Delete Device", "Please select a device to delete.");
        return;
    }

    const QString deviceName = m_vecDevice.at(currentRow).strDeviceName;
    const int result = QMessageBox::question(this,
                                            "Delete Device",
                                            QString("Are you sure you want to delete %1?").arg(deviceName));
    if (result == QMessageBox::Yes)
    {
        if (!DBManager::instance().deleteDevice(deviceName))
        {
            QMessageBox::critical(this, "删除设备", "从数据库删除设备失败。");
            return;
        }

        m_vecDevice.removeAt(currentRow);
        refreshTable();
    }
}

void DeviceManagerMain::onTableItemDoubleClicked(QTableWidgetItem *item)
{
    Q_UNUSED(item);
    onEditDevice();
}
