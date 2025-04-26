#pragma once
#include <QtWidgets/QMainWindow>
//#include "ui_Server.h"
#include <QLineEdit>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include "AudioDecoderWorker.h"
#include <QHostAddress>
#include <QByteArray>
#include <QDataStream>
#include <QTcpServer>

class Server : public QMainWindow
{
    Q_OBJECT

public:
    Server(QWidget *parent = nullptr);
    ~Server();

private slots:
    void onStartClicked();
    void onStopClicked();
    void onInfoReceived(QString message);
    void onErrorReceived(QString message);
    void onNewConnection(); // 处理新连接的槽函数


private:
    //Ui::ServerClass ui;
    QPushButton* startButton;
    QPushButton* stopButton;

    QLineEdit* bitrateEdit;
    QLineEdit* sampleRateEdit;
    QLineEdit* widthEdit;
    QLineEdit* superFrameEdit;

    QCheckBox* channelCheck;
    QCheckBox* sbrCheck;
    QCheckBox* mpsCheck;

    QLabel* statusLabel;

    AudioDecoderWorker* decoderWorker;
    QThread* thread;
    QTcpServer* tcpServer;


};
