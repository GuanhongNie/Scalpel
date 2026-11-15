#include "TcpProtocol.h"
#include <QHostAddress>
#include <QThread>
#include <QCoreApplication>


// 初始化静态成员变量
TcpProtocol* TcpProtocol::m_instance = nullptr;
QMutex TcpProtocol::m_mutex;

TcpProtocol* TcpProtocol::instance()
{
    QMutexLocker locker(&m_mutex);
    if (m_instance == nullptr)
    {
        m_instance = new TcpProtocol();
    }
    return m_instance;
}

void TcpProtocol::destroy()
{
    QMutexLocker locker(&m_mutex);
    if (m_instance != nullptr)
    {
        delete m_instance;
        m_instance = nullptr;
    }
}

TcpProtocol::TcpProtocol(QObject *parent) : QObject(parent)
{
    m_socket = new QTcpSocket(this);

    connect(m_socket, &QTcpSocket::connected, this, &TcpProtocol::onConnected);
    connect(m_socket, &QTcpSocket::disconnected, this, &TcpProtocol::onDisconnected);
    connect(m_socket, &QTcpSocket::readyRead, this, &TcpProtocol::onReadyRead);
    connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::errorOccurred),
            this, &TcpProtocol::onErrorOccurred);
}

TcpProtocol::~TcpProtocol()
{
    if (m_socket->state() == QAbstractSocket::ConnectedState)
        m_socket->disconnectFromHost();
    
    delete m_socket;
}

void TcpProtocol::connectToServer(const QString &host, quint16 port)
{
    if (m_socket->state() == QAbstractSocket::ConnectingState)
        return;

    if (m_socket->state() == QAbstractSocket::ConnectedState)
        return;

    m_host = host;
    m_port = port;

    m_socket->connectToHost(host, port);
}

void TcpProtocol::disconnectFromServer()
{
    m_socket->disconnectFromHost();
}

bool TcpProtocol::isConnected() const
{
    return m_socket->state() == QAbstractSocket::ConnectedState;
}

void TcpProtocol::onConnected()
{
    emit connected(m_host, m_port);
}

void TcpProtocol::onDisconnected()
{
    emit disconnected(m_host, m_port);
}

void TcpProtocol::onReadyRead()
{
    QByteArray data = m_socket->readAll();
    m_receiveBuffer.append(data);
    qDebug() << "Received bytes: " << m_receiveBuffer.size();
}

void TcpProtocol::onErrorOccurred(QAbstractSocket::SocketError error)
{
    Q_UNUSED(error);
    QString errorMsg = m_socket->errorString();
    emit errorOccurred(errorMsg);
}


void TcpProtocol::sendFlag(char cmd)
{
    // 发送数据
    qint64 bytesSent = m_socket->write(&cmd, 1);
    if (bytesSent == -1 || !m_socket->waitForBytesWritten(5000))
    {
        qDebug() << "Send failed or timed out: " << bytesSent;
        return; 
    }
}

QPair<int, QString> TcpProtocol::getResponse(QByteArray baCommand)
{
    QPair<int, QString> pairResponse;
    pairResponse.first = -1; // Default Error Code
    pairResponse.second = QString("No Response"); // Default Response String

    QString strResponse("No Response");
    
    // 清空接收缓冲区    
    while (m_socket->bytesAvailable() > 0) 
    {
        m_socket->readAll();
    }

    m_receiveBuffer.clear();

    // 发送数据
    qint64 bytesSent = m_socket->write(baCommand.constData(), baCommand.size());
    if (bytesSent == -1 || !m_socket->waitForBytesWritten(5000)) 
    {
        qDebug() << "Send failed or timed out: " << bytesSent;
        return pairResponse; // 发送失败或超时
    }

    // 等待接收数据
    QEventLoop loop;
    QTimer timer; // Timeout timer
    timer.setSingleShot(true);
    QObject::connect(m_socket,&QTcpSocket::readyRead, &loop, &QEventLoop::quit);
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timer.start(20000); // 20 seconds timeout
    loop.exec();

    if (timer.isActive()) 
    {
        bool bSuccess = (m_receiveBuffer.size() > 1) && (m_receiveBuffer[0] == baCommand[0]) /*&& (m_receiveBuffer[1] == 0)*/;
        if(bSuccess)
        {
            const char16_t *data = reinterpret_cast<const char16_t*>(m_receiveBuffer.constData());
            int length = m_receiveBuffer.size() / 2;
            strResponse = QString::fromUtf16(data + 2, length - 2); //data starts from index 2

            pairResponse.first = m_receiveBuffer[2]; // Error Code
            pairResponse.second = strResponse;
            qDebug() << "Error Code: " << pairResponse.first;
            qDebug() << "Response : " << strResponse << " (Length: " << strResponse.length() << ")";
        }    
    } 
    else // Timeout
    {
        qDebug() << "Receive timed out.";
    }


    return pairResponse;
}
