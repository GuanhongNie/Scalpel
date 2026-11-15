#include "ProcedureManager.h"
#include "ui_ProcedureManager.h"

ProcedureManager::ProcedureManager(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ProcedureManager)
{
    ui->setupUi(this);
}

ProcedureManager::~ProcedureManager()
{
    delete ui;
}
