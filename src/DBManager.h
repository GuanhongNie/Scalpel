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
    // 获取单例
    static DBManager &instance();

    void closeDatabase();

    // 用户管理
    bool verifyUser(const QString &strUserName, const QString &strPlainPassword);
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

    // 设备管理
    struct DeviceInfo
    {
        QString strDeviceName;
        QString strDeviceIP;
        QString strDevicePort;
        bool bDeviceEnabled;
    };
    bool addDevice(const QString &strDeviceName, const QString &strDeviceIP, const QString &strDevicePort, bool enabled);
    bool deleteDevice(const QString &strDeviceName);
    bool editDevice(const QString &strOldDeviceName, const QString &strNewDeviceName,
                    const QString &strNewDeviceIP, const QString &strNewDevicePort, bool enabled);
    bool getDeviceList(QVector<DeviceInfo> &devices);

    // 程序管理
    struct ProcedureInfo
    {
        int id;
        QString strName;
        QString strType;
        int iStepNumber;
        bool bFirstChoice;
    };
    bool addProcedure(const QString &strName, const QString &strType, int iStep, bool bFirstChoise);
    bool deleteProcedure(int procedureId);
    bool editProcedure(int procedureId, const QString &strNewName, const QString &strNewType, int iNewStep, bool bFirstChoise);
    bool getProcedureList(QVector<ProcedureInfo> &procedures);

    // 程序步骤管理
    struct ProcedureStepInfo
    {
        int iProcedureId;
        int iStepNumber;
        bool bRinse;
        int iPosition;
        int iAddVolume;
        QString strReagentName;
        QString strReagentType;
        double dDuration;
        int iHeatCool;
        int iTemperature;
        QString strConfig;
    };
    bool addProcedureStep(int iProcedureId, int iStepNumber, bool bRinse, int iPosition, int iAddVolume,
                 const QString &strReagentName, const QString &strReagentType, double dDuration,
                 int iHeatCool, int iTemperature, const QString &strConfig);
    bool deleteProcedureStep(int iProcedureId, int iStepNumber);
    bool editProcedureStep(int iProcedureId, int iStepNumber, bool bRinse,   
                int iPosition, int iAddVolume, const QString &strReagentName,
                     const QString &strReagentType, double dDuration, int iHeatCool,
                     int iTemperature, const QString &strConfig);   
    bool getProcedureStepList(int iProcedureId, QVector<ProcedureStepInfo> &steps);


private:
    explicit DBManager(QObject *parent = nullptr);
    ~DBManager();
    DBManager(const DBManager &) = delete;
    DBManager &operator=(const DBManager &) = delete;

    bool createTables();

    QSqlDatabase m_database;
    const QString m_strDBName;

    // 把明文转为 SHA256 十六进制哈希
    QString hashPassword(const QString &plainPassword);    

};

#endif // DBMANAGER_H
