#ifndef LOGINDLG_H
#define LOGINDLG_H

#include <thread>

#include <QMovie>
#include <QDialog>
#include <QMainWindow>
#include <QMessageBox>
#include <boost/asio.hpp>

#include "MainWindow.h"

#define SERVER_IP "43.136.55.98"
//#define SERVER_IP "192.168.199.144"
#define SERVER_PORT 10087

using namespace boost::asio;
using namespace boost::asio::ip;

class MainWindow;

namespace Ui {
class LoginDlg;
}

constexpr unsigned int MAIN_PAGE_ID=0;
constexpr unsigned int LOGIN_PAGE_ID=1;
constexpr unsigned int REGISTER_PAGE_ID=2;

class LoginDlg : public QDialog
{
	Q_OBJECT

public:
    explicit LoginDlg(QWidget *parent = nullptr);
    ~LoginDlg();

 public:
	//Client Data
	io_context* m_ioContext;
	tcp::socket* m_sockClient;
	tcp::endpoint* m_epRemoteServer;

	//UI Data
    Ui::LoginDlg *ui;
	QMovie* m_pLoggingMovie;
	QMovie* m_pRegisteringMovie;
	MainWindow *m_pMainWindow;


signals:
	//Client Signals
	void LoginResult(bool bResult);
	void RegisterResult(bool bResult);

	//Eorror Signals
	void ReadError(boost::system::error_code errCode);
	void WriteError(boost::system::error_code errCode);
	void ConnectError(boost::system::error_code errCode);

 public slots:
	//UI Slots
	void on_login_button_clicked();
	void on_register_button_clicked();

	//Client Slots
	void LoginResultSlot(bool bResult);
	void RegisterResultSlot(bool bResult);

	//Eorror Slots
	void ReadErrorSlot(boost::system::error_code errCode);
	void WriteErrorSlot(boost::system::error_code errCode);
	void ConnectErrorSlot(boost::system::error_code errCode);
};

#endif // LOGINDLG_H
