#include "Login.h"
#include "MainWindow.h"

#include <QDir>
#include <QFile>
#include <QMessageBox>
#include "DebugMessage.h"
#include "DBManager.h"


Login::Login(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui_Login)
{
    ui->setupUi(this);
    connect(ui->pushButton, &QPushButton::clicked, this, &Login::onLoginClicked);

    ui->lineEditUser->setText("1");// 设置默认用户名为 "1"
    ui->lineEditPassword->setText("1"); // 设置默认密码为 "1"
}

Login::~Login()
{
    delete ui;
}

void Login::onLoginClicked()
{
    QString user = ui->lineEditUser->text();
    QString pass = ui->lineEditPassword->text();

    if (user.isEmpty() || pass.isEmpty()) {
        QMessageBox::warning(this, "Login", "Username and password cannot be empty.");
        return;
    }

    if(DBManager::instance().verifyUser(user, pass))
    {
        MainWindow* mainWindow = new MainWindow();
        mainWindow->setAttribute(Qt::WA_DeleteOnClose);
        mainWindow->show();
        this->close();
        return;
    }
    else
    {
        QMessageBox::warning(this, "Login", "Invalid username or password.");        
    }
}