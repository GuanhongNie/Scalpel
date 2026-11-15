
#ifndef TCPPROTOCOL_H
#define TCPPROTOCOL_H

#include <QObject>
#include <QTcpSocket>
#include <QTimer>
#include <QEventLoop>
#include <QElapsedTimer>
#include <QMutex>

//
class TcpProtocol : public QObject
{
    Q_OBJECT

public:
    static TcpProtocol* instance();
    static void destroy();

    QPair<int, QString> getResponse(QByteArray baCommand); // (return code, response string)

    void sendFlag(char cmd);
    void connectToServer(const QString &host, quint16 port);
    void disconnectFromServer();
    bool isConnected() const;

    TcpProtocol(const TcpProtocol&)            = delete;
    TcpProtocol& operator=(const TcpProtocol&) = delete;
    TcpProtocol(TcpProtocol&&)                 = delete;
    TcpProtocol& operator=(TcpProtocol&&)      = delete;

signals:
    void connected(const QString &host, quint16 port);
    void disconnected(const QString &host, quint16 port);
    void errorOccurred(const QString &errorMessage);

public:
    explicit TcpProtocol(QObject *parent = nullptr);
    ~TcpProtocol();

private:
    static TcpProtocol* m_instance;
    static QMutex m_mutex;

    QTcpSocket *m_socket;
    QByteArray m_receiveBuffer;
    QElapsedTimer m_timer;
    QString m_host;
    quint16 m_port;

private slots:
    void onConnected();
    void onDisconnected();
    void onReadyRead();
    void onErrorOccurred(QAbstractSocket::SocketError error);

};

#endif // TCPPROTOCOL_H