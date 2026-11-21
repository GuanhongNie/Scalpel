#pragma once

#include <QEvent>
#include <QWidget>
#include <QVector>
#include "DBManager.h"

class QTableWidget;

namespace Ui {
class ProcedureManager;
}

class ProcedureManager : public QWidget
{
    Q_OBJECT

public:
    explicit ProcedureManager(QWidget *parent = nullptr);
    ~ProcedureManager() override;

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void updateTableRows(QTableWidget *table, bool addPreferredCheckboxes);
    void refreshTable();
    void refreshFromDatabase();
    void onAddProcedure();
    void onEditProcedure();
    void onDeleteProcedure();

    Ui::ProcedureManager *ui;
    QVector<DBManager::ProcedureInfo> m_vecProcedure;
};
