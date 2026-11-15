#pragma once

#include <QWidget>

namespace Ui {
class ReagentManager;
}

class ReagentManager : public QWidget
{
    Q_OBJECT

public:
    explicit ReagentManager(QWidget *parent = nullptr);
    ~ReagentManager() override;

private:
    Ui::ReagentManager *ui;
};
