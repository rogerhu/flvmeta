#include "hevc.h"
#include "bitstream.h"

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
//    HVCCNALUnitArray arrays[NB_ARRAYS];
} HEVCDecoderConfigurationRecord;

// https://github.com/axiomatic-systems/Bento4/blob/master/Source/C%2B%2B/Core/Ap4HvccAtom.cpp

typedef struct __HVCCAtom {
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
//    HVCCNALUnitArray arrays[NB_ARRAYS];
} HVCCAtom;

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
    && flv_read_tag_body(f, &hdcr->numOfArrays, 1) == 1)
   // && flv_read_tag_body(f, &hdcr->arrays, NB_ARRAYS) == NB_ARRAYS)
    {
        return FLV_OK;
    }

    return FLV_ERROR_EOF;
}

/**
    Tries to read the resolution of the current video packet.
    We assume to be at the first byte of the video data.
*/

#define HEVC_NAL_SPS 33
#define AP4_ATOM_HEADER_SIZE 8*8

static void hvcc_parse_sps(byte * sps, size_t sps_size, uint32 * width, uint32 * height) {
    bit_buffer bb;
    unsigned int sps_max_sub_layers_minus1;
    uint8_t temporalIdNested;
    uint8_t chromaFormat;
    uint16_t pic_width_in_luma_samples;
    uint16_t pic_length_in_luma_samples;

    bb.start = sps;
    bb.size = sps_size;
    bb.current = sps;
    bb.read_bits = 0;

    skip_bits(&bb, 4); // sps_video_parameter_set_id

    if (exp_golomb_ue(&bb) == 3) {
            skip_bits(&bb, 1);
    }
    /* bit depth luma minus8 */
    pic_width_in_luma_samples = exp_golomb_ue(&bb);
    /* bit depth chroma minus8 */
    pic_length_in_luma_samples = exp_golomb_ue(&bb);

    *width = (pic_width_in_luma_samples + 1) * 16;
    *height = (pic_length_in_luma_samples + 1) * 16;

    // if (!get_bits(&bb, 3, &sps_max_sub_layers_minus1)) {
    //     return;
    // }

    // if (!get_bits(&bb, 1, &temporalIdNested)) {
    //     return;
    // }
    // chromaFormat = get_ue_golomb_long(&bb);
    // if (chromaFormat == 3) {
    //     skip_bits(&bb, 1); // separate_colour_plane_flag
    // }
    // pic_width_in_luma_samples = get_ue_golomb_long(&bb);
    // pic_length_in_luma_samples = get_ue_golomb_long(&bb);

}

int read_hevc_resolution(flv_video_tag * vt, flv_stream * f, uint32 body_length, uint32 * width, uint32 * height) {
    int packet_type;
    uint24 composition_time;
    HEVCDecoderConfigurationRecord hdcr;
    byte hvccHeader[23];
    HVCCAtom hvccAtom;
    uint8 nal_header;

    uint16 sps_size;
    byte * sps_buffer;

    /* make sure we have enough bytes to read in the current tag */
    if (body_length < sizeof(byte) + sizeof(uint24) + sizeof(HEVCDecoderConfigurationRecord)) {
        return FLV_OK;
    }

    if(!flv_video_tag_is_ext_header(vt)) {
        return FLV_ERROR_EOF;
    }

    // https://github.com/ossrs/srs/blob/ad7ddde3181f9ad9c4c9225158ac6ec22944c429/trunk/src/kernel/srs_kernel_codec.cpp#L192
    // https://github.com/ossrs/srs/pull/3495/files#diff-b381246f7b6c2af916111db040dc6a1d52a5288f4c0d2b783f5a90b13ac0d5b8
    packet_type = flv_video_tag_ext_packet_type(vt);

    /* Based on Extended RTMP spec */
    if (packet_type != FLV_AAC_PACKET_TYPE_SEQUENCE_HEADER) {
      return FLV_OK;
    }

    // if (packet_type != FLV_VIDEO_TAG_PACKET_TYPE_CODED_FRAMES &&
    // packet_type != FLV_VIDEO_TAG_PACKET_TYPE_CODED_FRAMES_X) {
    //     return FLV_OK;
    // }
    // /* read the composition time */
    // if (flv_read_tag_body(f, &composition_time, sizeof(uint24)) < sizeof(uint24)) {
    //     return FLV_ERROR_EOF;
    // }

    /* Composition time is only specified for the coded frames packet type */
    if (packet_type == FLV_VIDEO_TAG_PACKET_TYPE_CODED_FRAMES) {
        /* read the composition time */
        if (flv_read_tag_body(f, &composition_time, sizeof(uint24)) < sizeof(uint24)) {
            return FLV_ERROR_EOF;
        }
    }

    /* we need to read an AVCDecoderConfigurationRecord */
    // if (read_hevc_decoder_configuration_record(f, &hdcr) == FLV_ERROR_EOF) {
    //     return FLV_ERROR_EOF;
    // }

    // /* number of SequenceParameterSets */
    // if ((hdcr.numOfSequenceParameterSets & 0x1F) == 0) {
    //     /* no SPS, return */
    //     return FLV_OK;
    // }

    /** read the first SequenceParameterSet found */
    /* SPS size */
    /* Read HVCC atom*/

//    hvccHeader = (byte *) malloc((size_t)AP4_ATOM_HEADER_SIZE);
    // byte header;
    // if (flv_read_tag_body(f, &header, sizeof(byte)) < sizeof(byte)) {
    //     return FLV_ERROR_EOF;
    // }


    if (flv_read_tag_body(f, &hvccHeader, sizeof(hvccHeader)) < sizeof(hvccHeader)) {
        return FLV_ERROR_EOF;
    }

    hvccAtom.configurationVersion = hvccHeader[0];
    hvccAtom.general_profile_idc = hvccHeader[1] & 0x1F;
    hvccAtom.lengthSizeMinusOne = (hvccHeader[21] & 0x03) + 1;
    hvccAtom.numOfArrays = hvccHeader[22];

    int i, j;
    HVCCNALUnitArray unitArray;
    uint16_be numNalus;

    for (i = 0; i < hvccAtom.numOfArrays; i++) {
        if (flv_read_tag_body(f, &unitArray.NAL_unit_type, 1) < 1) {
            return FLV_ERROR_EOF;
        }
        if (flv_read_tag_body(f, &numNalus, 2) < 2) {
            return FLV_ERROR_EOF;
        }

        int unitType = unitArray.NAL_unit_type & 0x3F;
        int nalCount = swap_uint16(numNalus);
        int nalLengthSize = 2; /* hvcc always contains 2 */
        unitArray.nalUnitLength = malloc(nalCount * sizeof(uint16));
        byte *buf;

        // https://github.com/FFmpeg/FFmpeg/blob/master/libavcodec/hevc_parse.c#L80
        for (j = 0; j < nalCount; j++) {
            if (flv_read_tag_body(f, &unitArray.nalUnitLength[j], 2) < 2) {
                return FLV_ERROR_EOF;
            }
            unitArray.nalUnitLength[j] = swap_uint16(unitArray.nalUnitLength[j]);

            // Skip over the first NALU
            buf = malloc(unitArray.nalUnitLength[j]);
            if (flv_read_tag_body(f, buf, unitArray.nalUnitLength[j]) < unitArray.nalUnitLength[j]) {
                return FLV_ERROR_EOF;
            }

            if (unitType == 33) {
                /* parse SPS to determine video resolution */
                hvcc_parse_sps(buf, unitArray.nalUnitLength[j], width, height);
                free(buf);
                return FLV_OK;
            }
            free(buf);

        }
    }
//    hvccAtom.constantFrameRate = (hvccHeader[21] >> 6) & 0x03;
    // if (flv_read_tag_body(f, &nal_header, sizeof(uint8)) < sizeof(uint8)) {
    //     return FLV_ERROR_EOF;
    // }
    // /* 0111 1110 */
    // int header_type = (nal_header & 0x7E) >> 1;
    // printf("%d\n", nal_header);

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

