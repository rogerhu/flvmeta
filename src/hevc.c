#include "hevc.h"

// https://www.codeproject.com/Tips/896030/The-Structure-of-HEVC-Video
/* https://github.com/virinext/hevcesbrowser/blob/master/hevcparser/src/HevcParserImpl.cpp */
/* See FFmpeg hevc.c*/

enum {
    VPS_INDEX,
    SPS_INDEX,
    PPS_INDEX,
    SEI_PREFIX_INDEX,
    SEI_SUFFIX_INDEX,
    NB_ARRAYS
};

typedef struct __HVCCNALUnitArray {
    uint8  array_completeness;
    uint8 NAL_unit_type;
    uint16 numNalus;
    uint16 *nalUnitLength;
    uint8  **nalUnit;
} HVCCNALUnitArray;


typedef struct __HEVCDecoderConfigurationRecord {
    uint8  configurationVersion;
    uint8  general_profile_space;
    uint8  general_tier_flag;
    uint8  general_profile_idc;
    uint32 general_profile_compatibility_flags;
    uint64 general_constraint_indicator_flags;
    uint8  general_level_idc;
    uint16 min_spatial_segmentation_idc;
    uint8  parallelismType;
    uint8  chromaFormat;
    uint8  bitDepthLumaMinus8;
    uint8  bitDepthChromaMinus8;
    uint16 avgFrameRate;
    uint8  constantFrameRate;
    uint8  numTemporalLayers;
    uint8  temporalIdNested;
    uint8  lengthSizeMinusOne;
    uint8  numOfArrays;
    HVCCNALUnitArray arrays[NB_ARRAYS];
} HEVCDecoderConfigurationRecord;

static int read_hevc_decoder_configuration_record(flv_stream * f, HEVCDecoderConfigurationRecord * hdcr) {
    if (flv_read_tag_body(f, &hdcr->configurationVersion, 1) == 1
    && flv_read_tag_body(f, &hdcr->general_profile_space, 1) == 1
    && flv_read_tag_body(f, &hdcr->general_tier_flag, 1) == 1
    && flv_read_tag_body(f, &hdcr->general_profile_idc, 1) == 1
    && flv_read_tag_body(f, &hdcr->general_profile_compatibility_flags, 1) == 1
    && flv_read_tag_body(f, &hdcr->general_constraint_indicator_flags, 1) == 1
    && flv_read_tag_body(f, &hdcr->general_level_idc, 1) == 1
    && flv_read_tag_body(f, &hdcr->min_spatial_segmentation_idc, 1) == 1
    && flv_read_tag_body(f, &hdcr->parallelismType, 1) == 1
    && flv_read_tag_body(f, &hdcr->chromaFormat, 1) == 1
    && flv_read_tag_body(f, &hdcr->bitDepthLumaMinus8, 1) == 1
    && flv_read_tag_body(f, &hdcr->bitDepthChromaMinus8, 1) == 1
    && flv_read_tag_body(f, &hdcr->avgFrameRate, 1) == 1
    && flv_read_tag_body(f, &hdcr->constantFrameRate, 1) == 1
    && flv_read_tag_body(f, &hdcr->numTemporalLayers, 1) == 1
    && flv_read_tag_body(f, &hdcr->temporalIdNested, 1) == 1
    && flv_read_tag_body(f, &hdcr->lengthSizeMinusOne, 1) == 1
    && flv_read_tag_body(f, &hdcr->numOfArrays, 1) == 1
    && flv_read_tag_body(f, &hdcr->arrays, NB_ARRAYS) == NB_ARRAYS)
    {
        return FLV_OK;
    }

    return FLV_ERROR_EOF;
}

/**
    Tries to read the resolution of the current video packet.
    We assume to be at the first byte of the video data.
*/
int read_hevc_resolution(flv_video_tag * vt, flv_stream * f, uint32 body_length, uint32 * width, uint32 * height) {
    int packet_type;
    uint24 composition_time;
    HEVCDecoderConfigurationRecord hdcr;
    byte nal_header;

    uint16 sps_size;
    byte * sps_buffer;

    /* make sure we have enough bytes to read in the current tag */
    if (body_length < sizeof(byte) + sizeof(uint24) + sizeof(HEVCDecoderConfigurationRecord)) {
        return FLV_OK;
    }

    if(!flv_video_tag_is_ext_header(vt)) {
        return FLV_ERROR_EOF;
    }

    packet_type = flv_video_tag_ext_packet_type(vt);

    if (packet_type != FLV_VIDEO_TAG_PACKET_TYPE_SEQUENCE_START) {
        return FLV_OK;
    }

    /* we need to read an AVCDecoderConfigurationRecord */
    if (read_hevc_decoder_configuration_record(f, &hdcr) == FLV_ERROR_EOF) {
        return FLV_ERROR_EOF;
    }

    // /* number of SequenceParameterSets */
    // if ((hdcr.numOfSequenceParameterSets & 0x1F) == 0) {
    //     /* no SPS, return */
    //     return FLV_OK;
    // }

    /** read the first SequenceParameterSet found */
    /* SPS size */
    if (flv_read_tag_body(f, &nal_header, sizeof(byte)) < sizeof(byte)) {
        return FLV_ERROR_EOF;
    }
    printf("%d\n", nal_header);

    // sps_size = swap_uint16(sps_size);

    // /* SPS size should not be zero or more than the remaining bytes in the tag body */
    // if (sps_size == 0
    // || sps_size > body_length - (sizeof(HEVCDecoderConfigurationRecord) + 1 + sizeof(uint24) + sizeof(uint16))) {
    //     return FLV_ERROR_EOF;
    // }

    // /* read the SPS entirely */
    // sps_buffer = (byte *) malloc((size_t)sps_size);
    // if (sps_buffer == NULL) {
    //     return FLV_ERROR_MEMORY;
    // }
    // if (flv_read_tag_body(f, sps_buffer, (size_t)sps_size) < (size_t)sps_size) {
    //     free(sps_buffer);
    //     return FLV_ERROR_EOF;
    // }

    // /* parse SPS to determine video resolution */
    // parse_sps(sps_buffer, (size_t)sps_size, width, height);

    //free(sps_buffer);
    return FLV_OK;
}
