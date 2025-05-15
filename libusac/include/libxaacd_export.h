#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef BUILDING_LIBXAACDEC 
#define LIBXAACDEC_API __declspec(dllexport)
#else 
#define LIBXAACDEC_API __declspec(dllimport)
#endif


    LIBXAACDEC_API typedef struct {
        FILE* pf_out;
        char output_path[512];
        signed int bitrate;
        signed int samp_freq;
        signed int pcm_wd_sz;
        signed int num_chan;
        signed int sbr_flag;
        signed int mps_flag;
        signed int Super_frame_mode;
    } decode_para;

    LIBXAACDEC_API typedef struct {
        void* pv_ia_process_api_obj;
        decode_para* decode_para;
        signed int ui_inp_size;
        signed int i_buff_size;
        signed int i_bytes_consumed;
        signed char* pb_inp_buf;
        signed char* pb_out_buf;
    } decode_obj;




    LIBXAACDEC_API decode_obj* xheaacd_create(decode_para* decode_para);

    LIBXAACDEC_API int xheaacd_decode_frame(decode_obj* pv_decode_obj, void* audioframe, int ixheaacd_i_bytes_to_read);

    LIBXAACDEC_API signed int write_header(FILE* fp, signed int pcmbytes, signed int freq, signed int channels, signed int bits, signed int i_channel_mask);


#ifdef __cplusplus
}
#endif
