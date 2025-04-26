#pragma once
#include <QObject>
#include <QTcpSocket>
#include <QHostAddress>
#include <QByteArray>
#include <QDataStream>
#include <QElapsedTimer>
#include <QAtomicInteger>
#include "audio_capture.h"

class AudioEncoderWorker : public QObject
{
    Q_OBJECT

public:
    AudioEncoderWorker();
    ~AudioEncoderWorker();
    void setEncodingParameters(int bitrate, int sampleRate, int bitWidth, int superFrameMs, int channels, int sbr_flag, int mps_flag);
    void setFileInputMode(bool enabled, const QString& path);

public slots:
    void process();
    void stop();

signals:
    void finished();
    void error(QString message);
    void info(QString message);

private:
    QAtomicInteger<bool> isEncoding;
    int bitrate, sampleRate, bitWidth, superFrameMs;
    int channels, sbr_flag, mps_flag;

    QTcpSocket* tcpSocket = nullptr;
    QString serverAddress = "127.0.0.1";
    quint16 serverPort = 12345;

    FILE* g_pf_out;
    FILE* g_pf_inp;

    QElapsedTimer timer;
    qint64 elapsedTime;
    qint64 lastelapsedTime;
    int time_flag;

    bool useFileInput = false;
    QString inputFilePath;
    AudioCapture capture;

    uint8_t* buffer;
    int input_length;
    qint64 frame_duration;

    void sendSuperframeToDecoder(uint8_t* superframe, int length);

};

