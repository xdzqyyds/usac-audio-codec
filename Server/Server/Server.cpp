#include "Server.h"
#include <QThread>
#include <QTcpSocket>
#include <QHostAddress>

Server::Server(QWidget *parent)
    : QMainWindow(parent)
{
    //ui.setupUi(this);
    tcpServer = new QTcpServer(this);
    connect(tcpServer, &QTcpServer::newConnection, this, &Server::onNewConnection);

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

    startButton = new QPushButton("Start Receiving", this);
    stopButton = new QPushButton("Stop", this);
    statusLabel = new QLabel("Status: Ready", this);

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

    layout->addWidget(startButton);
    layout->addWidget(stopButton);
    layout->addWidget(statusLabel);

    setCentralWidget(centralWidget);

    // Connect buttons to slots
    connect(startButton, &QPushButton::clicked, this, &Server::onStartClicked);
    connect(stopButton, &QPushButton::clicked, this, &Server::onStopClicked);

}

Server::~Server()
{
   if (thread) {
       thread->quit();
       thread->wait();
       delete thread;
   }

   delete decoderWorker;
   delete tcpServer;
}

void Server::onStartClicked()
{
    if (!tcpServer->listen(QHostAddress::Any, 12345)) {
        statusLabel->setText("Status: Listen Failed");
        return;
    }

    decoderWorker = new AudioDecoderWorker();

    // Get values from UI
    int bitrate = bitrateEdit->text().toInt();
    int sampleRate = sampleRateEdit->text().toInt();
    int bitWidth = widthEdit->text().toInt();
    int superFrameMs = superFrameEdit->text().toInt();
    int channels = channelCheck->isChecked() ? 2 : 1;
    int sbrFlag = sbrCheck->isChecked() ? 1 : 0;
    int mpsFlag = mpsCheck->isChecked() ? 1 : 0;

    // Initialize AudioDecoderWorker
    decoderWorker->setDecodingParameters(bitrate, sampleRate, bitWidth, superFrameMs, channels, sbrFlag, mpsFlag);

    // Move worker to a new thread
    thread = new QThread();
    decoderWorker->moveToThread(thread);

    connect(thread, &QThread::started, decoderWorker, &AudioDecoderWorker::initDecoderResources);

    // Connect signals and slots
    connect(decoderWorker, &AudioDecoderWorker::info, this, &Server::onInfoReceived);
    connect(decoderWorker, &AudioDecoderWorker::error, this, &Server::onErrorReceived);
    connect(decoderWorker, &AudioDecoderWorker::finished, thread, &QThread::quit);

    // Start thread
    thread->start();

    statusLabel->setText("Status: Receiving...");
}


void Server::onStopClicked()
{
    if (decoderWorker) {
        decoderWorker->stop();
        QMetaObject::invokeMethod(decoderWorker, "freeDecoderResources", Qt::QueuedConnection);
        tcpServer->close();
        statusLabel->setText("Status: Stopped");
    }
}

void Server::onInfoReceived(QString message)
{
    // Display information messages
    statusLabel->setText("Status: " + message);
}

void Server::onErrorReceived(QString message)
{
    // Display error messages
    statusLabel->setText("Status: Error");
}

void Server::onNewConnection() {
    QTcpSocket* socket = tcpServer->nextPendingConnection();
    if (socket && decoderWorker) {
        decoderWorker->setSocket(socket);
        statusLabel->setText("Status: Client Connected");
    }
}