#include "ReagentManager.h"
#include "ui_ReagentManager.h"

ReagentManager::ReagentManager(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ReagentManager)
{
    ui->setupUi(this);
}

ReagentManager::~ReagentManager()
{
    delete ui;
}
