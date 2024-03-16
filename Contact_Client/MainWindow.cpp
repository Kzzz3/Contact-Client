#include "MainWindow.h"

MainWindow::MainWindow(io_context* ioContext, tcp::socket* sockClient, User_Inf infUser) :
	ui(new Ui::MainWindow),
	m_vpCamera(0),
	m_infUser(infUser),
	m_ioContext(ioContext),
	m_sockClient(sockClient),
	m_timerVideoCapture(new QTimer(this)),
	m_timerAudioCapture(new QTimer(this)),
	m_thdRecievePacket([this]() {PacketDealing(); })
{
	ui->setupUi(this);

	QPixmap pixmapMainBackground(":/Image/img/MainBackground.jpg");
	m_labelMainBackground = new QLabel(this);
	m_labelMainBackground->setGeometry(0, 0, this->width(), this->height());
	m_labelMainBackground->setPixmap(pixmapMainBackground.scaled(m_labelMainBackground->size(), Qt::KeepAspectRatio));
	m_labelMainBackground->setScaledContents(true);
	m_labelMainBackground->lower();
	m_labelMainBackground->show();

	QPixmap pixmapVideoBackground(":/Image/img/VideoBackground.jpg");
	m_labelVideoBackground = new QLabel(ui->VideoShow_widget);
	m_labelVideoBackground->setGeometry(0, 0, ui->VideoShow_widget->width(), ui->VideoShow_widget->height());
	m_labelVideoBackground->setPixmap(pixmapVideoBackground.scaled(m_labelVideoBackground->size(), Qt::KeepAspectRatio));
	m_labelVideoBackground->setScaledContents(true);
	m_labelVideoBackground->lower();
	m_labelVideoBackground->show();

	//初始化Contact Audio
	QAudioFormat formatAudio;
	formatAudio.setSampleRate(8000);
	formatAudio.setChannelCount(1);
	formatAudio.setSampleFormat(QAudioFormat::UInt8);

	m_audioSink = new QAudioSink(QMediaDevices::defaultAudioOutput(), formatAudio, this);
	m_audioSource = new QAudioSource(QMediaDevices::defaultAudioInput(), formatAudio, this);
	m_audioDeviceOutput = m_audioSink->start();

	//设置收包线程分离
	m_thdRecievePacket.detach();

	//初始化大厅房间列表
	m_stdmodelRooms = new QStandardItemModel(this);
	QStringList strlistHead = QString("房间ID,房间名,所有者ID,房间容量,当前人数").simplified().split(",");

	m_stdmodelRooms->setHorizontalHeaderLabels(strlistHead);
	ui->RoomsList_tableview->setModel(m_stdmodelRooms);
	ui->RoomsList_tableview->horizontalHeader()->setStretchLastSection(true);
	ui->RoomsList_tableview->setSelectionBehavior(QAbstractItemView::SelectRows);
	ui->RoomsList_tableview->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

	//初始化房间用户显示列表
	m_stdmodelUsers = new QStandardItemModel(this);
	ui->UsersList_listview->setModel(m_stdmodelUsers);

	//初始化视频显示区域
	int nPerWidth = ui->VideoShow_widget->width() / VideoRoom::DOUBLED_ROOM_CAPACITY;
	int nPerHeight = ui->VideoShow_widget->height();
	for (int i = 0; i < VideoRoom::DOUBLED_ROOM_CAPACITY; ++i)
	{
		QLabel* labelTemp = new QLabel(ui->VideoShow_widget);
		labelTemp->setGeometry(i * nPerWidth, 0, nPerWidth, nPerHeight);
		m_vecDoubleUsersLabel.push_back(labelTemp);
	}

	nPerWidth = ui->VideoShow_widget->width() / VideoRoom::PER_ROW_USERS;
	nPerHeight = ui->VideoShow_widget->height() / (VideoRoom::MAX_ROOM_CAPACITY / VideoRoom::PER_ROW_USERS);
	for (int i = 0; i < VideoRoom::MAX_ROOM_CAPACITY; ++i)
	{
		int nRow = i / VideoRoom::PER_ROW_USERS;
		int nCol = i % VideoRoom::PER_ROW_USERS;
		QLabel* labelTemp = new QLabel(ui->VideoShow_widget);
		labelTemp->setGeometry(nCol * nPerWidth, nRow * nPerHeight, nPerWidth, nPerHeight);
		m_vecMutiUsersLabel.push_back(labelTemp);
	}

	//Client Slots
	connect(m_timerVideoCapture, &QTimer::timeout, this, &MainWindow::VideoCapture);
	connect(m_timerAudioCapture, &QTimer::timeout, this, &MainWindow::AudioCapture);
	connect(this, &MainWindow::MatchResult, this, &MainWindow::MatchResultSlot);
	connect(this, &MainWindow::RecieveImage, this, &MainWindow::RecieveImageSlot);
	connect(this, &MainWindow::RecieveAudio, this, &MainWindow::RecieveAudioSlot);
	connect(this, &MainWindow::JoinRoomInfo, this, &MainWindow::JoinRoomInfoSlot);
	connect(this, &MainWindow::LeaveRoomInfo, this, &MainWindow::LeaveRoomInfoSlot);
	connect(this, &MainWindow::JoinRoomResult, this, &MainWindow::JoinRoomResultSlot);
	connect(this, &MainWindow::LeaveRoomResult, this, &MainWindow::LeaveRoomResultSlot);
	connect(this, &MainWindow::RoomsInfoResult, this, &MainWindow::RoomsInfoResultSlot);

	//Errot Slots
	connect(this, &MainWindow::ReadError, this, &MainWindow::ReadErrorSlot);
	connect(this, &MainWindow::WriteError, this, &MainWindow::WriteErrorSlot);
	connect(this, &MainWindow::ConnectError, this, &MainWindow::ConnectErrorSlot);

	this->showMaximized();
}

MainWindow::~MainWindow()
{
	delete ui;
}

void MainWindow::resizeEvent(QResizeEvent* event) 
{
	QMainWindow::resizeEvent(event);

	m_labelMainBackground->setGeometry(0, 0, this->width(), this->height());
	m_labelVideoBackground->setGeometry(0, 0, ui->VideoShow_widget->width(), ui->VideoShow_widget->height());
}

void MainWindow::on_JoinRoom_btn_clicked()
{
	int nRow = ui->RoomsList_tableview->selectionModel()->currentIndex().row();
	QModelIndex nIndex = ui->RoomsList_tableview->model()->index(nRow, ROOM_ID_COL);

	int nRoomId = ui->RoomsList_tableview->model()->data(nIndex).toInt();
	Packet pktSend(m_infUser._user_id, nullptr, sizeof(int), Pkt_Type::USER_OP_JOIN_ROOM);
	*((int*)pktSend.get_pbody()) = nRoomId;

	pktSend.encode();
	boost::asio::write(*m_sockClient, boost::asio::buffer(pktSend._pdata, pktSend.get_packet_total_size()));
}

void MainWindow::on_FlushList_btn_clicked()
{
	//清空列表
	m_stdmodelRooms->removeRows(0, m_stdmodelRooms->rowCount());

	Packet pktSend(m_infUser._user_id, nullptr, 0, Pkt_Type::USER_OP_GET_HALL_INF);
	pktSend.encode();
	boost::asio::write(*m_sockClient, boost::asio::buffer(pktSend._pdata, Packet::HEADER_LENGTH));
}

void MainWindow::on_VideoMatch_btn_clicked()
{
	this->ui->MainStackWidget_widget->setCurrentIndex(VIDEO_CHAT_PAGE_ID);
}

void MainWindow::on_SocialHall_btn_clicked()
{
	this->ui->MainStackWidget_widget->setCurrentIndex(SOCIAL_HALL_PAGE_ID);
}

void MainWindow::on_CreateRoom_btn_clicked()
{
	bool bIsOk;
	std::string strText = QInputDialog::getText(this, tr("创建房间"), tr("请输入房间名字"), QLineEdit::Normal, 0, &bIsOk).toStdString();
	if (bIsOk && !strText.empty())
	{
		Packet pktSend(m_infUser._user_id, nullptr, sizeof(VideoRoom::_room_info._room_name), Pkt_Type::USER_OP_CREATE_ROOM);
		memcpy(pktSend.get_pbody(), strText.data(), sizeof(VideoRoom::_room_info._room_name));

		pktSend.encode();
		boost::asio::write(*m_sockClient, boost::asio::buffer(pktSend._pdata, pktSend.get_packet_total_size()));

		this->ui->MainStackWidget_widget->setCurrentIndex(VIDEO_CHAT_PAGE_ID);
	}
	else if(bIsOk&& strText.empty())
	{
		QMessageBox::information(this, "创建房间", "房间名为空，请重新创建！");
	}
}

void MainWindow::on_StartMatch_btn_clicked()
{
	std::string strLabel = ui->Label_edit->text().toStdString();

	//发送匹配请求
	if (strLabel.empty())
	{
		strLabel = "你好";
	}
	Packet pktSend(m_infUser._user_id, nullptr, strLabel.size() + 1, Pkt_Type::USER_OP_MATCH);
	memcpy(pktSend.get_pbody(), strLabel.data(), strLabel.size());

	pktSend.encode();
	boost::asio::write(*m_sockClient, boost::asio::buffer(pktSend._pdata, pktSend.get_packet_total_size()));
}

void MainWindow::on_CloseVideo_btn_clicked()
{
	this->m_timerVideoCapture->stop();
	this->m_timerAudioCapture->stop();
	this->m_audioDeviceInput = nullptr;

	Packet pktSend(m_infUser._user_id, nullptr, 0, Pkt_Type::USER_OP_LEAVE_ROOM);
	pktSend.encode();
	boost::asio::write(*m_sockClient, boost::asio::buffer(pktSend._pdata, Packet::HEADER_LENGTH));

	//释放资源
	for (auto& [_, pLabel] : m_umapIdToLabel)
	{
		pLabel->hide();
		pLabel->clear();
	}
	m_umapIdToLabel.clear();
	m_stdmodelUsers->removeRows(0, m_stdmodelUsers->rowCount());
}
