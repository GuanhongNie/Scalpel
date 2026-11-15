#include "dbmanager.h"
#include <QDateTime>
#include <QSqlRecord>
#include <QDir>
#include "DebugMessage.h"

DBManager &DBManager::instance()
{
    static DBManager obj;
    return obj;
}

DBManager::DBManager(QObject *parent)
    : QObject(parent)
    , m_strDBName(QString("CoreDataBase.db"))
{
    if (!createTables())
    {
        DEBUG_MESSAGE("DBManager initialization failed while creating database tables");
        return;
    }

    if (!addUser(QString("1"), QString("1")))
    {
        DEBUG_MESSAGE("DBManager initialization failed while adding the default user");
        return;
    }

    if (!getUserList())
    {
        DEBUG_MESSAGE("DBManager initialization failed while loading users");
    }
}

DBManager::~DBManager()
{
    closeDatabase();
}

bool DBManager::createTables()
{
    if (!m_database.isValid())
    {
        m_database = QSqlDatabase::addDatabase("QSQLITE", m_strDBName );

        //在exe文件当前目录创建数据库
        QString strDBPath = QDir::currentPath() + QString("/") + m_strDBName;
        m_database.setDatabaseName(strDBPath);
    }

    // 连接可能有效但尚未打开；打开失败时后续调用仍可重试。
    if (!m_database.isOpen() && !m_database.open())
    {
        DEBUG_MESSAGE("FAILED to open core database: " << m_database.lastError().text());
        return false;
    }

    QSqlQuery createTablesQuery(m_database);
    if (!createTablesQuery.exec(
            "CREATE TABLE IF NOT EXISTS USERS"
            "(USER_NAME TEXT NOT NULL, USER_PASSWORD TEXT NOT NULL, "
            "PRIMARY KEY(USER_NAME))"))
    {
        DEBUG_MESSAGE("FAILED to create USERS table: " << createTablesQuery.lastError().text());
        return false;
    }

    if (!createTablesQuery.exec(
            "CREATE TABLE IF NOT EXISTS DEVICES"
            "(DEVICE_NAME TEXT NOT NULL, DEVICE_IP TEXT NOT NULL, "
            "DEVICE_PORT TEXT NOT NULL, DEVICE_ENABLED BOOLEAN DEFAULT TRUE, "
            "PRIMARY KEY(DEVICE_NAME))"))
    {
        DEBUG_MESSAGE("FAILED to create DEVICES table: " << createTablesQuery.lastError().text());
        return false;
    }

    return true;
}


bool DBManager::addUser(const QString &strUserName, const QString &strPassword)
{
    if (strUserName.isEmpty() || strPassword.isEmpty())
    {
        DEBUG_MESSAGE("FAILED to add user: username and password must not be empty");
        return false;
    }

    QSqlQuery addUserQuery(m_database);
    // 使用占位符绑定用户数据，避免将输入拼接进 SQL。
    addUserQuery.prepare(
        "INSERT OR IGNORE INTO USERS(USER_NAME, USER_PASSWORD) "
        "VALUES (:username, :password)");
    addUserQuery.bindValue(":username", strUserName);
    addUserQuery.bindValue(":password", hashPassword(strPassword));
    if (!addUserQuery.exec())
    {
        DEBUG_MESSAGE("FAILED to add user: " << strUserName << " - " << addUserQuery.lastError().text());
        return false;
    }

    return true;
}

bool DBManager::deleteUser(const QString &strUserName)
{
    if (strUserName.isEmpty())
    {
        DEBUG_MESSAGE("FAILED to delete user: username must not be empty");
        return false;
    }

    QSqlQuery deleteUserQuery(m_database);
    // 使用占位符绑定用户名，避免将输入拼接进 SQL。
    deleteUserQuery.prepare("DELETE FROM USERS WHERE USER_NAME = :username");
    deleteUserQuery.bindValue(":username", strUserName);
    if (!deleteUserQuery.exec())
    {
        DEBUG_MESSAGE("FAILED to delete user: " << strUserName << " - " << deleteUserQuery.lastError().text());
        return false;
    }
    return true;
}

bool DBManager::editUser(const QString &strUserName, const QString &strNewPassword)
{
    if (strUserName.isEmpty() || strNewPassword.isEmpty())
    {
        DEBUG_MESSAGE("FAILED to edit user: username and password must not be empty");
        return false;
    }

    QSqlQuery editUserQuery(m_database);
    // 使用占位符绑定用户名和密码，避免将输入拼接进 SQL。
    editUserQuery.prepare(
        "UPDATE USERS SET USER_PASSWORD = :password WHERE USER_NAME = :username");
    editUserQuery.bindValue(":password", hashPassword(strNewPassword));
    editUserQuery.bindValue(":username", strUserName);
    if (!editUserQuery.exec())
    {
        DEBUG_MESSAGE("FAILED to edit user: " << strUserName << " - " << editUserQuery.lastError().text());
        return false;
    }
    return true;
}

bool DBManager::getUserList()
{
    QSqlQuery queryUserList(m_database);
    bool bRretval = queryUserList.exec("SELECT * FROM USERS");
    if( !bRretval )
    {
        DEBUG_MESSAGE("FAILED to query user list: " << queryUserList.lastError().text());
        return bRretval;
    }

    m_vecUserInfo.clear();
    while (queryUserList.next())
    {
        QString strUserName = queryUserList.value("USER_NAME").toString();
        QString strPasswordHash = queryUserList.value("USER_PASSWORD").toString();
        struct UserInfo userInfo;
        userInfo.username = strUserName;
        userInfo.passwordHash = strPasswordHash;
        
        m_vecUserInfo.append(userInfo);
        DEBUG_MESSAGE("User: " << strUserName << ", Password Hash: " << strPasswordHash);
    }

    return true;
}

bool DBManager::addDevice(const QString &strDeviceName, const QString &strDeviceIP, const QString &strDevicePort, bool enabled)
{
    if (strDeviceName.isEmpty() || strDeviceIP.isEmpty() || strDevicePort.isEmpty())
    {
        DEBUG_MESSAGE("FAILED to add device: name, IP, and port must not be empty");
        return false;
    }

    QSqlQuery addDeviceQuery(m_database);
    // 使用占位符绑定设备信息，避免将输入拼接进 SQL。
    addDeviceQuery.prepare(
        "INSERT INTO DEVICES(DEVICE_NAME, DEVICE_IP, DEVICE_PORT, DEVICE_ENABLED) "
        "VALUES (:name, :ip, :port, :enabled)");
    addDeviceQuery.bindValue(":name", strDeviceName);
    addDeviceQuery.bindValue(":ip", strDeviceIP);
    addDeviceQuery.bindValue(":port", strDevicePort);
    addDeviceQuery.bindValue(":enabled", enabled);
    if (!addDeviceQuery.exec())
    {
        DEBUG_MESSAGE("FAILED to add device: " << strDeviceName << " - " << addDeviceQuery.lastError().text());
        return false;
    }
    return true;
}

bool DBManager::deleteDevice(const QString &strDeviceName)
{
    if (strDeviceName.isEmpty())
    {
        DEBUG_MESSAGE("FAILED to delete device: name must not be empty");
        return false;
    }

    QSqlQuery deleteDeviceQuery(m_database);
    // 使用占位符绑定设备名，避免将输入拼接进 SQL。
    deleteDeviceQuery.prepare("DELETE FROM DEVICES WHERE DEVICE_NAME = :name");
    deleteDeviceQuery.bindValue(":name", strDeviceName);
    if (!deleteDeviceQuery.exec())
    {
        DEBUG_MESSAGE("FAILED to delete device: " << strDeviceName << " - " << deleteDeviceQuery.lastError().text());
        return false;
    }
    return true;
}

bool DBManager::editDevice(const QString &strOldDeviceName, const QString &strNewDeviceName,
                           const QString &strNewDeviceIP, const QString &strNewDevicePort, bool enabled)
{
    if (strOldDeviceName.isEmpty() || strNewDeviceName.isEmpty()
        || strNewDeviceIP.isEmpty() || strNewDevicePort.isEmpty())
    {
        DEBUG_MESSAGE("FAILED to edit device: name, IP, and port must not be empty");
        return false;
    }

    QSqlQuery editDeviceQuery(m_database);
    // 使用占位符绑定设备信息，避免将输入拼接进 SQL。
    editDeviceQuery.prepare(
        "UPDATE DEVICES SET DEVICE_NAME = :newName, DEVICE_IP = :ip, DEVICE_PORT = :port, DEVICE_ENABLED = :enabled "
        "WHERE DEVICE_NAME = :oldName");
    editDeviceQuery.bindValue(":newName", strNewDeviceName);
    editDeviceQuery.bindValue(":ip", strNewDeviceIP);
    editDeviceQuery.bindValue(":port", strNewDevicePort);
    editDeviceQuery.bindValue(":enabled", enabled);
    editDeviceQuery.bindValue(":oldName", strOldDeviceName);
    if (!editDeviceQuery.exec())
    {
        DEBUG_MESSAGE("FAILED to edit device: " << strOldDeviceName << " - " << editDeviceQuery.lastError().text());
        return false;
    }
    if (editDeviceQuery.numRowsAffected() == 0)
    {
        DEBUG_MESSAGE("FAILED to edit device: no device found with name " << strOldDeviceName);
        return false;
    }
    return true;
}

bool DBManager::getDeviceList(QVector<DeviceInfo> &devices)
{
    QSqlQuery queryDeviceList(m_database);
    if (!queryDeviceList.exec("SELECT DEVICE_NAME, DEVICE_IP, DEVICE_PORT, DEVICE_ENABLED FROM DEVICES ORDER BY DEVICE_NAME"))
    {
        DEBUG_MESSAGE("FAILED to query device list: " << queryDeviceList.lastError().text());
        return false;
    }

    QVector<DeviceInfo> loadedDevices;
    while (queryDeviceList.next())
    {
        DeviceInfo deviceInfo;
        deviceInfo.strDeviceName = queryDeviceList.value("DEVICE_NAME").toString();
        deviceInfo.strDeviceIP = queryDeviceList.value("DEVICE_IP").toString();
        deviceInfo.strDevicePort = queryDeviceList.value("DEVICE_PORT").toString();
        deviceInfo.bDeviceEnabled = queryDeviceList.value("DEVICE_ENABLED").toBool();
        loadedDevices.append(deviceInfo);
    }

    devices = loadedDevices;
    return true;
}


void DBManager::closeDatabase()
{
    if (m_database.isOpen())
    {
        m_database.close();
    }

    // 移除连接前释放成员持有的数据库句柄，避免 Qt 报告连接仍在使用。
    m_database = QSqlDatabase();
    if (QSqlDatabase::contains(m_strDBName))
    {
        QSqlDatabase::removeDatabase(m_strDBName);
    }
}


QString DBManager::hashPassword(const QString &plainPassword)
{
    QByteArray raw = plainPassword.toUtf8();
    QByteArray hash = QCryptographicHash::hash(raw, QCryptographicHash::Sha256);
    return QString(hash.toHex());
}


bool DBManager::verifyUser(const QString &username, const QString &plainPassword)
{
    if (username.isEmpty() || plainPassword.isEmpty())
    {
        return false;
    }

    QString passHash = hashPassword(plainPassword);

    for (const auto &userInfo : m_vecUserInfo)
    {
        if (userInfo.username == username && userInfo.passwordHash == passHash)
        {
            return true;
        }
    }
    return false;
}
