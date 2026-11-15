#pragma once

#include <QWidget>

namespace Ui {
class LabHistory;
}

class LabHistory : public QWidget
{
    Q_OBJECT

public:
    explicit LabHistory(QWidget *parent = nullptr);
    ~LabHistory() override;

private:
    Ui::LabHistory *ui;
};
