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

    // 创建用户表
    if (!createTablesQuery.exec(
            "CREATE TABLE IF NOT EXISTS USERS"
            "(USER_NAME TEXT NOT NULL, USER_PASSWORD TEXT NOT NULL, "
            "PRIMARY KEY(USER_NAME))"))
    {
        DEBUG_MESSAGE("FAILED to create USERS table: " << createTablesQuery.lastError().text());
        return false;
    }

    // 创建设备表
    if (!createTablesQuery.exec(
            "CREATE TABLE IF NOT EXISTS DEVICES"
            "(DEVICE_NAME TEXT NOT NULL, DEVICE_IP TEXT NOT NULL, "
            "DEVICE_PORT TEXT NOT NULL, DEVICE_ENABLED BOOLEAN DEFAULT TRUE, "
            "PRIMARY KEY(DEVICE_NAME))"))
    {
        DEBUG_MESSAGE("FAILED to create DEVICES table: " << createTablesQuery.lastError().text());
        return false;
    }

    // 创建程序表
    if (!createTablesQuery.exec(
            "CREATE TABLE IF NOT EXISTS PROCEDURE"
            "(ID INTEGER PRIMARY KEY AUTOINCREMENT, "
            "PROCEDURE_NAME TEXT NOT NULL, PROCEDURE_TYPE TEXT NOT NULL, "
            "STEP_NUMBER INTEGER NOT NULL, FIRST_CHOICE INTEGER NOT NULL DEFAULT 0)"))
    {
        DEBUG_MESSAGE("FAILED to create PROCEDURE table: " << createTablesQuery.lastError().text());
        return false;
    }

    QSqlQuery procedureColumns(m_database);
    if (procedureColumns.exec("PRAGMA table_info(PROCEDURE)")
        && procedureColumns.isActive())
    {
        bool hasStepCount = false;
        bool hasStepNumber = false;
        while (procedureColumns.next())
        {
            const QString columnName = procedureColumns.value("name").toString();
            hasStepCount = hasStepCount || columnName.compare("STEP_COUNT", Qt::CaseInsensitive) == 0;
            hasStepNumber = hasStepNumber || columnName.compare("STEP_NUMBER", Qt::CaseInsensitive) == 0;
        }

        if (hasStepCount && !hasStepNumber)
        {
            QSqlQuery renameStepColumn(m_database);
            if (!renameStepColumn.exec("ALTER TABLE PROCEDURE RENAME COLUMN STEP_COUNT TO STEP_NUMBER"))
            {
                DEBUG_MESSAGE("FAILED to rename PROCEDURE.STEP_COUNT: " << renameStepColumn.lastError().text());
                return false;
            }
        }
    }

    // 创建程序步骤表, 存储每一个程序里面的详细步骤，通过外键关联到主表
        if( !createTablesQuery.exec(
            "CREATE TABLE IF NOT EXISTS PROCEDURE_STEP"
            "(ID INTEGER PRIMARY KEY AUTOINCREMENT, "
            "PROCEDURE_ID INTEGER NOT NULL, STEP_NO INTEGER NOT NULL, "
            "IS_RINSE BOOLEAN DEFAULT FALSE, POSITION INTEGER, "
            "ADD_VOLUME INTEGER, REAGENT_NAME TEXT, REAGENT_TYPE TEXT, "
            "DURATION REAL, HEAT_COOL INTEGER, TEMPERATURE INTEGER, CONFIG TEXT, "
            "FOREIGN KEY(PROCEDURE_ID) REFERENCES PROCEDURE(ID) ON DELETE CASCADE)"))
    {
        DEBUG_MESSAGE("FAILED to create PROCEDURE_STEP table: " << createTablesQuery.lastError().text());
        return false;
    }

    return true;
}

/***************************************************************************************************/
/*********************************** 用 户 管 理 ****************************************************/
/***************************************************************************************************/
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

QString DBManager::hashPassword(const QString &plainPassword)
{
    QByteArray raw = plainPassword.toUtf8();
    QByteArray hash = QCryptographicHash::hash(raw, QCryptographicHash::Sha256);
    return QString(hash.toHex());
}

/***************************************************************************************************/
/*********************************** 设 备 管 理 ****************************************************/
/***************************************************************************************************/
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


/***************************************************************************************************/
/*********************************** 程 序 管 理 ****************************************************/
/***************************************************************************************************/
bool DBManager::addProcedure(const QString &strName, const QString &strType, int iStep, bool bFirstChoise)
{
    if (strName.isEmpty() || strType.isEmpty() || iStep < 0)
    {
        DEBUG_MESSAGE("FAILED to add procedure: name, type must not be empty and step count must be positive");
        return false;
    }

    QSqlQuery addProcedureQuery(m_database);
    // 使用占位符绑定程序信息，避免将输入拼接进 SQL。
    addProcedureQuery.prepare(
        "INSERT INTO PROCEDURE(PROCEDURE_NAME, PROCEDURE_TYPE, STEP_NUMBER, FIRST_CHOICE) "
        "VALUES (:name, :type, :stepNumber, :firstChoice)");
    addProcedureQuery.bindValue(":name", strName);
    addProcedureQuery.bindValue(":type", strType);
    addProcedureQuery.bindValue(":stepNumber", iStep);
    addProcedureQuery.bindValue(":firstChoice", bFirstChoise ? 1 : 0);
    if (!addProcedureQuery.exec())
    {
        DEBUG_MESSAGE("FAILED to add procedure: " << strName << " - " << addProcedureQuery.lastError().text());
        return false;
    }
    return true;
}

bool DBManager::deleteProcedure(int procedureId)
{
    if (procedureId <= 0)
    {
        DEBUG_MESSAGE("FAILED to delete procedure: invalid procedure ID");
        return false;
    }

    QSqlQuery deleteProcedureQuery(m_database);
    // 使用占位符绑定程序ID，避免将输入拼接进 SQL。
    deleteProcedureQuery.prepare("DELETE FROM PROCEDURE WHERE ID = :id");
    deleteProcedureQuery.bindValue(":id", procedureId);
    if (!deleteProcedureQuery.exec())
    {
        DEBUG_MESSAGE("FAILED to delete procedure with ID: " << procedureId << " - " << deleteProcedureQuery.lastError().text());
        return false;
    }
    return true;
}

bool DBManager::editProcedure(int procedureId, const QString &strNewName, const QString &strNewType, int iNewStep, bool bFirstChoise)
{
    if (procedureId <= 0 || strNewName.isEmpty() || strNewType.isEmpty() || iNewStep < 0)
    {
        DEBUG_MESSAGE("FAILED to edit procedure: invalid ID or empty name/type or non-positive step count");
        return false;
    }

    QSqlQuery editProcedureQuery(m_database);
    // 使用占位符绑定程序信息，避免将输入拼接进 SQL。
    editProcedureQuery.prepare(
        "UPDATE PROCEDURE SET PROCEDURE_NAME = :newName, PROCEDURE_TYPE = :newType, "
        "STEP_NUMBER = :newStepNumber, FIRST_CHOICE = :firstChoice "
        "WHERE ID = :id");
    editProcedureQuery.bindValue(":newName", strNewName);
    editProcedureQuery.bindValue(":newType", strNewType);
    editProcedureQuery.bindValue(":newStepNumber", iNewStep);
    editProcedureQuery.bindValue(":firstChoice", bFirstChoise ? 1 : 0);
    editProcedureQuery.bindValue(":id", procedureId);
    if (!editProcedureQuery.exec())
    {
        DEBUG_MESSAGE("FAILED to edit procedure with ID: " << procedureId << " - " << editProcedureQuery.lastError().text());
        return false;
    }
    if (editProcedureQuery.numRowsAffected() == 0)
    {
        DEBUG_MESSAGE("FAILED to edit procedure: no procedure found with ID " << procedureId);
        return false;
    }
    return true;
}

bool DBManager::getProcedureList(QVector<ProcedureInfo> &procedures)
{
    QSqlQuery queryProcedureList(m_database);
    if (!queryProcedureList.exec("SELECT ID, PROCEDURE_NAME, PROCEDURE_TYPE, STEP_NUMBER, FIRST_CHOICE FROM PROCEDURE ORDER BY PROCEDURE_NAME"))
    {
        DEBUG_MESSAGE("FAILED to query procedure list: " << queryProcedureList.lastError().text());
        return false;
    }

    QVector<ProcedureInfo> loadedProcedures;
    while (queryProcedureList.next())
    {
        ProcedureInfo procedureInfo;
        procedureInfo.id = queryProcedureList.value("ID").toInt();
        procedureInfo.strName = queryProcedureList.value("PROCEDURE_NAME").toString();
        procedureInfo.strType = queryProcedureList.value("PROCEDURE_TYPE").toString();
        procedureInfo.iStepNumber = queryProcedureList.value("STEP_NUMBER").toInt();
        procedureInfo.bFirstChoice = queryProcedureList.value("FIRST_CHOICE").toBool();
        loadedProcedures.append(procedureInfo);
    }

    procedures = loadedProcedures;
    return true;
}

/***************************************************************************************************/
/******************************** 程 序 步 骤 管 理 *************************************************/
/***************************************************************************************************/
bool DBManager::addProcedureStep(int iProcedureId, int iStepNumber, bool bRinse, int iPosition, int iAddVolume,
                                    const QString &strReagentName, const QString &strReagentType,
                                    double dDuration, int iHeatCool, int iTemperature, const QString &strConfig)
{
        if (iProcedureId <= 0 || iStepNumber <= 0)
        {
            DEBUG_MESSAGE("FAILED to add procedure step: invalid procedure ID or step number");
            return false;
        }
    
        QSqlQuery addProcedureStepQuery(m_database);
        // 使用占位符绑定步骤信息，避免将输入拼接进 SQL。
        addProcedureStepQuery.prepare(
            "INSERT INTO PROCEDURE_STEP(PROCEDURE_ID, STEP_NO, IS_RINSE, POSITION, ADD_VOLUME, "
            "REAGENT_NAME, REAGENT_TYPE, DURATION, HEAT_COOL, TEMPERATURE, CONFIG) "
            "VALUES (:procedureId, :stepNo, :isRinse, :position, :addVolume, "
            ":reagentName, :reagentType, :duration, :heatCool, :temperature, :config)");
        addProcedureStepQuery.bindValue(":procedureId", iProcedureId);
        addProcedureStepQuery.bindValue(":stepNo", iStepNumber);
        addProcedureStepQuery.bindValue(":isRinse", bRinse ? 1 : 0);
        addProcedureStepQuery.bindValue(":position", iPosition);
        addProcedureStepQuery.bindValue(":addVolume", iAddVolume);
        addProcedureStepQuery.bindValue(":reagentName", strReagentName);
        addProcedureStepQuery.bindValue(":reagentType", strReagentType);
        addProcedureStepQuery.bindValue(":duration", dDuration);
        addProcedureStepQuery.bindValue(":heatCool", iHeatCool);
        addProcedureStepQuery.bindValue(":temperature", iTemperature);
        addProcedureStepQuery.bindValue(":config", strConfig);
    
        if (!addProcedureStepQuery.exec())
        {
            DEBUG_MESSAGE("FAILED to add procedure step for procedure ID: " << iProcedureId << " - " << addProcedureStepQuery.lastError().text());
            return false;
        }
        return true;
}

bool DBManager::deleteProcedureStep(int iProcedureId, int iStepNumber)
{
        if (iProcedureId <= 0 || iStepNumber <= 0)
        {
            DEBUG_MESSAGE("FAILED to delete procedure step: invalid procedure ID or step number");
            return false;
        }
    
        QSqlQuery deleteProcedureStepQuery(m_database);
        // 使用占位符绑定步骤信息，避免将输入拼接进 SQL。
        deleteProcedureStepQuery.prepare(
            "DELETE FROM PROCEDURE_STEP WHERE PROCEDURE_ID = :procedureId AND STEP_NO = :stepNo");
        deleteProcedureStepQuery.bindValue(":procedureId", iProcedureId);
        deleteProcedureStepQuery.bindValue(":stepNo", iStepNumber);
    
        if (!deleteProcedureStepQuery.exec())
        {
            DEBUG_MESSAGE("FAILED to delete procedure step for procedure ID: " << iProcedureId << ", step number: " << iStepNumber << " - " << deleteProcedureStepQuery.lastError().text());
            return false;
        }
        return true;
}

bool DBManager::editProcedureStep(int iProcedureId, int iStepNumber, bool bRinse, int iPosition, int iAddVolume,
                                    const QString &strReagentName, const QString &strReagentType,
                                    double dDuration, int iHeatCool, int iTemperature, const QString &strConfig)
{
        if (iProcedureId <= 0 || iStepNumber <= 0)
        {
            DEBUG_MESSAGE("FAILED to edit procedure step: invalid procedure ID or step number");
            return false;
        }
    
        QSqlQuery editProcedureStepQuery(m_database);
        // 使用占位符绑定步骤信息，避免将输入拼接进 SQL。
        editProcedureStepQuery.prepare(
            "UPDATE PROCEDURE_STEP SET IS_RINSE = :isRinse, POSITION = :position, ADD_VOLUME = :addVolume, "
            "REAGENT_NAME = :reagentName, REAGENT_TYPE = :reagentType, DURATION = :duration, "
            "HEAT_COOL = :heatCool, TEMPERATURE = :temperature, CONFIG = :config "
            "WHERE PROCEDURE_ID = :procedureId AND STEP_NO = :stepNo");
        editProcedureStepQuery.bindValue(":isRinse", bRinse ? 1 : 0);
        editProcedureStepQuery.bindValue(":position", iPosition);
        editProcedureStepQuery.bindValue(":addVolume", iAddVolume);
        editProcedureStepQuery.bindValue(":reagentName", strReagentName);
        editProcedureStepQuery.bindValue(":reagentType", strReagentType);
        editProcedureStepQuery.bindValue(":duration", dDuration);
        editProcedureStepQuery.bindValue(":heatCool", iHeatCool);
        editProcedureStepQuery.bindValue(":temperature", iTemperature);
        editProcedureStepQuery.bindValue(":config", strConfig);
        editProcedureStepQuery.bindValue(":procedureId", iProcedureId);
        editProcedureStepQuery.bindValue(":stepNo", iStepNumber);
    
        if (!editProcedureStepQuery.exec())
        {
            DEBUG_MESSAGE("FAILED to edit procedure step for procedure ID: " << iProcedureId << ", step number: " << iStepNumber << " - " << editProcedureStepQuery.lastError().text());
            return false;
        }
        if (editProcedureStepQuery.numRowsAffected() == 0)
        {
            DEBUG_MESSAGE("FAILED to edit procedure step: no matching step found");
            return false;
        }
        return true;
}

bool DBManager::getProcedureStepList(int iProcedureId, QVector<ProcedureStepInfo> &steps)
{
        if (iProcedureId <= 0)
        {
            DEBUG_MESSAGE("FAILED to get procedure steps: invalid procedure ID");
            return false;
        }
    
        QSqlQuery queryProcedureSteps(m_database);
        queryProcedureSteps.prepare(
            "SELECT STEP_NO, IS_RINSE, POSITION, ADD_VOLUME, REAGENT_NAME, REAGENT_TYPE, "
            "DURATION, HEAT_COOL, TEMPERATURE, CONFIG "
            "FROM PROCEDURE_STEP WHERE PROCEDURE_ID = :procedureId ORDER BY STEP_NO");
        queryProcedureSteps.bindValue(":procedureId", iProcedureId);
    
        if (!queryProcedureSteps.exec())
        {
            DEBUG_MESSAGE("FAILED to query procedure steps for procedure ID: " << iProcedureId << " - " << queryProcedureSteps.lastError().text());
            return false;
        }
    
        QVector<ProcedureStepInfo> loadedSteps;
        while (queryProcedureSteps.next())
        {
            ProcedureStepInfo stepInfo;
            stepInfo.iStepNumber = queryProcedureSteps.value("STEP_NO").toInt();
            stepInfo.bRinse = queryProcedureSteps.value("IS_RINSE").toBool();
            stepInfo.iPosition = queryProcedureSteps.value("POSITION").toInt();
            stepInfo.iAddVolume = queryProcedureSteps.value("ADD_VOLUME").toInt();
            stepInfo.strReagentName = queryProcedureSteps.value("REAGENT_NAME").toString();
            stepInfo.strReagentType = queryProcedureSteps.value("REAGENT_TYPE").toString();
            stepInfo.dDuration = queryProcedureSteps.value("DURATION").toDouble();
            stepInfo.iHeatCool = queryProcedureSteps.value("HEAT_COOL").toInt();
            stepInfo.iTemperature = queryProcedureSteps.value("TEMPERATURE").toInt();
            stepInfo.strConfig = queryProcedureSteps.value("CONFIG").toString();
    
            loadedSteps.append(stepInfo);
        }
    
        steps = loadedSteps;
        return true;
}
    