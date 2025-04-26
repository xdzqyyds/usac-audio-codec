#ifndef AUDIO_CAPTURE_H
#define AUDIO_CAPTURE_H

#include <cstdint>
#include <QAudioInput>
#include <QIODevice>
#include <QAudioFormat>
#include "ring_buffer.h"

class AudioCapture
{
public:
    AudioCapture();
    ~AudioCapture();

    bool init_audio_capture(int sampleRate, int channels, int sampleSize);
    int capture_audio_frame(uint8_t** buffer);
    void close_audio_capture();
    int read_frame_from_wav(FILE* wav_file, uint8_t** buffer, int frame_size);
    void start_capture_loop(RingBuffer* buffer);
    void stop_capture_loop();                    

private:
    QAudioInput audio_input;
    QIODevice* audio_device;
    QAudioFormat format;
    bool is_capturing;

    std::atomic<bool> capture_running; 
    std::thread capture_thread;
};

#endif // AUDIO_CAPTURE_H
