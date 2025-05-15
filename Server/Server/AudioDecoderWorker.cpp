#include "AudioDecoderWorker.h"
#include <QDebug>
#include <cstring>
#include <libxaacd_export.h>
#include <libsuperframe_export.h>
#include "audio_player.h"
#include <QElapsedTimer>
#include <QCoreApplication>


AudioDecoderWorker::AudioDecoderWorker()
    : isDecoding(false),
    decoder(nullptr),
    super_decoder(nullptr),
    decoderSocket(nullptr),
    bitrate(64000),
    sampleRate(44100),
    bitWidth(16),
    superFrameMs(400),
    channels(2),
    sbr_flag(0),
    mps_flag(0)
{
}

AudioDecoderWorker::~AudioDecoderWorker() {
    freeDecoderResources();
    if (decoderSocket) {
        decoderSocket->disconnectFromHost();
        decoderSocket->deleteLater();
    }
}

void AudioDecoderWorker::initDecoderResources() {

    handleFrameLength = 256 * channels * bitWidth;
    int superFrameLength = bitrate * superFrameMs / 8000;

    decode_para dec_para = {};
    dec_para.bitrate = bitrate;
    dec_para.samp_freq = sampleRate;
    dec_para.pcm_wd_sz = bitWidth;
    dec_para.Super_frame_mode = superFrameMs;

    dec_para.num_chan = channels;
    dec_para.sbr_flag = sbr_flag;
    dec_para.mps_flag = mps_flag;
    strcpy(dec_para.output_path, "decoded_output.wav");
    dec_para.pf_out = fopen(dec_para.output_path, "wb");

    decoder = (decode_obj*)xheaacd_create(&dec_para);
    if (!decoder) {
        emit info("xheaacd_create error");
        return;
    }

    super_decoder = create_superframe_decoder(superFrameLength);

    player = new AudioPlayerThread();
    player->initialize(sampleRate, channels, bitWidth);
    connect(this, &AudioDecoderWorker::audioDataReady,
        player, [=](const QByteArray& pcmData) {
            player->playAudio(pcmData.constData(), pcmData.size());
        });
    player->start(QThread::HighPriority);

    isDecoding = true;

}

void AudioDecoderWorker::freeDecoderResources() {

    if (player) {
        player->stopPlayer();
        player->quit();
        player->wait();
        delete player;
        player = nullptr;
    }

    if (super_decoder) {
        free(super_decoder);
        super_decoder = nullptr;
    }
}

void AudioDecoderWorker::setDecodingParameters(int bitrate_, int sampleRate_, int bitWidth_, int superFrameMs_,
    int channels_, int sbr_flag_, int mps_flag_)
{
    bitrate = bitrate_;
    sampleRate = sampleRate_;
    bitWidth = bitWidth_;
    superFrameMs = superFrameMs_;
    channels = channels_;
    sbr_flag = sbr_flag_;
    mps_flag = mps_flag_;
}

void AudioDecoderWorker::stop() {
    isDecoding = false;
}

void AudioDecoderWorker::onNewDataReceived() {
    if (decoderSocket && decoderSocket->bytesAvailable() > 0) {
        QByteArray data = decoderSocket->readAll();
        handleReceivedData(data);
    }
}

void AudioDecoderWorker::handleReceivedData(const QByteArray& data) {
    uint8_t* superframe_output = const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(data.data()));

    process(superframe_output);
}

void AudioDecoderWorker::process(uint8_t* superframe_output) {
         if (!isDecoding) return;

        uint32_t total_size = 0;
        uint32_t frame_sizes[17] = { 0 };
        uint32_t offset = 0;
        uint32_t i = 0;
        /*decode one superframe into multiple audio frames*/

        uint8_t* audio_frame = (uint8_t*)decode_superframe(superframe_output, super_decoder, &total_size, frame_sizes);



        while (offset < total_size && i < 17) {
            if (frame_sizes[i] > 0) {
                if (xheaacd_decode_frame(decoder, audio_frame + offset, frame_sizes[i]) != 0) {
                    emit error("Frame decoding failed");
                }
                
                if (decoder->pb_out_buf && handleFrameLength > 0) {
                    QByteArray pcmData(reinterpret_cast<const char*>(decoder->pb_out_buf),
                        handleFrameLength);
                    emit audioDataReady(pcmData);
                }
            }
            offset += frame_sizes[i];
            i++;
            QCoreApplication::processEvents();
        }

        free(audio_frame);
        emit info("A Superframe decoding completed");
 }


void AudioDecoderWorker::setSocket(QTcpSocket* socket) {
    if (decoderSocket) {
        decoderSocket->deleteLater();
    }
    decoderSocket = socket;
    connect(decoderSocket, &QTcpSocket::readyRead, this, &AudioDecoderWorker::onNewDataReceived);
}