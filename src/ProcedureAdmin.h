#pragma once

#include <QDialog>
#include "DBManager.h"

namespace Ui {
class ProcedureAdmin;
}

class ProcedureAdmin : public QDialog
{
    Q_OBJECT

public:
    explicit ProcedureAdmin(QWidget *parent = nullptr);
    ~ProcedureAdmin() override;

    void setProcedure(const DBManager::ProcedureInfo &procedure);
    DBManager::ProcedureInfo procedure() const;
    void setExistingProcedureNames(const QStringList &procedureNames);
    void setCurrentProcedureName(const QString &procedureName);

private slots:
    void onOkClicked();

private:
    Ui::ProcedureAdmin *ui;
    QStringList m_existingProcedureNames;
    QString m_currentProcedureName;
};
