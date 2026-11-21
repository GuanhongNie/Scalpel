#include "DeviceManager.h"
#include "DeviceAdmin.h"
#include "ui_DeviceManager.h"

#include <QMessageBox>
#include <QTableWidgetItem>

#include "DBManager.h"

DeviceManager::DeviceManager(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DeviceManager)
{
    ui->setupUi(this);
   // setWindowTitle("Device Manager");

    ui->tblWidgetDevices->setColumnCount(4);
    ui->tblWidgetDevices->setHorizontalHeaderLabels({"设备名称", "设备 IP", "设备端口", "启用设备"});
    ui->tblWidgetDevices->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tblWidgetDevices->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tblWidgetDevices->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tblWidgetDevices->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tblWidgetDevices->horizontalHeader()->setStyleSheet(
        "QHeaderView::section {"
        "background-color: #C8DFF0;"
        "color: #17324D;"
        "font-weight: bold;"
        "padding: 5px;"
        "border: 1px solid #A9C3D8;"
        "}");
    ui->tblWidgetDevices->setAlternatingRowColors(true);

    ui->pushButtonEdit->setEnabled(false);
    ui->pushButtonDelete->setEnabled(false);

    connect(ui->pushButtonNew, &QPushButton::clicked, this, &DeviceManager::onAddDevice);
    connect(ui->pushButtonEdit, &QPushButton::clicked, this, &DeviceManager::onEditDevice);
    connect(ui->pushButtonDelete, &QPushButton::clicked, this, &DeviceManager::onDeleteDevice);
    connect(ui->tblWidgetDevices, &QTableWidget::itemSelectionChanged, this, [this]() {
        const bool hasSelection = ui->tblWidgetDevices->currentRow() >= 0
            && ui->tblWidgetDevices->currentRow() < ui->tblWidgetDevices->rowCount();
        ui->pushButtonEdit->setEnabled(hasSelection);
        ui->pushButtonDelete->setEnabled(hasSelection);
    });
    connect(ui->tblWidgetDevices, &QTableWidget::itemDoubleClicked,
            this, &DeviceManager::onTableItemDoubleClicked);

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

DeviceManager::~DeviceManager()
{
    delete ui;
}

void DeviceManager::setDevices(const QVector<Device> &devices)
{
    m_vecDevice = devices;
    refreshTable();
}

void DeviceManager::refreshTable()
{
    ui->tblWidgetDevices->clearContents();
    ui->tblWidgetDevices->setRowCount(static_cast<int>(m_vecDevice.size()));
    ui->tblWidgetDevices->clearSelection();

    for (int row = 0; row < m_vecDevice.size(); ++row)
    {
        const Device &device = m_vecDevice.at(row);
        ui->tblWidgetDevices->setItem(row, 0, new QTableWidgetItem(device.strDeviceName));
        ui->tblWidgetDevices->setItem(row, 1, new QTableWidgetItem(device.strDeviceIP));
        ui->tblWidgetDevices->setItem(row, 2, new QTableWidgetItem(device.strDevicePort));
        ui->tblWidgetDevices->setItem(row, 3, new QTableWidgetItem(device.bEnabled ? "是" : "否"));
    }

    ui->pushButtonEdit->setEnabled(ui->tblWidgetDevices->currentRow() >= 0 && ui->tblWidgetDevices->currentRow() < ui->tblWidgetDevices->rowCount());
    ui->pushButtonDelete->setEnabled(ui->tblWidgetDevices->currentRow() >= 0 && ui->tblWidgetDevices->currentRow() < ui->tblWidgetDevices->rowCount());
}

void DeviceManager::onAddDevice()
{
    QStringList existingNames;
    for (const Device &device : m_vecDevice)
    {
        existingNames << device.strDeviceName;
    }

    DeviceAdmin dialog(this);
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

void DeviceManager::onEditDevice()
{
    const int currentRow = ui->tblWidgetDevices->currentRow();
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

    DeviceAdmin dialog(this);
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

void DeviceManager::onDeleteDevice()
{
    const int currentRow = ui->tblWidgetDevices->currentRow();
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

void DeviceManager::onTableItemDoubleClicked(QTableWidgetItem *item)
{
    Q_UNUSED(item);
    onEditDevice();
}
