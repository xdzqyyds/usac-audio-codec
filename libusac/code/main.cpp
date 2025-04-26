#include <iostream>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <chrono> 
#include "libxaace_export.h"
#include "libxaacd_export.h"
#include "audio_cap_play.h"


#define AUDIO_BITRATE        16000
#define AUDIO_SAMPLE_RATE    48000
#define AUDIO_CHANNELS       2
#define AUDIO_PCM_WIDTH      16
#define AUDIO_SBR_FLAG       0
#define AUDIO_MPS_FLAG       0
#define AUDIO_SUPERFRAME     400

#define Handle_frame_length  256*AUDIO_CHANNELS*AUDIO_PCM_WIDTH 
#define Super_frame_length   AUDIO_BITRATE*AUDIO_SUPERFRAME/8000


int main() {
    system("chcp 65001 > nul");

    /*encoder initialize(Includes super frame encoder initialization)*/
    encode_para enc_para;
    memset(&enc_para, 0, sizeof(encode_para));
    enc_para.bitrate = AUDIO_BITRATE;
    enc_para.ui_pcm_wd_sz = AUDIO_PCM_WIDTH;
    enc_para.ui_samp_freq = AUDIO_SAMPLE_RATE;
    enc_para.ui_num_chan = AUDIO_CHANNELS;
    enc_para.sbr_flag = AUDIO_SBR_FLAG;
    enc_para.mps_flag = AUDIO_MPS_FLAG;
    enc_para.Super_frame_mode = AUDIO_SUPERFRAME;
    encode_obj* encoder = xheaace_create(&enc_para);
    if (!encoder) {
        printf("xheaace_create error\n");
        return 1;
    }


    /*decoder initialize*/
    decode_para dec_para;
    memset(&dec_para, 0, sizeof(decode_para));
    dec_para.bitrate = AUDIO_BITRATE;
    dec_para.pcm_wd_sz = AUDIO_PCM_WIDTH;
    dec_para.samp_freq = AUDIO_SAMPLE_RATE;
    dec_para.num_chan = AUDIO_CHANNELS;
    dec_para.sbr_flag = AUDIO_SBR_FLAG;
    dec_para.mps_flag = AUDIO_MPS_FLAG;
    dec_para.Super_frame_mode = AUDIO_SUPERFRAME;
    decode_obj* decoder = (decode_obj*)xheaacd_create(&dec_para);
    if (!decoder) {
        printf("xheaacd_create error\n");
        return 1;
    }
    superframe_decoder_t* super_decoder = create_superframe_decoder(Super_frame_length);


    /*<ffmpeg -list_devices true -f dshow -i dummy> List the available audio devices */
    /*<ffmpeg -list_options true -f dshow -i audio="麦克风 (Realtek(R) Audio)"> Check the formats supported by the current device*/
    //list_audio_devices();
    /*There are areas that need improvement. It should automatically identify microphone devices*/
    //const char* dev_name = u8"audio=麦克风 (Realtek(R) Audio)";

    std::string dev_name = find_default_microphone();
    if (dev_name.empty()) {
        std::cerr << "Failed to automatically recognize the microphone device" << std::endl;
        return -1;
    }

    std::cout << "Microphone detected: " << dev_name << std::endl;

    if (init_audio_capture(dev_name.c_str(), AUDIO_SAMPLE_RATE, AUDIO_CHANNELS, AUDIO_PCM_WIDTH) != 0) {
        std::cerr << "Init failed" << std::endl;
        return -1;
    }

    uint8_t* superframe_output = (uint8_t*)malloc(Super_frame_length);
    uint8_t* frame_buffer = (uint8_t*)malloc(Handle_frame_length);
    int frame_offset = 0;

    /*Cyclic codec processing*/
    while (1) {
        uint8_t* buffer = nullptr;
        int input_length = capture_audio_frame(&buffer);
        if (input_length > 0) {
            printf("Captured %d bytes\n", input_length);
        }
        int input_offset = 0;

        /*Because the audio frames collected at one time are too long, they need to be segmented and encoded*/
        while (input_offset < input_length) {
            int remaining_frame_space = Handle_frame_length - frame_offset;
            int copy_len = (input_length - input_offset) < remaining_frame_space ? (input_length - input_offset) : remaining_frame_space;
            memcpy(frame_buffer + frame_offset, buffer + input_offset, copy_len);
            frame_offset += copy_len;
            input_offset += copy_len;
            if (frame_offset == Handle_frame_length) {
                /*complete the encoding of an audio frame and try to combine it into a super frame*/
                int result = xheaace_encode_frame(encoder, superframe_output, frame_buffer);
                int super_frame_length = xheaace_get_super_frame_length(encoder);
                frame_offset = 0;

                /*complete the decoding of a superframe*/
                if (result == 1) {
                    uint32_t total_size = 0;
                    uint32_t frame_sizes[17] = { 0 };
                    uint32_t offset = 0;
                    uint32_t i = 0;
                    /*decode one superframe into multiple audio frames*/
                    uint8_t* audio_frame = (uint8_t*)decode_superframe(superframe_output, super_decoder, &total_size, frame_sizes);

                    uint8_t* pcm_buffer = (uint8_t*)malloc(Handle_frame_length * 15);
                    uint32_t pcm_offset = 0;
                    while (offset <= total_size && i < 17) {
                        if (frame_sizes[i] > 0) {
                            /*decoded audio frame*/
                            xheaacd_decode_frame(decoder, audio_frame + offset, frame_sizes[i], pcm_buffer + pcm_offset);
                            pcm_offset += Handle_frame_length;
                        }
                        offset += frame_sizes[i];
                        i++;
                    }
                    /*audio player*/
                    win_play_pcm(pcm_buffer, pcm_offset, dec_para.samp_freq, dec_para.num_chan);
                    free(pcm_buffer);
                }
            }
        }
        free(buffer);
        /*At present, the target frame length is set as the condition for terminating the loop*/
        if (xheaace_get_frame_count(encoder) == 4000) break;
    }

    close_audio_capture();
    signed int err = xheaace_delete(encoder);
    free(superframe_output);
    free(frame_buffer);
    if (err != IA_NO_ERROR) {
        printf("xheaace_delete error 0x%x\n", err);
    }

    return 0;
}

