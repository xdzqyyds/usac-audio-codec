#include "audio_player.h"
#include <QDebug>
#include <QElapsedTimer>
#include <cstring>

AudioPlayerThread::AudioPlayerThread(QObject* parent)
    : QThread(parent)
{
}

AudioPlayerThread::~AudioPlayerThread()
{
    stopPlayer();
}

void AudioPlayerThread::initialize(int sampleRate, int channelCount, int sampleSize)
{
    QMutexLocker locker(&m_mutex);

    m_format.setSampleRate(sampleRate);
    m_format.setChannelCount(channelCount);
    m_format.setSampleSize(sampleSize);
    m_format.setCodec("audio/pcm");
    m_format.setByteOrder(QAudioFormat::LittleEndian);
    m_format.setSampleType(QAudioFormat::SignedInt);

    QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice());
    if (!info.isFormatSupported(m_format)) {
        m_format = info.nearestFormat(m_format);
        emit errorOccurred("Using nearest supported audio format");
    }

    m_audioOutput = new QAudioOutput(m_format, this);
    m_audioOutput->setBufferSize(81920);
    m_audioIO = m_audioOutput->start();
    QByteArray silence(81920, 0);
    m_audioIO->write(silence);


    m_initialized = true;
}

void AudioPlayerThread::playAudio(const char* data, qint64 len)
{
    QMutexLocker lock(&m_bufferMutex);
    m_bufferQueue.enqueue(QByteArray(data, len));
    m_dataAvailable.release();
}

void AudioPlayerThread::stopPlayer()
{
    QMutexLocker locker(&m_mutex);
    if (m_audioOutput) {
        m_audioOutput->stop();
        delete m_audioOutput;
        m_audioOutput = nullptr;
    }
    m_initialized = false;
    requestInterruption();
    m_dataAvailable.release();
}

void AudioPlayerThread::run() {
    while (!isInterruptionRequested()) {
        m_dataAvailable.acquire();
        if (isInterruptionRequested()) break;

        QByteArray data;
        {
            QMutexLocker lock(&m_bufferMutex);
            if (m_bufferQueue.isEmpty()) continue;
            data = m_bufferQueue.dequeue();
        }

        writeAudioData(data.constData(), data.size());
    }
}

void AudioPlayerThread::writeAudioData(const char* data, qint64 len) {
    QMutexLocker locker(&m_mutex);
    if (!m_initialized) return;

    qint64 written = 0;
    while (written < len) {
        qint64 free = m_audioOutput->bytesFree();
        if (free == 0) {
            m_audioOutput->resume();
            free = m_audioOutput->bytesFree();
        }

        qint64 chunk = qMin(len - written, free);
        qint64 ret = m_audioIO->write(data + written, chunk);
        if (ret > 0) written += ret;
        else QThread::usleep(1000);
    }
}