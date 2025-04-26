#pragma once
#include <QQueue>
#include <QByteArray>
#include <QThread>
#include <QAudioOutput>
#include <QAudioDeviceInfo>
#include <QIODevice>
#include <QMutex>
#include <QSemaphore>
#include <QWaitCondition>
#include <QElapsedTimer>


class AudioPlayerThread : public QThread
{
    Q_OBJECT

public:
    explicit AudioPlayerThread(QObject* parent = nullptr);
    ~AudioPlayerThread();

    void initialize(int sampleRate, int channelCount, int sampleSize);
    void playAudio(const char* data, qint64 len);
    void stopPlayer();

protected:
    void run() override;

signals:
    void errorOccurred(const QString& message);

private:
    QAudioFormat m_format;
    QAudioOutput* m_audioOutput = nullptr;
    QIODevice* m_audioIO = nullptr;
    QMutex m_mutex;
    bool m_initialized = false;
    QElapsedTimer timer;

    QQueue<QByteArray> m_bufferQueue;
    QSemaphore m_dataAvailable;
    QMutex m_bufferMutex;

    void writeAudioData(const char* data, qint64 len);
};
