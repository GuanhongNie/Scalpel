#ifndef DBMANAGER_H
#define DBMANAGER_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QString>
#include <QVariant>
#include <QCryptographicHash>
#include <QVector>

class DBManager : public QObject
{
    Q_OBJECT
public:
    struct DeviceInfo
    {
        QString strDeviceName;
        QString strDeviceIP;
        QString strDevicePort;
        bool bDeviceEnabled;
    };

    // 获取单例
    static DBManager &instance();

    void closeDatabase();

    bool verifyUser(const QString &strUserName, const QString &strPlainPassword);

    bool addDevice(const QString &strDeviceName, const QString &strDeviceIP, const QString &strDevicePort, bool enabled);
    bool deleteDevice(const QString &strDeviceName);
    bool editDevice(const QString &strOldDeviceName, const QString &strNewDeviceName,
                    const QString &strNewDeviceIP, const QString &strNewDevicePort, bool enabled);
    bool getDeviceList(QVector<DeviceInfo> &devices);

private:
    explicit DBManager(QObject *parent = nullptr);
    ~DBManager();
    DBManager(const DBManager &) = delete;
    DBManager &operator=(const DBManager &) = delete;

    bool createTables();
    bool addUser(const QString &strUserName, const QString &strPassword);
    bool deleteUser(const QString &strUserName);
    bool editUser(const QString &strUserName, const QString &strNewPassword);
    bool getUserList();
    struct UserInfo
    {
        QString username;
        QString passwordHash;
    };
    
    QVector<UserInfo>  m_vecUserInfo; // 用户名，密码哈希

    QSqlDatabase m_database;

    // 把明文转为 SHA256 十六进制哈希
    QString hashPassword(const QString &plainPassword);    

    const QString m_strDBName;
};

#endif // DBMANAGER_H
