#include "LoginDlg.h"
#include "ui_LoginDlg.h"

LoginDlg::LoginDlg(QWidget* parent) :
	QDialog(parent),
	ui(new Ui::LoginDlg),
	m_ioContext(new io_context),
	m_sockClient(new tcp::socket(*m_ioContext)),
	m_epRemoteServer(new tcp::endpoint(ip::address::from_string(SERVER_IP), SERVER_PORT))
{
	ui->setupUi(this);
	this->setFixedSize(400, 300);

	//设置图标
	QIcon Icon(":/Image/img/App.ico");
	this->setWindowIcon(Icon);

	//设置主界面背景
	QPixmap pixmapBackground(":/Image/img/LoginBackground.jpg");
	ui->background_label->setScaledContents(true);
	ui->background_label->setPixmap(pixmapBackground.scaled(ui->background_label->size(), Qt::KeepAspectRatio));

	//设置登陆中动画

	m_pLoggingMovie = new QMovie(":/Gif/img/Logging.gif");
	ui->login_label->setMovie(m_pLoggingMovie);
	ui->login_label->setScaledContents(true);

	//设置注册中动画
	m_pRegisteringMovie = new QMovie(":/Gif/img/Registering.gif");
	ui->register_label->setMovie(m_pRegisteringMovie);
	ui->register_label->setScaledContents(true);

	connect(this, &LoginDlg::LoginResult, this, &LoginDlg::LoginResultSlot);
	connect(this, &LoginDlg::RegisterResult, this, &LoginDlg::RegisterResultSlot);

	connect(this, &LoginDlg::ReadError, this, &LoginDlg::ReadErrorSlot);
	connect(this, &LoginDlg::WriteError, this, &LoginDlg::WriteErrorSlot);
	connect(this, &LoginDlg::ConnectError, this, &LoginDlg::ConnectErrorSlot);
}

LoginDlg::~LoginDlg()
{
	delete ui;
}

void LoginDlg::on_register_button_clicked()
{
	this->ui->login_stacked_widget->setCurrentIndex(REGISTER_PAGE_ID);
	m_pRegisteringMovie->start();

	std::thread thdRegister([this]() {
		boost::system::error_code errCode;
		m_sockClient->close();
		m_sockClient->connect(*m_epRemoteServer,errCode);
		if (errCode)
		{
			emit ConnectError(errCode);
			return;
		}

		Packet result_pkt;
		Packet register_pkt(this->ui->user_id_edit->text().toLatin1().data(), this->ui->user_pwd_edit->text().toLatin1().data(), 0,
			Pkt_Type::USER_OP_REGISTER, Pkt_Status::DEFAULT);
		if (std::strcmp(register_pkt._user_inf._user_id, "") == 0 || std::strcmp(register_pkt._user_inf._user_passsword, "") == 0)
		{
			emit RegisterResult(false);
			return;
		}

		register_pkt.encode();
		boost::asio::write(*m_sockClient, buffer(register_pkt._pdata, register_pkt.get_packet_total_size()),errCode);
		if (errCode)
		{
			emit WriteError(errCode);
			return;
		}

		boost::asio::read(*m_sockClient, buffer((char*)&result_pkt, Packet::HEADER_LENGTH),errCode);
		if (errCode)
		{
			emit ReadError(errCode);
			return;
		}

		emit RegisterResult(result_pkt._pkt_inf._status == Pkt_Status::SUCCESSED);
		});
	thdRegister.detach();
}

void LoginDlg::on_login_button_clicked()
{
	this->ui->login_stacked_widget->setCurrentIndex(LOGIN_PAGE_ID);
	m_pLoggingMovie->start();

	std::thread thdLogin([this]() {
		boost::system::error_code errCode;
		m_sockClient->close();
		m_sockClient->connect(*m_epRemoteServer, errCode);
		if (errCode)
		{
			emit ConnectError(errCode);
			return;
		}

		Packet result_pkt;
		Packet register_pkt(this->ui->user_id_edit->text().toLatin1().data(), this->ui->user_pwd_edit->text().toLatin1().data(), 0,
			Pkt_Type::USER_OP_LOGIN, Pkt_Status::DEFAULT);
		register_pkt.encode();

		boost::asio::write(*m_sockClient, buffer(register_pkt._pdata, register_pkt.get_packet_total_size()),errCode);
		if (errCode)
		{
			emit WriteError(errCode);
			return;
		}

		boost::asio::read(*m_sockClient, buffer((char*)&result_pkt, Packet::HEADER_LENGTH),errCode);
		if (errCode)
		{
			emit ReadError(errCode);
			return;
		}

		emit LoginResult(result_pkt._pkt_inf._status == Pkt_Status::SUCCESSED);
		});
	thdLogin.detach();
}

void LoginDlg::LoginResultSlot(bool bResult)
{
	if (bResult)
	{
		User_Inf infUser(this->ui->user_id_edit->text().toLatin1().data(), this->ui->user_pwd_edit->text().toLatin1().data());
		this->m_pMainWindow = new MainWindow(m_ioContext, m_sockClient, infUser);
		this->m_pMainWindow->show();
		this->close();
	}
	else
	{
		QMessageBox::information(this, "登录提示", "账号或密码错误！");

		m_pLoggingMovie->stop();
		this->ui->login_stacked_widget->setCurrentIndex(MAIN_PAGE_ID);
	}
}

void LoginDlg::RegisterResultSlot(bool bResult)
{
	if (bResult)
	{
		QMessageBox::information(this, "注册提示", "注册成功！");
	}
	else
	{
		QMessageBox::information(this, "注册提示", "注册失败！");
	}

	m_pRegisteringMovie->stop();
	this->ui->login_stacked_widget->setCurrentIndex(MAIN_PAGE_ID);
}

void LoginDlg::ReadErrorSlot(boost::system::error_code errCode)
{
	QMessageBox::information(this, "Error", "Read error!");
	this->ui->login_stacked_widget->setCurrentIndex(MAIN_PAGE_ID);
}

void LoginDlg::WriteErrorSlot(boost::system::error_code errCode)
{
	QMessageBox::information(this, "Error", "Write error!");
	this->ui->login_stacked_widget->setCurrentIndex(MAIN_PAGE_ID);
}

void LoginDlg::ConnectErrorSlot(boost::system::error_code errCode)
{
	QMessageBox::information(this, "Error", "Connect error!");
	this->ui->login_stacked_widget->setCurrentIndex(MAIN_PAGE_ID);
}