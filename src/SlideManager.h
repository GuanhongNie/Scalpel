#pragma once

#include <QWidget>

namespace Ui {
class SlideManager;
}

class SlideManager : public QWidget
{
    Q_OBJECT

public:
    explicit SlideManager(QWidget *parent = nullptr);
    ~SlideManager() override;

private:
    Ui::SlideManager *ui;
};
