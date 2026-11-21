#include "ProcedureManager.h"
#include "ProcedureAdmin.h"
#include "ui_ProcedureManager.h"

#include <QCheckBox>
#include <QColor>
#include <QMessageBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QPalette>
#include <QTableWidget>
#include <QTimer>
#include <QVector>
#include <QWidget>
#include <QTableWidgetItem>

ProcedureManager::ProcedureManager(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ProcedureManager)
{
    ui->setupUi(this);
    ui->mainLayout->setStretch(0, 4);
    ui->mainLayout->setStretch(1, 6);
    ui->procedureLayout->setStretch(1, 1);
    ui->stepLayout->setStretch(1, 1);

    const auto configureTable = [](QTableWidget *table, const QStringList &headers,
                                   QHeaderView::ResizeMode rowResizeMode) {
        table->setColumnCount(headers.size());
        table->setHorizontalHeaderLabels(headers);
        table->setAlternatingRowColors(true);
        table->setSelectionBehavior(QAbstractItemView::SelectRows);
        table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        table->verticalHeader()->hide();
        table->verticalHeader()->setDefaultSectionSize(24);
        table->verticalHeader()->setSectionResizeMode(rowResizeMode);
        table->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
        table->horizontalHeader()->setStyleSheet(
            "QHeaderView::section {"
            "background-color: #C8DFF0;"
            "color: #17324D;"
            "font-weight: bold;"
            "padding: 4px;"
            "border: 1px solid #A9C3D8;"
            "}");

        QPalette palette = table->palette();
        palette.setColor(QPalette::AlternateBase, QColor(220, 248, 247));
        table->setPalette(palette);
    };

    configureTable(ui->tblProcedure,
                   {"程序名称", "程序类型", "步骤", "首选"},
                   QHeaderView::Fixed);
    configureTable(ui->tblStep, {
        "编号", "冲洗", "位置", "加液量", "试剂名",
        "试剂类型", "时间", "加/散热", "温度", "配置"
    }, QHeaderView::Stretch);

    ui->tblProcedure->installEventFilter(this);
    ui->tblStep->installEventFilter(this);
    connect(ui->btnNew, &QPushButton::clicked, this, &ProcedureManager::onAddProcedure);
    connect(ui->btnEdit, &QPushButton::clicked, this, &ProcedureManager::onEditProcedure);
    connect(ui->btnDelete1, &QPushButton::clicked, this, &ProcedureManager::onDeleteProcedure);
    connect(ui->tblProcedure, &QTableWidget::itemSelectionChanged, this, [this]() {
        const bool hasSelection = ui->tblProcedure->currentRow() >= 0
            && ui->tblProcedure->currentRow() < m_vecProcedure.size();
        ui->btnCopy->setEnabled(hasSelection);
        ui->btnEdit->setEnabled(hasSelection);
        ui->btnDelete1->setEnabled(hasSelection);
    });
    connect(ui->tblProcedure, &QTableWidget::itemDoubleClicked,
            this, [this](QTableWidgetItem *) { onEditProcedure(); });

    ui->btnNew->setEnabled(true);
    ui->btnCopy->setEnabled(false);
    ui->btnEdit->setEnabled(false);
    ui->btnDelete1->setEnabled(false);
    refreshFromDatabase();
    updateTableRows(ui->tblProcedure, true);
    updateTableRows(ui->tblStep, false);
}

ProcedureManager::~ProcedureManager()
{
    delete ui;
}

bool ProcedureManager::eventFilter(QObject *watched, QEvent *event)
{
    if ((watched == ui->tblProcedure || watched == ui->tblStep)
        && (event->type() == QEvent::Resize || event->type() == QEvent::Show))
    {
        QTimer::singleShot(0, this, [this]() {
            updateTableRows(ui->tblProcedure, true);
            updateTableRows(ui->tblStep, false);
        });
    }

    return QWidget::eventFilter(watched, event);
}

void ProcedureManager::updateTableRows(QTableWidget *table, bool addPreferredCheckboxes)
{
    const int defaultRowHeight = table->verticalHeader()->defaultSectionSize();
    if (!addPreferredCheckboxes)
    {
        const int rowCount = qMax(1, (table->viewport()->height() + defaultRowHeight - 1) / defaultRowHeight);
        if (table->rowCount() != rowCount)
            table->setRowCount(rowCount);
    }

    const QVector<int> columnWeights = addPreferredCheckboxes
        ? QVector<int>{45, 25, 15, 15}
        : QVector<int>{6, 6, 6, 10, 19, 12, 8, 10, 8, 15};
    int totalWeight = 0;
    for (const int weight : columnWeights)
        totalWeight += weight;

    const int availableWidth = table->viewport()->width();
    int assignedWidth = 0;
    for (int column = 0; column < columnWeights.size(); ++column)
    {
        const int columnWidth = column == columnWeights.size() - 1
            ? availableWidth - assignedWidth
            : availableWidth * columnWeights.at(column) / totalWeight;
        table->setColumnWidth(column, columnWidth);
        assignedWidth += columnWidth;
    }

    if (addPreferredCheckboxes)
    {
        for (int row = 0; row < table->rowCount(); ++row)
        {
            if (table->cellWidget(row, 3))
                continue;

            auto *cell = new QWidget(table);
            auto *layout = new QHBoxLayout(cell);
            layout->setContentsMargins(0, 0, 0, 0);
            layout->setAlignment(Qt::AlignCenter);
            layout->addWidget(new QCheckBox(cell));
            table->setCellWidget(row, 3, cell);
        }
    }
}

void ProcedureManager::refreshTable()
{
    ui->tblProcedure->clearContents();
    ui->tblProcedure->setRowCount(m_vecProcedure.size());
    ui->tblProcedure->clearSelection();

    for (int row = 0; row < m_vecProcedure.size(); ++row)
    {
        const DBManager::ProcedureInfo &procedure = m_vecProcedure.at(row);
        ui->tblProcedure->setItem(row, 0, new QTableWidgetItem(procedure.strName));
        ui->tblProcedure->setItem(row, 1, new QTableWidgetItem(procedure.strType));
        ui->tblProcedure->setItem(row, 2, new QTableWidgetItem(QString::number(procedure.iStepNumber)));
        auto *cell = new QWidget(ui->tblProcedure);
        auto *layout = new QHBoxLayout(cell);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setAlignment(Qt::AlignCenter);
        auto *checkBox = new QCheckBox(cell);
        checkBox->setChecked(procedure.bFirstChoice);
        checkBox->setEnabled(false);
        layout->addWidget(checkBox);
        ui->tblProcedure->setCellWidget(row, 3, cell);
    }

    ui->btnNew->setEnabled(true);
    ui->btnCopy->setEnabled(false);
    ui->btnEdit->setEnabled(false);
    ui->btnDelete1->setEnabled(false);
}

void ProcedureManager::refreshFromDatabase()
{
    QVector<DBManager::ProcedureInfo> procedures;
    if (!DBManager::instance().getProcedureList(procedures))
    {
        QMessageBox::critical(this, "程序管理", "从数据库加载程序列表失败。");
        return;
    }

    m_vecProcedure = procedures;
    refreshTable();
}

void ProcedureManager::onAddProcedure()
{
    QStringList existingNames;
    for (const DBManager::ProcedureInfo &procedure : m_vecProcedure)
        existingNames << procedure.strName;

    ProcedureAdmin dialog(this);
    dialog.setWindowTitle("新建程序");
    dialog.setExistingProcedureNames(existingNames);
    DBManager::ProcedureInfo procedure;
    procedure.iStepNumber = 0;
    dialog.setProcedure(procedure);

    if (dialog.exec() == QDialog::Accepted)
    {
        procedure = dialog.procedure();
        if (!DBManager::instance().addProcedure(
                procedure.strName,
                procedure.strType,
                procedure.iStepNumber,
                procedure.bFirstChoice))
        {
            QMessageBox::critical(this, "新建程序", "保存程序到数据库失败。程序名称可能已存在。");
            return;
        }

        refreshFromDatabase();
    }
}

void ProcedureManager::onEditProcedure()
{
    const int currentRow = ui->tblProcedure->currentRow();
    if (currentRow < 0 || currentRow >= m_vecProcedure.size())
    {
        QMessageBox::warning(this, "编辑程序", "请先选择要编辑的程序。");
        return;
    }

    QStringList existingNames;
    for (int i = 0; i < m_vecProcedure.size(); ++i)
    {
        if (i != currentRow)
            existingNames << m_vecProcedure.at(i).strName;
    }

    ProcedureAdmin dialog(this);
    dialog.setWindowTitle("编辑程序");
    dialog.setProcedure(m_vecProcedure.at(currentRow));
    dialog.setExistingProcedureNames(existingNames);
    dialog.setCurrentProcedureName(m_vecProcedure.at(currentRow).strName);

    if (dialog.exec() == QDialog::Accepted)
    {
        DBManager::ProcedureInfo updatedProcedure = dialog.procedure();
        const DBManager::ProcedureInfo &oldProcedure = m_vecProcedure.at(currentRow);
        if (!DBManager::instance().editProcedure(
                oldProcedure.id,
                updatedProcedure.strName,
                updatedProcedure.strType,
                updatedProcedure.iStepNumber,
                updatedProcedure.bFirstChoice))
        {
            QMessageBox::critical(this, "编辑程序", "更新程序数据库记录失败。");
            return;
        }

        refreshFromDatabase();
    }
}

void ProcedureManager::onDeleteProcedure()
{
    const int currentRow = ui->tblProcedure->currentRow();
    if (currentRow < 0 || currentRow >= m_vecProcedure.size())
    {
        QMessageBox::warning(this, "删除程序", "请先选择要删除的程序。");
        return;
    }

    const DBManager::ProcedureInfo &procedure = m_vecProcedure.at(currentRow);
    const int result = QMessageBox::question(
        this,
        "删除程序",
        QString("确定要删除程序“%1”吗？").arg(procedure.strName),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);

    if (result != QMessageBox::Yes)
        return;

    if (!DBManager::instance().deleteProcedure(procedure.id))
    {
        QMessageBox::critical(this, "删除程序", "从数据库删除程序失败。");
        return;
    }

    refreshFromDatabase();
}
