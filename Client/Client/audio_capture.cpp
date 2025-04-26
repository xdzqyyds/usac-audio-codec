#include "audio_capture.h"
#include <QDebug>

AudioCapture::AudioCapture()
    : audio_input(), audio_device(nullptr), is_capturing(false)
{
}

AudioCapture::~AudioCapture()
{
    close_audio_capture();
}

bool AudioCapture::init_audio_capture(int sampleRate, int channels, int sampleSize)
{
    if (is_capturing) {
        qWarning() << "Audio capture is already initialized";
        return false;
    }

    QAudioFormat format;
    format.setSampleRate(sampleRate);
    format.setChannelCount(channels);
    format.setSampleSize(sampleSize);
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::SignedInt);

    audio_input.~QAudioInput();
    new (&audio_input) QAudioInput(QAudioDeviceInfo::defaultInputDevice(), format);

    audio_device = audio_input.start();
    if (!audio_device) {
        qCritical() << "Failed to start audio capture";
        return false;
    }

    is_capturing = true;
    return true;
}

int AudioCapture::capture_audio_frame(uint8_t** buffer)
{
    if (!is_capturing || !audio_device) {
        *buffer = nullptr;
        return 0;
    }

    qint64 bytes_available = audio_input.bytesReady();
    if (bytes_available <= 0) {
        *buffer = nullptr;
        return 0;
    }

    *buffer = new uint8_t[bytes_available];
    qint64 bytes_read = audio_device->read(reinterpret_cast<char*>(*buffer), bytes_available);

    if (bytes_read <= 0) {
        delete[] * buffer;
        *buffer = nullptr;
        return 0;
    }

    return static_cast<int>(bytes_read);
}

void AudioCapture::close_audio_capture()
{
    if (is_capturing) {
        audio_input.stop();
    }
    audio_device = nullptr;
    is_capturing = false;
}

int AudioCapture::read_frame_from_wav(FILE* wav_file, uint8_t** buffer, int frame_size) {
    if (!wav_file || !buffer) return -1;

    *buffer = (uint8_t*)malloc(frame_size);
    if (!*buffer) return -1;

    size_t bytes_read = fread(*buffer, 1, frame_size, wav_file);
    if (bytes_read != frame_size) {
        free(*buffer);
        *buffer = NULL;
        return -1;
    }

    return (int)bytes_read;
}


void AudioCapture::start_capture_loop(RingBuffer* ring_buffer) {
    if (capture_running) return;
    capture_running = true;

    capture_thread = std::thread([this, ring_buffer]() {
        constexpr size_t chunk_size = 4096;
        uint8_t temp_buffer[chunk_size];

        while (capture_running) {
            qint64 bytes_available = audio_input.bytesReady();
            if (bytes_available <= 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }

            qint64 bytes_read = audio_device->read(
                reinterpret_cast<char*>(temp_buffer),
                std::min(bytes_available, static_cast<qint64>(chunk_size))
            );

            if (bytes_read > 0) {
                ring_buffer->write(temp_buffer, bytes_read);
            }
        }
        });
}

void AudioCapture::stop_capture_loop() {
    capture_running = false;
    if (capture_thread.joinable()) {
        capture_thread.join();
    }
    close_audio_capture();
}