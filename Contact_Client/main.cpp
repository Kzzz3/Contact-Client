#include "MainWindow.h"
#include "LoginDlg.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);
    LoginDlg login_dlg(nullptr);
    login_dlg.show();

    return application.exec();
}