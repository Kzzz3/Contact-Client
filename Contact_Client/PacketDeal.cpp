#include "MainWindow.h"
#include "ui_MainWindow.h"

void MainWindow::PacketDealing()
{
    auto funcDealing = [this](std::shared_ptr<Packet> pktRecieve)
    {
        switch (pktRecieve->_pkt_inf._type)
        {
        case Pkt_Type::USER_OP_MATCH:
            emit MatchResult(pktRecieve);
            break;

        case Pkt_Type::USER_OP_JOIN_ROOM:
            emit JoinRoomResult(pktRecieve);
            break;

        case Pkt_Type::USER_OP_LEAVE_ROOM:
            emit LeaveRoomResult(pktRecieve);
            break;

        case Pkt_Type::USER_OP_GET_HALL_INF:
            emit RoomsInfoResult(pktRecieve);
            break;

        case Pkt_Type::USER_OP_JOIN_ROOM_INF:
            emit JoinRoomInfo(pktRecieve);
			break;

        case Pkt_Type::USER_OP_LEAVE_ROOM_INF:
		    emit LeaveRoomInfo(pktRecieve);
		break;

        case Pkt_Type::USER_DATA_IMG:
        {
            cv::Mat matImg;
            std::vector<uchar> vecImgCompressed(pktRecieve->_pkt_inf._body_length);
            std::memcpy(vecImgCompressed.data(), pktRecieve->get_pbody(), pktRecieve->_pkt_inf._body_length);
            cv::imdecode(vecImgCompressed, true, &matImg);

            cv::cvtColor(matImg, matImg, cv::COLOR_BGR2RGBA);
            QImage qimgTemp(matImg.data, matImg.cols, matImg.rows, QImage::Format_RGBA8888);
            std::shared_ptr<QPixmap> pixmapImage(new QPixmap(QPixmap::fromImage(qimgTemp)));

            emit RecieveImage(pktRecieve->_user_inf._user_id,pixmapImage);
            break;
        }

        case Pkt_Type::USER_DATA_AUDIO:
        {
            emit RecieveAudio(pktRecieve);
        }

        default:
            break;
        }
    };
    while (true)
    {
        boost::system::error_code errCode;
        std::shared_ptr<Packet> pktRecieve(new Packet());
        int nRet=boost::asio::read(*m_sockClient, boost::asio::buffer((char*)&(*pktRecieve), Packet::HEADER_LENGTH), errCode);
        if (errCode||nRet!= Packet::HEADER_LENGTH)
        {
			emit ReadError(errCode);
			return;
		}

        pktRecieve->set_body_length(pktRecieve->_pkt_inf._body_length);
        nRet=boost::asio::read(*m_sockClient, boost::asio::buffer(pktRecieve->get_pbody(), pktRecieve->_pkt_inf._body_length), errCode);
        if (errCode||nRet!= pktRecieve->_pkt_inf._body_length)
        {
			emit ReadError(errCode);
			return;
		}

        funcDealing(pktRecieve);
    }
}

void MainWindow::ReadErrorSlot(boost::system::error_code errCode)
{
    QMessageBox::information(this, "Error", "Read error!");
}

void MainWindow::WriteErrorSlot(boost::system::error_code errCode)
{
    QMessageBox::information(this, "Error", "Write error!");
}

void MainWindow::ConnectErrorSlot(boost::system::error_code errCode)
{
    QMessageBox::information(this, "Error", "Connect error!");
}

void MainWindow::VideoCapture()
{
    cv::Mat matFrame;
    m_vpCamera >> matFrame;
    if (matFrame.empty())
        return;

    std::vector<uchar> vecBuffer;
    cv::imencode(".jpg", matFrame, vecBuffer, { cv::IMWRITE_JPEG_QUALITY, GRAPH_QUALITY });

    Packet pkt(m_infUser._user_id, nullptr, vecBuffer.size(), Pkt_Type::USER_DATA_IMG);
    memcpy(pkt.get_pbody(), vecBuffer.data(), vecBuffer.size());
    pkt.encode();

    boost::system::error_code errCode;
    int nRet = boost::asio::write(*m_sockClient, boost::asio::buffer(pkt._pdata, pkt.get_packet_total_size()), errCode);
    if (errCode || nRet != pkt.get_packet_total_size())
    {
        emit WriteError(errCode);
    }

    //本机显示
    if (m_umapIdToLabel.find(m_infUser._user_id) != m_umapIdToLabel.end())
    {
        int nLabelWidth = m_umapIdToLabel[m_infUser._user_id]->width();
        int nLabelHeight = m_umapIdToLabel[m_infUser._user_id]->height();

        cv::cvtColor(matFrame, matFrame, cv::COLOR_BGR2RGBA);
        QImage qimgTemp(matFrame.data, matFrame.cols, matFrame.rows, QImage::Format_RGBA8888);
        m_umapIdToLabel[m_infUser._user_id]->setPixmap(QPixmap::fromImage(qimgTemp).scaled(nLabelWidth, nLabelHeight, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
}

void MainWindow::AudioCapture()
{
    if (m_audioDeviceInput == nullptr)
        return;

    QByteArray audioData = m_audioDeviceInput->readAll();
    if (audioData.isEmpty())
		return;

    boost::system::error_code errCode;
    Packet pktSend(m_infUser._user_id, nullptr, audioData.size(), Pkt_Type::USER_DATA_AUDIO);
    memcpy(pktSend.get_pbody(), audioData.data(), audioData.size());
    pktSend.encode();

    //m_audioDeviceOutput->write(pktSend.get_pbody(), pktSend._pkt_inf._body_length);

	int nRet = boost::asio::write(*m_sockClient, boost::asio::buffer(pktSend._pdata, pktSend.get_packet_total_size()), errCode);
    if (errCode || nRet != pktSend.get_packet_total_size())
    {
		emit WriteError(errCode);
	}
}

void MainWindow::MatchResultSlot(std::shared_ptr<Packet> pktRecieve)
{
    int nPerWidth = ui->VideoShow_widget->width() / VideoRoom::DOUBLED_ROOM_CAPACITY;
    int nPerHeight = ui->VideoShow_widget->height();
    for (int i = 0; i < VideoRoom::DOUBLED_ROOM_CAPACITY; ++i)
    {
        m_vecDoubleUsersLabel[i]->setGeometry(i * nPerWidth, 0, nPerWidth, nPerHeight);
    }

    int nUsersNum = pktRecieve->_pkt_inf._body_length / sizeof(User_Inf::_user_id);
    for (int i = 0; i < nUsersNum; ++i)
    {
        char strUserId[sizeof(User_Inf::_user_id)] = { 0 };
        memcpy(strUserId, pktRecieve->get_pbody() + i * sizeof(User_Inf::_user_id), sizeof(User_Inf::_user_id));

        m_umapIdToLabel[strUserId] = m_vecDoubleUsersLabel[i];
        m_umapIdToLabel[strUserId]->show();

        QStandardItem* pItem = new QStandardItem(strUserId);
        pItem->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        m_stdmodelUsers->appendRow(pItem);
    }

    m_audioDeviceInput = m_audioSource->start();
    m_timerAudioCapture->start(AUDIO_CAPTURE_INTERVAL);
    m_timerVideoCapture->start(VIDEO_CAPTURE_INTERVAL);
}

void MainWindow::JoinRoomInfoSlot(std::shared_ptr<Packet> pktRecieve)
{
    char strUserId[sizeof(User_Inf::_user_id)] = { 0 };
    memcpy(strUserId, pktRecieve->get_pbody(), sizeof(User_Inf::_user_id));

    for (auto pUsedLabel : m_vecMutiUsersLabel)
    {
        bool bUsed = false;
        for (auto& [_, pLabel] : m_umapIdToLabel)
        {
            if (pLabel == pUsedLabel)
            {
                bUsed = true;
                break;
            }
        }
        if (!bUsed)
        {
			m_umapIdToLabel[strUserId] = pUsedLabel;
			m_umapIdToLabel[strUserId]->show();

            QStandardItem* pItem = new QStandardItem(strUserId);
            pItem->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
            m_stdmodelUsers->appendRow(pItem);
			break;
		}
    }
}

void MainWindow::LeaveRoomInfoSlot(std::shared_ptr<Packet> pktRecieve)
{
    char strUserId[sizeof(User_Inf::_user_id)] = { 0 };
    memcpy(strUserId, pktRecieve->get_pbody(), sizeof(User_Inf::_user_id));

    if (m_umapIdToLabel.find(strUserId) != m_umapIdToLabel.end())
    {
        m_umapIdToLabel[strUserId]->clear();
		m_umapIdToLabel[strUserId]->hide();
		m_umapIdToLabel.erase(strUserId);

        for (int i = 0; i < m_stdmodelUsers->rowCount(); ++i)
        {
            if (m_stdmodelUsers->data(m_stdmodelUsers->index(i,0)).toString().toStdString() == strUserId)
            {
                m_stdmodelUsers->removeRow(i);
				break;
			}
		}
	}
}

void MainWindow::RecieveAudioSlot(std::shared_ptr<Packet> pktRecieve)
{
    m_audioDeviceOutput->write(pktRecieve->get_pbody(), pktRecieve->_pkt_inf._body_length);
}

void MainWindow::RecieveImageSlot(std::string strUserId,std::shared_ptr<QPixmap> pixmapImage)
{
    if (m_umapIdToLabel.find(strUserId) != m_umapIdToLabel.end())
    {
        int nLabelWidth = m_umapIdToLabel[strUserId]->width();
        int nLabelHeight = m_umapIdToLabel[strUserId]->height();
        m_umapIdToLabel[strUserId]->setPixmap((*pixmapImage).scaled(nLabelWidth, nLabelHeight, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
}

void MainWindow::JoinRoomResultSlot(std::shared_ptr<Packet> pktRecieve)
{
    int nPerWidth = ui->VideoShow_widget->width() / VideoRoom::PER_ROW_USERS;
    int nPerHeight = ui->VideoShow_widget->height() / (VideoRoom::MAX_ROOM_CAPACITY / VideoRoom::PER_ROW_USERS);
    for (int i = 0; i < VideoRoom::MAX_ROOM_CAPACITY; ++i)
    {
        int nRow = i / VideoRoom::PER_ROW_USERS;
        int nCol = i % VideoRoom::PER_ROW_USERS;
        m_vecMutiUsersLabel[i]->setGeometry(nCol * nPerWidth, nRow * nPerHeight, nPerWidth, nPerHeight);
    }

    int nUsersNum = pktRecieve->_pkt_inf._body_length / sizeof(User_Inf::_user_id);
    for (int i = 0; i < nUsersNum; ++i)
    {
        char strUserId[sizeof(User_Inf::_user_id)] = { 0 };
        memcpy(strUserId, pktRecieve->get_pbody() + i * sizeof(User_Inf::_user_id), sizeof(User_Inf::_user_id));

        m_umapIdToLabel[strUserId] = m_vecMutiUsersLabel[i];
        m_umapIdToLabel[strUserId]->show();

        QStandardItem* pItem = new QStandardItem(strUserId);
        pItem->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        m_stdmodelUsers->appendRow(pItem);
    }

    m_audioDeviceInput = m_audioSource->start();
    m_timerAudioCapture->start(AUDIO_CAPTURE_INTERVAL);
    m_timerVideoCapture->start(VIDEO_CAPTURE_INTERVAL);

    this->ui->MainStackWidget_widget->setCurrentIndex(VIDEO_CHAT_PAGE_ID);
}

void MainWindow::LeaveRoomResultSlot(std::shared_ptr<Packet> pktRecieve)
{
    this->m_timerVideoCapture->stop();

    //释放资源
    for (auto& [_, pLabel] : m_umapIdToLabel)
    {
        pLabel->hide();
        pLabel->clear();
    }
    m_umapIdToLabel.clear();
    m_stdmodelUsers->removeRows(0, m_stdmodelUsers->rowCount());
}

void MainWindow::RoomsInfoResultSlot(std::shared_ptr<Packet> pktRecieve)
{
    char* pLast = pktRecieve->_pdata + pktRecieve->get_packet_total_size();
    for (VideoRoomInfo* pCurInfo = (VideoRoomInfo*)pktRecieve->get_pbody(); (char*)pCurInfo < pLast; pCurInfo++)
    {
        int nRow = m_stdmodelRooms->rowCount();

        QStandardItem* pItem = new QStandardItem(QString("%1").arg(pCurInfo->_room_id));
        pItem->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        m_stdmodelRooms->setItem(nRow, ROOM_ID_COL, pItem);

        pItem = new QStandardItem(QString("%1").arg(pCurInfo->_room_name));
        pItem->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        m_stdmodelRooms->setItem(nRow, ROOM_NAME_COL, pItem);

        pItem = new QStandardItem(QString("%1").arg(pCurInfo->_room_owner_id));
        pItem->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        m_stdmodelRooms->setItem(nRow, ROOM_OWNER_ID_COL, pItem);

        pItem = new QStandardItem(QString("%1").arg(pCurInfo->_room_capacity));
        pItem->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        m_stdmodelRooms->setItem(nRow, ROOM_CAPACITY_COL, pItem);

        pItem = new QStandardItem(QString("%1").arg(pCurInfo->_room_size));
        pItem->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        m_stdmodelRooms->setItem(nRow, ROOM_SIZE_COL, pItem);
    }
}

