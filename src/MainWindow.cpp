#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "DBManager.h"
#include "DeviceManagerMain.h"
#include "SlideManager.h"
#include "LabHistory.h"
#include "ProcedureManager.h"
#include "ReagentManager.h"
#include "DeviceDetail.h"
#include "TcpProtocol.h"

#include <QHBoxLayout>
#include <QMessageBox>
#include <QResizeEvent>
#include <QSizePolicy>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->statusbar->hide();

    // 使用centralwidget布局管理器来管理顶部按钮组和左侧设备列表滚动区域，使其布满整个窗口
    auto *centralLayout = new QVBoxLayout(ui->centralwidget);
    centralLayout->setContentsMargins(0, 0, 0, 0);
    centralLayout->setSpacing(0);

    // 顶部按钮带区域的设置
    ui->grbTop->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    ui->grbTop->setFixedHeight(60);
    centralLayout->addWidget(ui->grbTop, 0, Qt::AlignTop);

    // 左边设备列表滚动区域设置
    ui->scaDevice->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    ui->scaDevice->setFixedWidth(100);
    centralLayout->addWidget(ui->scaDevice, 1);
    
   // ui->scaDevice->setWidgetResizable(true);
    ui->scaDevice->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    ui->scaDevice->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    QWidget *deviceWidget = new QWidget(ui->scaDevice);
    ui->scaDevice->setWidget(deviceWidget);

    m_vblDeviceLayout = new QVBoxLayout(deviceWidget);
    m_vblDeviceLayout->setContentsMargins(2, 2, 2, 2);
    m_vblDeviceLayout->setSpacing(2);

    // 设备管理按钮
    connect(ui->btnDeviceManager, &QPushButton::clicked, this, [this]() {
        DeviceManagerMain dialog(this);
        dialog.exec();
        refreshDeviceButtons();
    });

    refreshDeviceButtons();

    m_deviceConnectionTimer = new QTimer(this);
    m_deviceConnectionTimer->setInterval(2000);
    connect(m_deviceConnectionTimer, &QTimer::timeout,
            this, &MainWindow::checkEnabledDeviceConnections);
    m_deviceConnectionTimer->start();

    m_pSliderManager = new SlideManager(ui->centralwidget);
    m_pSliderManager->hide();
    m_pSliderManager->setAttribute(Qt::WA_StyledBackground, true);
    m_pSliderManager->setAutoFillBackground(true);
    m_pSliderManager->resize(200, 120);

    QPalette subPalette = m_pSliderManager->palette();
    subPalette.setColor(QPalette::Window, Qt::red);
    m_pSliderManager->setPalette(subPalette);

    connect(ui->btnSlideManager, &QPushButton::clicked, this, [this]() {
        showPopup(m_pSliderManager);
    });

    m_pLabHistory = new LabHistory(ui->centralwidget);
    m_pLabHistory->hide();
    m_pLabHistory->setAttribute(Qt::WA_StyledBackground, true);
    m_pLabHistory->setAutoFillBackground(true);
    m_pLabHistory->resize(200, 120);

    QPalette labHistoryPalette = m_pLabHistory->palette();
    labHistoryPalette.setColor(QPalette::Window, Qt::green);
    m_pLabHistory->setPalette(labHistoryPalette);

    connect(ui->btnLabHistory, &QPushButton::clicked, this, [this]() {
        showPopup(m_pLabHistory);
    });

    m_pProcedureManager = new ProcedureManager(ui->centralwidget);
    m_pProcedureManager->hide();
    m_pProcedureManager->setAttribute(Qt::WA_StyledBackground, true);
    m_pProcedureManager->setAutoFillBackground(true);
    m_pProcedureManager->resize(200, 120);

    QPalette procedureManagerPalette = m_pProcedureManager->palette();
    procedureManagerPalette.setColor(QPalette::Window, Qt::blue);
    m_pProcedureManager->setPalette(procedureManagerPalette);

    connect(ui->btnProcedureManager, &QPushButton::clicked, this, [this]() {
        showPopup(m_pProcedureManager);
    });

    m_pReagentManager = new ReagentManager(ui->centralwidget);
    m_pReagentManager->hide();
    m_pReagentManager->setAttribute(Qt::WA_StyledBackground, true);
    m_pReagentManager->setAutoFillBackground(true);
    m_pReagentManager->resize(200, 120);

    QPalette reagentManagerPalette = m_pReagentManager->palette();
    reagentManagerPalette.setColor(QPalette::Window, Qt::yellow);
    m_pReagentManager->setPalette(reagentManagerPalette);

    connect(ui->btnReagentManager, &QPushButton::clicked, this, [this]() {
        showPopup(m_pReagentManager);
    });

    m_pDeviceDetail = new DeviceDetail(ui->centralwidget);
    m_pDeviceDetail->setAttribute(Qt::WA_StyledBackground, true);
    m_pDeviceDetail->setAutoFillBackground(true);
    m_pDeviceDetail->resize(200, 120);

    QPalette deviceDetailPalette = m_pDeviceDetail->palette();
    deviceDetailPalette.setColor(QPalette::Window, Qt::cyan);
    m_pDeviceDetail->setPalette(deviceDetailPalette);

    // 最大化窗口，保留任务栏
    this->showMaximized();
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);

    const QRect centralRect = ui->centralwidget->rect();
    const int x = ui->scaDevice->geometry().right() + 1;
    const int y = ui->grbTop->geometry().bottom() + 1;
    const QRect popupGeometry(
        x,
        y,
        qMax(0, centralRect.right() - x + 1),
        qMax(0, centralRect.bottom() - y + 1));

    if (m_pSliderManager)
        m_pSliderManager->setGeometry(popupGeometry);
    if (m_pLabHistory)
        m_pLabHistory->setGeometry(popupGeometry);
    if (m_pProcedureManager)
        m_pProcedureManager->setGeometry(popupGeometry);
    if (m_pReagentManager)
        m_pReagentManager->setGeometry(popupGeometry);
    if (m_pDeviceDetail)
        m_pDeviceDetail->setGeometry(popupGeometry);
}

void MainWindow::showPopup(QWidget *target)
{
    if (!target)
        return;

    if (m_currentPopup == target && target->isVisible())
    {
        return;
    }

    if (m_currentPopup && m_currentPopup != target)
    {
        m_currentPopup->hide();
    }

    target->raise();
    target->show();
    target->activateWindow();

    m_currentPopup = target;
}

MainWindow::~MainWindow()
{
    delete ui;
}

bool MainWindow::isIpConnected(const QString &ip) const
{
    return m_connectionStatus.value(ip, false);
}

void MainWindow::checkEnabledDeviceConnections()
{
    for (const DeviceButtonInfo &device : m_vecDeviceButtonInfo)
    {
        if (!device.bDeviceEnabled)
            continue;

        TcpProtocol *client = m_clients.value(device.strDeviceIP, nullptr);
        if (client && !client->isConnected())
        {
            qDebug() << "Device disconnected, reconnecting:" << device.strDeviceIP;
            client->connectToServer(device.strDeviceIP, device.strDevicePort.toUShort());
        }
    }
}

void MainWindow::createClientForIp(const QString &ip)
{
    TcpProtocol *client = new TcpProtocol(this);
    m_clients.insert(ip, client);
    m_connectionStatus.insert(ip, false);

    connect(client, &TcpProtocol::connected,
            this, [this, ip](const QString &host, quint16 port) {
                Q_UNUSED(port);
                onTcpConnected(host, port);
            });

    connect(client, &TcpProtocol::disconnected,
            this, [this, ip](const QString &host, quint16 port) {
                Q_UNUSED(port);
                onTcpDisconnected(host, port);
            });
}

void MainWindow::onTcpConnected(const QString &host, quint16 port)
{
    if (!m_connectionStatus.contains(host))
    {
        qDebug() << "Unknown connected host:" << host << ":" << port;
        return;
    }

    m_connectionStatus[host] = true;
    updateDeviceButtonStatus(host, true);
    qDebug() << "Connected:" << host << ":" << port;
}

void MainWindow::onTcpDisconnected(const QString &host, quint16 port)
{
    if (!m_connectionStatus.contains(host))
    {
        qDebug() << "Unknown disconnected host:" << host << ":" << port;
        return;
    }

    m_connectionStatus[host] = false;
    updateDeviceButtonStatus(host, false);
    qDebug() << "Disconnected:" << host << ":" << port;
}


// 更新设备按钮的状态，根据连接状态和启用状态设置按钮的背景颜色（临时测试用）
void MainWindow::updateDeviceButtonStatus(const QString &ip, bool connected)
{
    for (const auto &dbi : m_vecDeviceButtonInfo)
    {
        if (dbi.strDeviceIP == ip)
        {
            if (!dbi.bDeviceEnabled)
                dbi.btnDevice->setStyleSheet("QPushButton { background-color: red; }");
            else
                dbi.btnDevice->setStyleSheet(
                    connected ? "QPushButton { background-color: green; }" : QString());
            break;
        }
    }
}

void MainWindow::onDeviceButtonClicked(int index)
{
    qDebug() << "Device button clicked:" << index;
    m_pDeviceDetail->setText(m_vecDeviceButtonInfo[index - 1].strDeviceName);
    showPopup(m_pDeviceDetail);
}

void MainWindow::refreshDeviceButtons()
{
    QVector<DBManager::DeviceInfo> devices;
    if (!DBManager::instance().getDeviceList(devices))
    {
        return;
    }

    // 先清除旧按钮，再创建新按钮。
    while (QLayoutItem *item = m_vblDeviceLayout->takeAt(0))
    {
        delete item->widget();
        delete item;
    }

    m_vecDeviceButtonInfo.clear();

    const int buttonCount = devices.size();
    const int buttonSide = qMax(40, ui->scaDevice->viewport()->width() - 8);
    for (int i = 0; i < buttonCount; ++i)
    {
        const DBManager::DeviceInfo &device = devices.at(i);
        auto *btnDevice = new QPushButton(ui->scaDevice->widget());
        btnDevice->setFixedHeight(buttonSide);
        btnDevice->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        btnDevice->setText(device.strDeviceName);
        connect(btnDevice, &QPushButton::clicked, this, [this, i]() {
            onDeviceButtonClicked(i + 1);
        });

        m_vblDeviceLayout->addWidget(btnDevice);
        DeviceButtonInfo dbi;
        dbi.strDeviceName = device.strDeviceName;
        dbi.strDeviceIP = device.strDeviceIP;
        dbi.strDevicePort = device.strDevicePort;
        dbi.bDeviceEnabled = device.bDeviceEnabled;
        dbi.btnDevice = btnDevice;
        m_vecDeviceButtonInfo.append(dbi);
        updateDeviceButtonStatus(device.strDeviceIP, isIpConnected(device.strDeviceIP));
        if (device.bDeviceEnabled && !m_clients.contains(device.strDeviceIP))
            createClientForIp(device.strDeviceIP);
        else if (!device.bDeviceEnabled)
        {
            TcpProtocol *client = m_clients.value(device.strDeviceIP, nullptr);
            if (client)
                client->disconnectFromServer();
        }
    }

    const int totalHeight = buttonCount * buttonSide
        + qMax(0, buttonCount - 1) * m_vblDeviceLayout->spacing() + 10;
    ui->scaDevice->widget()->setMinimumHeight(totalHeight);
}