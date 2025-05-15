#include "Client.h"
#include <QVBoxLayout>
#include <QWidget>
#include <QMessageBox>
#include <QThread>
#include <libxaacempeg_export.h>
#include "audio_capture.h"
#include "AudioEncoderWorker.h"

Client::Client(QWidget *parent)
    : QMainWindow(parent)
{
    QWidget* centralWidget = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(centralWidget);


    bitrateEdit = new QLineEdit("64000", this);
    sampleRateEdit = new QLineEdit("44100", this);
    widthEdit = new QLineEdit("16", this);
    superFrameEdit = new QLineEdit("400", this);

    channelCheck = new QCheckBox("stereo", this);
    channelCheck->setChecked(true);
    sbrCheck = new QCheckBox("Enable SBR", this);
    mpsCheck = new QCheckBox("Enable MPS", this);

    startButton = new QPushButton("Start Encoding", this);
    stopButton = new QPushButton("Stop", this);
    statusLabel = new QLabel("Status: Ready", this);

    filePathEdit = new QLineEdit("/*Please enter the file path*/", this);
    fileModeButton = new QPushButton("Enable File Input", this);
    filePathEdit->setVisible(false);

    layout->addWidget(new QLabel("Bitrate (bps):", this));
    layout->addWidget(bitrateEdit);
    layout->addWidget(new QLabel("Sample Rate (Hz):", this));
    layout->addWidget(sampleRateEdit);
    layout->addWidget(new QLabel("PCM Bit Width:", this));
    layout->addWidget(widthEdit);
    layout->addWidget(new QLabel("Superframe Length (ms): 200 or 400", this));
    layout->addWidget(superFrameEdit);
    layout->addWidget(channelCheck);
    layout->addWidget(sbrCheck);
    layout->addWidget(mpsCheck);

    layout->addWidget(fileModeButton);
    layout->addWidget(new QLabel("File Path:", this));
    layout->addWidget(filePathEdit);

    layout->addWidget(startButton);
    layout->addWidget(stopButton);
    layout->addWidget(statusLabel);

    setCentralWidget(centralWidget);

    connect(fileModeButton, &QPushButton::clicked, this, [=]() {
        isFileMode = !isFileMode;
        filePathEdit->setVisible(isFileMode);
        fileModeButton->setText(isFileMode ? "Disable File Input" : "Enable File Input");
        });
    connect(startButton, &QPushButton::clicked, this, &Client::onStartClicked);
    connect(stopButton, &QPushButton::clicked, this, &Client::onStopClicked);
    //ui.setupUi(this);
}

Client::~Client()
{}

void Client::onStartClicked()
{
    QThread* thread = new QThread;
    //thread->start(QThread::HighPriority);
    AudioEncoderWorker* worker = new AudioEncoderWorker;

    worker->setEncodingParameters(
        bitrateEdit->text().toInt(),
        sampleRateEdit->text().toInt(),
        widthEdit->text().toInt(),
        superFrameEdit->text().toInt(),
        channelCheck->isChecked() ? 2 : 1,
        sbrCheck->isChecked() ? 1 : 0,
        mpsCheck->isChecked() ? 1 : 0
    );

    worker->moveToThread(thread);
    worker->setFileInputMode(isFileMode, filePathEdit->text());

    connect(thread, &QThread::started, worker, &AudioEncoderWorker::process);
    connect(worker, &AudioEncoderWorker::finished, thread, &QThread::quit);
    connect(worker, &AudioEncoderWorker::finished, worker, &AudioEncoderWorker::deleteLater);
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);

    connect(worker, &AudioEncoderWorker::info, statusLabel, &QLabel::setText);
    connect(worker, &AudioEncoderWorker::error, statusLabel, &QLabel::setText);

    connect(worker, &AudioEncoderWorker::finished, this, [=]() {
        statusLabel->setText("Status: Stopped");
        });

    thread->start();

    encoderThread = thread;
    encoderWorker = worker;
}

void Client::onStopClicked()
{
    if (encoderWorker) {
        encoderWorker->stop();
    }
    statusLabel->setText("Status: Stopped");
}