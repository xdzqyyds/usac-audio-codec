#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#ifdef BUILDING_LIBXAACENC
#define LIBXAACENC_API __declspec(dllexport)
#else
#define LIBXAACENC_API __declspec(dllimport)
#endif

#define IA_NO_ERROR 0x00000000

    typedef struct encode_obj_ encode_obj;

    LIBXAACENC_API typedef struct {
        signed int bitrate;            // 编码码率(推荐16000)
        signed int ui_pcm_wd_sz;       // pcm格式(pcm_16le)
        signed int ui_samp_freq;       // 采样率
        signed int ui_num_chan;        // 通道数量
        signed int sbr_flag;           // 是否使用SBR(0-不使用, 1-使用)
        signed int mps_flag;           // 是否使用MPS(0-不使用, 1-使用)
        signed int Super_frame_mode;   // 超级帧格式（400ms或200ms）
    } encode_para;

    LIBXAACENC_API encode_obj* xheaace_create(encode_para* encode_para);

    LIBXAACENC_API signed int xheaace_encode_frame(encode_obj* pv_encode_obj, void* superframe_output, const unsigned char* raw_pcm_frame);

    LIBXAACENC_API signed int xheaace_delete(encode_obj* pv_encode_obj);

    LIBXAACENC_API signed int xheaace_get_frame_count(encode_obj* pv_encode_obj);

    LIBXAACENC_API signed int xheaace_bytes_read(encode_obj* pv_encode_obj);

    LIBXAACENC_API signed int xheaace_get_expected_frame(encode_obj* pv_encode_obj);

    LIBXAACENC_API signed int xheaace_get_super_frame_length(encode_obj* pv_encode_obj);


#ifdef __cplusplus
}
#endif
