#include "ProcedureAdmin.h"
#include "ui_ProcedureAdmin.h"

#include <QMessageBox>

ProcedureAdmin::ProcedureAdmin(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ProcedureAdmin)
{
    ui->setupUi(this);
    setWindowTitle("Procedure Detail");

    connect(ui->pushButtonOK, &QPushButton::clicked, this, &ProcedureAdmin::onOkClicked);
    connect(ui->pushButtonCancel, &QPushButton::clicked, this, &QDialog::reject);
}

ProcedureAdmin::~ProcedureAdmin()
{
    delete ui;
}

void ProcedureAdmin::setProcedure(const DBManager::ProcedureInfo &procedure)
{
    ui->lineEditProcedureName->setText(procedure.strName);
    const int typeIndex = ui->comboBoxProcedureType->findText(procedure.strType);
    ui->comboBoxProcedureType->setCurrentIndex(typeIndex >= 0 ? typeIndex : 0);
    ui->lineEditStepNumber->setText(QString::number(procedure.iStepNumber));
    ui->checkBoxFirstChoice->setChecked(procedure.bFirstChoice);
}

DBManager::ProcedureInfo ProcedureAdmin::procedure() const
{
    DBManager::ProcedureInfo procedure;
    procedure.strName = ui->lineEditProcedureName->text().trimmed();
    procedure.strType = ui->comboBoxProcedureType->currentText().trimmed();
    procedure.iStepNumber = ui->lineEditStepNumber->text().toInt();
    procedure.bFirstChoice = ui->checkBoxFirstChoice->isChecked();
    return procedure;
}

void ProcedureAdmin::setExistingProcedureNames(const QStringList &procedureNames)
{
    m_existingProcedureNames = procedureNames;
}

void ProcedureAdmin::setCurrentProcedureName(const QString &procedureName)
{
    m_currentProcedureName = procedureName;
}

void ProcedureAdmin::onOkClicked()
{
    const QString name = ui->lineEditProcedureName->text().trimmed();
    const QString type = ui->comboBoxProcedureType->currentText().trimmed();
    if (name.isEmpty())
    {
        QMessageBox::warning(this, "Invalid input", "Procedure name cannot be empty.");
        return;
    }
    if (type.isEmpty())
    {
        QMessageBox::warning(this, "Invalid input", "Procedure type cannot be empty.");
        return;
    }

    const bool isDuplicate = m_existingProcedureNames.contains(name, Qt::CaseInsensitive)
        && name.compare(m_currentProcedureName, Qt::CaseInsensitive) != 0;
    if (isDuplicate)
    {
        QMessageBox::warning(this, "Duplicate procedure", "A procedure with the same name already exists.");
        return;
    }

    accept();
}
