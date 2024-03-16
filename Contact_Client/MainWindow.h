#pragma once

#include <mutex>
#include <future>
#include <QTimer>
#include <QFile>
#include <qevent.h>
#include <qlabel.h>
#include <QMainWindow>
#include <QMessageBox>
#include <QAudioSink>
#include <QAudioSource>
#include <QInputdialog>
#include <QMediadevices>
#include <qstandarditemmodel.h>
#include <QStandardItemModel>
#include <opencv2/opencv.hpp>

#include "VideoRoom.h"
#include "ui_MainWindow.h"
#include "PacketDefinition.hpp"

using namespace boost::asio;
using namespace boost::asio::ip;

constexpr int GRAPH_QUALITY = 10;
constexpr int VIDEO_CAPTURE_INTERVAL = 1000 / 10;
constexpr int AUDIO_CAPTURE_INTERVAL = 1000 / 10;

constexpr unsigned int VIDEO_CHAT_PAGE_ID = 0;
constexpr unsigned int SOCIAL_HALL_PAGE_ID = 1;

constexpr unsigned int ROOM_ID_COL = 0;
constexpr unsigned int ROOM_NAME_COL = 1;
constexpr unsigned int ROOM_OWNER_ID_COL = 2;
constexpr unsigned int ROOM_CAPACITY_COL = 3;
constexpr unsigned int ROOM_SIZE_COL = 4;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindowClass; };
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow()=delete;
    MainWindow(io_context* ioContext, tcp::socket* sockClient, User_Inf infUser);
    ~MainWindow();

public:
    //UI Data
    Ui::MainWindow* ui;
    QLabel* m_labelMainBackground;
    QLabel* m_labelVideoBackground;
    QStandardItemModel* m_stdmodelUsers;
    QStandardItemModel* m_stdmodelRooms;
    std::vector<QLabel*> m_vecMutiUsersLabel;
    std::vector<QLabel*> m_vecDoubleUsersLabel;

    //user info
    User_Inf m_infUser;
    std::unordered_map<std::string, QLabel*> m_umapIdToLabel;

    //Asio net
    io_context* m_ioContext;
    tcp::socket* m_sockClient;
    std::thread m_thdRecievePacket;

    //Contact video 
    QTimer* m_timerVideoCapture;
    cv::VideoCapture m_vpCamera;

    //Contact audio
    QTimer* m_timerAudioCapture;
    QAudioSink* m_audioSink;
    QAudioSource* m_audioSource;
    QIODevice* m_audioDeviceInput;
    QIODevice* m_audioDeviceOutput;

public:
    //Client Functions
    void PacketDealing();

    //ÊÂ¼þÖØÐ´
    void resizeEvent(QResizeEvent* event);

signals:
    //Client Signals
    void MatchResult(std::shared_ptr<Packet> pktRecieve);
    void JoinRoomInfo(std::shared_ptr<Packet> pktRecieve);
    void LeaveRoomInfo(std::shared_ptr<Packet> pktRecieve);
    void JoinRoomResult(std::shared_ptr<Packet> pktRecieve);
    void LeaveRoomResult(std::shared_ptr<Packet> pktRecieve);
    void RoomsInfoResult(std::shared_ptr<Packet> pktRecieve);

    void RecieveAudio(std::shared_ptr<Packet> pktRecieve);
    void RecieveImage(std::string strUserId, std::shared_ptr<QPixmap> pixmapImage);

    //Eorror Signals
    void ReadError(boost::system::error_code errCode);
    void WriteError(boost::system::error_code errCode);
    void ConnectError(boost::system::error_code errCode);

public slots:
    //UI Slots
    void on_JoinRoom_btn_clicked();
    void on_FlushList_btn_clicked();
    void on_VideoMatch_btn_clicked();
    void on_SocialHall_btn_clicked();
    void on_CreateRoom_btn_clicked();
    void on_StartMatch_btn_clicked();
    void on_CloseVideo_btn_clicked();

    //Client Slot
    void VideoCapture();
    void AudioCapture();
    void MatchResultSlot(std::shared_ptr<Packet> pktRecieve);
    void JoinRoomInfoSlot(std::shared_ptr<Packet> pktRecieve);
    void LeaveRoomInfoSlot(std::shared_ptr<Packet> pktRecieve);
    void JoinRoomResultSlot(std::shared_ptr<Packet> pktRecieve);
    void LeaveRoomResultSlot(std::shared_ptr<Packet> pktRecieve);
    void RoomsInfoResultSlot(std::shared_ptr<Packet> pktRecieve);

    void RecieveAudioSlot(std::shared_ptr<Packet> pktRecieve);
    void RecieveImageSlot(std::string strUserId, std::shared_ptr<QPixmap> pixmapImage);

    //Error Slots
    void ReadErrorSlot(boost::system::error_code errCode);
    void WriteErrorSlot(boost::system::error_code errCode);
    void ConnectErrorSlot(boost::system::error_code errCode);
};
