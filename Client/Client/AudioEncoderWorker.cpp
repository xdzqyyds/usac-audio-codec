#include "AudioEncoderWorker.h"
#include <libxaacempeg_export.h>
#include <libsuperframe_export.h>
#include <libxaacd_export.h>
#include "audio_capture.h"
#include <QDebug>
#include <QThread>


AudioEncoderWorker::AudioEncoderWorker()
    : isEncoding(false),
    bitrate(64000),
    sampleRate(44100),
    bitWidth(16),
    superFrameMs(400),
    channels(2),
    sbr_flag(0),
    mps_flag(0)

{
    tcpSocket = new QTcpSocket(this);
}

AudioEncoderWorker::~AudioEncoderWorker() {
    if (tcpSocket) {
        tcpSocket->disconnectFromHost();
        delete tcpSocket;
    }
}

void AudioEncoderWorker::setEncodingParameters(int bitrate_, int sampleRate_, int bitWidth_, int superFrameMs_,
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

void AudioEncoderWorker::stop() {
    isEncoding.storeRelease(false);
}

void AudioEncoderWorker::sendSuperframeToDecoder(uint8_t* superframe, int length) {

    QByteArray byteArray(reinterpret_cast<char*>(superframe), length);
    tcpSocket->write(byteArray);
    if (!tcpSocket->waitForBytesWritten()) {
        emit error("Failed to send superframe data");
        return;
    }

    qDebug() << "Superframe sent to decoder!";
}


void AudioEncoderWorker::process() {

    if (tcpSocket->state() != QAbstractSocket::ConnectedState) {
        tcpSocket->connectToHost(serverAddress, serverPort);
        if (!tcpSocket->waitForConnected(3000)) {
            emit error("Failed to connect to decoder");
            return;
        }
    }
    int handleFrameLength = 256 * channels * bitWidth;
    int superFrameLength = bitrate * superFrameMs / 8000;

    encode_para enc_para = {};
    enc_para.bitrate = bitrate;
    enc_para.ui_samp_freq = sampleRate;
    enc_para.ui_pcm_wd_sz = bitWidth;
    enc_para.Super_frame_mode = superFrameMs;

    enc_para.ui_num_chan = channels;
    enc_para.sbr_flag = sbr_flag;
    enc_para.mps_flag = mps_flag;
    enc_para.input_file_flag = 0;
    enc_para.output_file_flag = 1;


    FILE* encoded_wav_file = NULL;
    unsigned char* encoded_data = NULL;
    int encoded_size = 0;
    const char* encoded_wav_path = "encoded.mp4";
    if (enc_para.output_file_flag) {
        encoded_wav_file = fopen(encoded_wav_path, "wb");
    }

    elapsedTime = 0;
    time_flag = 0;


    encode_obj* encoder = xheaace_create(&enc_para);
    if (!encoder) {
        emit info("xheaace_create error");
        return;
    }

    superframe_encoder_t* super_ctx = create_superframe_encoder(superFrameLength);
    uint8_t* superframe_output = (uint8_t*)malloc(superFrameLength);
    int frame_offset = 0;
    isEncoding = true;

    //g_pf_out = fopen("capture_pcm.wav", "wb");
    //uint8_t wav_header[44] = { 0 };
    //fwrite(wav_header, 1, 44, g_pf_out);
    //int total_bytes = 0;

    const size_t buffer_size = handleFrameLength * 50;
    RingBuffer ring_buffer(buffer_size);

    FILE* g_pf_inp = nullptr;
    if (useFileInput) {
        g_pf_inp = fopen(inputFilePath.toUtf8().constData(), "rb");
        if (!g_pf_inp) {
            emit error("Failed to open input file");
            return;
        }
        fseek(g_pf_inp, 44, SEEK_SET);
    }
    else {
        if (!capture.init_audio_capture(sampleRate, channels, bitWidth)) {
            emit info("Capture initialization failed");
            return;
        }
        capture.start_capture_loop(&ring_buffer);
    }



    while (isEncoding.loadAcquire()) {
        ///*Collecting audio*/
        if (useFileInput) {
            buffer = nullptr;
            input_length = capture.read_frame_from_wav(g_pf_inp, &buffer, handleFrameLength);
            //QThread::msleep(30);
            if (input_length <= 0) {
                emit info("File read completed");
                break;
            }
            /*Writing to audio files*/
            //fwrite(buffer, 1, input_length, g_pf_out);
            //total_bytes = total_bytes + input_length;
            //long current_pos = ftell(g_pf_out);
            //if (!fseek(g_pf_out, 0, SEEK_SET)) {
            //    write_header(g_pf_out, total_bytes, enc_para.ui_samp_freq, enc_para.ui_num_chan,
            //        enc_para.ui_pcm_wd_sz, 3);
            //}
        }
        else {
            buffer = new uint8_t[handleFrameLength];
            input_length = ring_buffer.read(buffer, handleFrameLength);
            if (input_length < handleFrameLength) {
                continue;
            }
        }
        if (input_length > 0) {

            if (xheaace_encode_frame(encoder, buffer, &encoded_data, &encoded_size) == 1) {
                if (encode_superframe(super_ctx, encoded_data, encoded_size, superframe_output) == 1) {
                    elapsedTime = timer.elapsed();
                    timer.start();
                    if (time_flag == 0 || elapsedTime > 400) {
                        elapsedTime = 0;
                        lastelapsedTime = 0;
                        time_flag = 1;
                    }
                    else {
                        elapsedTime = elapsedTime - lastelapsedTime;
                        //QThread::msleep(superFrameMs - elapsedTime);
                        qDebug() << "Delay time:" << (superFrameMs - elapsedTime);
                        lastelapsedTime = superFrameMs - elapsedTime;
                    }
                    qDebug() << "Superframe encoding successful!";
                    emit info("Superframe encoding successful!");
                    sendSuperframeToDecoder(superframe_output, superFrameLength);

                }
            }
            free(buffer);
        }

    }

    if (useFileInput && g_pf_inp) {
        fclose(g_pf_inp);
    }
    else {
        capture.stop_capture_loop();
        capture.close_audio_capture();
    }
    free(encoded_data);
    free(superframe_output);
    xheaace_delete(encoder, encoded_wav_path);
    destroy_superframe_encoder(super_ctx);
    if (enc_para.output_file_flag) {
        fclose(encoded_wav_file);
    }
    emit info("Encoding stopped");
    emit finished();
}


void AudioEncoderWorker::setFileInputMode(bool enabled, const QString& path) {
    useFileInput = enabled;
    inputFilePath = path;
}