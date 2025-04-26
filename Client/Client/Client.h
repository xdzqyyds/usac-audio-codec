#pragma once

#include <QtWidgets/QMainWindow>
//#include "ui_Client.h"
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QCheckBox>
#include "AudioEncoderWorker.h"

class Client : public QMainWindow
{
    Q_OBJECT

public:
    Client(QWidget *parent = nullptr);
    ~Client();

private slots:
    void onStartClicked();
    void onStopClicked();

private:
    //Ui::ClientClass ui;
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

    QThread* encoderThread = nullptr;
    AudioEncoderWorker* encoderWorker = nullptr;

    QPushButton* fileModeButton;
    QLineEdit* filePathEdit;
    bool isFileMode = false; 
};
