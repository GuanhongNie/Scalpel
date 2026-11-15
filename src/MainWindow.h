#pragma once

#include <QMainWindow>
#include <QMap>
#include <QStringList>
#include <QTimer>
#include <QPushButton>

class SlideManager;
class LabHistory;
class ProcedureManager;
class ReagentManager;
class DeviceDetail;
class TcpProtocol;
class QVBoxLayout;
class QResizeEvent;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

    bool isIpConnected(const QString &ip) const;

private:
    void resizeEvent(QResizeEvent *event) override;

    Ui::MainWindow *ui;

    typedef struct {
        QString strDeviceName;
        QString strDeviceIP;
        QString strDevicePort;
        bool bDeviceEnabled;
        QPushButton *btnDevice;
    } DeviceButtonInfo;

    QVector<DeviceButtonInfo> m_vecDeviceButtonInfo;
    
    QVBoxLayout *m_vblDeviceLayout = nullptr;

    void onDeviceButtonClicked(int index);
    void refreshDeviceButtons();
    void updateDeviceButtonStatus(const QString &ip, bool connected);

    SlideManager *m_pSliderManager = nullptr;
    LabHistory *m_pLabHistory = nullptr;
    ProcedureManager *m_pProcedureManager = nullptr;
    ReagentManager *m_pReagentManager = nullptr;
    DeviceDetail *m_pDeviceDetail = nullptr;
    
    ////////以下测试/////////////
    QMap<QString, TcpProtocol*> m_clients;
    QMap<QString, bool> m_connectionStatus;
    QTimer *m_deviceConnectionTimer = nullptr;

    QWidget *m_currentPopup = nullptr;

    void showPopup(QWidget *target);
    void createClientForIp(const QString &ip);
    void checkEnabledDeviceConnections();
    void onTcpConnected(const QString &host, quint16 port);
    void onTcpDisconnected(const QString &host, quint16 port);
};
