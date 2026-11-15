#pragma once

#include "ui_Login.h"
#include <QMainWindow>

class Login : public QMainWindow {
    Q_OBJECT

public:
    explicit Login(QWidget* parent = nullptr);
    ~Login() override;

private slots:
    void onLoginClicked();

private:
    Ui_Login* ui;
};