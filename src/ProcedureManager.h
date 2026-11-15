#pragma once

#include <QWidget>

namespace Ui {
class ProcedureManager;
}

class ProcedureManager : public QWidget
{
    Q_OBJECT

public:
    explicit ProcedureManager(QWidget *parent = nullptr);
    ~ProcedureManager() override;

private:
    Ui::ProcedureManager *ui;
};
