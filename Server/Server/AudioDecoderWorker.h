#pragma once
#include <QObject>
#include <QTcpSocket>
#include <QHostAddress>
#include <QByteArray>
#include <QDataStream>
#include <libxaacd_export.h>
#include <libsuperframe_export.h>
#include <QThread>
#include <QMutex>
#include "audio_player.h"

class AudioDecoderWorker : public QObject
{
    Q_OBJECT

public:
    AudioDecoderWorker();
    ~AudioDecoderWorker();
    void setDecodingParameters(int bitrate, int sampleRate, int bitWidth, int superFrameMs, int channels, int sbr_flag, int mps_flag);
    void setSocket(QTcpSocket* socket);

public slots:
    void initDecoderResources();
    void freeDecoderResources();
    void process(uint8_t* superframe_output);
    void stop();
    void onNewDataReceived();

signals:
    void finished();
    void error(QString message);
    void info(QString message);
    void audioDataReady(const QByteArray& data);

private:
    bool isDecoding;
    int bitrate, sampleRate, bitWidth, superFrameMs;
    int channels, sbr_flag, mps_flag;

    decode_obj* decoder;
    superframe_decoder_t* super_decoder;
    AudioPlayerThread* player;

    int handleFrameLength;

    QTcpSocket* decoderSocket;
    QString serverAddress = "127.0.0.1";
    quint16 serverPort = 12345;


    void handleReceivedData(const QByteArray& data);

};

