/*
 * Adobe Action Message Format Parser
 * Copyright (c) 2013 Cory McCarthy
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/**
 * @file
 * @brief Adobe Action Message Format Parser
 * @author Cory McCarthy
 * @see http://download.macromedia.com/f4v/video_file_format_spec_v10_1.pdf
 * @see http://www.adobe.com/content/dam/Adobe/en/devnet/amf/pdf/amf-file-format-spec.pdf
 */

#include "amfmetadata.h"
#include "avio_internal.h"
#include "flv.h"
#include "libavutil/avstring.h"
#include "libavutil/intfloat.h"

static int amf_metadata_read_string_value(AVIOContext *in, char *str, int str_size)
{
    uint8_t type;

    type = avio_r8(in);
    if(type != 0x02) {
        av_log(NULL, AV_LOG_ERROR, "amfmetadata Expected type 2, type = %d \n", type);
        return -1;
    }

    return ff_amf_get_string(in, str, str_size);
}

static void amf_metadata_assign_property_number(AMFMetadata *metadata,
    const char *name, double value)
{
    if(!av_strcasecmp("width", name)) {
        metadata->width = (int)value;
    }
    else
    if(!av_strcasecmp("height", name)) {
        metadata->height = (int)value;
    }
    else
    if(!av_strcasecmp("framerate", name)) {
        metadata->frame_rate = (int)value;
    }
    else
    if(!av_strcasecmp("videodatarate", name)) {
        metadata->video_data_rate = (int)value;
    }
    else
    if(!av_strcasecmp("audiosamplerate", name)) {
        metadata->audio_sample_rate = (int)value;
    }
    else
    if(!av_strcasecmp("audiochannels", name)) {
        metadata->nb_audio_channels = (int)value;
    }
    else
    if(!av_strcasecmp("stereo", name)) {
        metadata->nb_audio_channels = ((int)value) ? 2 : 1;
    }
    else
    if(!av_strcasecmp("audiodatarate", name)) {
        metadata->audio_data_rate = (int)value;
    }
    else
    if(!av_strcasecmp("audiocodecid", name)) {
        if((int)value == 10)
            metadata->audio_codec_id = CODEC_ID_AAC;
    }
    else
    if(!av_strcasecmp("videocodecid", name)) {
        if((int)value == 7)
            metadata->video_codec_id = CODEC_ID_H264;
    }
}

static void amf_metadata_assign_property_string(AMFMetadata *metadata,
    const char *name, const char *value)
{
    if(!av_strcasecmp("audiocodecid", name)) {
        if(!av_strcasecmp("mp4a", value))
            metadata->audio_codec_id = CODEC_ID_AAC;
    }
    else
    if(!av_strcasecmp("videocodecid", name)) {
        if(!av_strcasecmp("avc1", value))
            metadata->video_codec_id = CODEC_ID_H264;
    }
}

static int amf_metadata_parse(AVIOContext *in, AMFMetadata *metadata)
{
    uint8_t type;
    char name[INT16_MAX];
    char value_str[INT16_MAX];
    double value_number;
    int ret;

    if((ret = amf_metadata_read_string_value(in, name, sizeof(name))) < 0) {
        av_log(NULL, AV_LOG_ERROR, "amfmetadata Failed to read onMetadata string, ret: %d \n", ret);
        return ret;
    }

    if(av_strcasecmp(name, "onMetaData")) {
        av_log(NULL, AV_LOG_ERROR, "amfmetadata Expected onMetadata, str = %s \n", name);
        return -1;
    }

    type = avio_r8(in);
    if(type == 0x08) {
        avio_skip(in, 4);
    }
    else
    if(type != 0x03) {
        av_log(NULL, AV_LOG_ERROR, "amfmetadata Expected type 3, type = %x \n", type);
        return -1;
    }

    while(!url_feof(in)) {
        if((ret = ff_amf_get_string(in, name, sizeof(name))) < 0)
            return ret;

        type = avio_r8(in);
        if(type == 0x00) {
            value_number = av_int2double(avio_rb64(in));
            amf_metadata_assign_property_number(metadata, name, value_number);
        }
        else if(type == 0x01) {
            value_number = avio_r8(in);
            amf_metadata_assign_property_number(metadata, name, value_number);
        }
        else if(type == 0x02) {
            if((ret = ff_amf_get_string(in, value_str, sizeof(value_str))) < 0)
                return ret;
            amf_metadata_assign_property_string(metadata, name, value_str);
        }
        else if(type == 0x09) {
            break;
        }
    }

    return 0;
}

int ff_parse_amf_metadata(uint8_t *buffer, int buffer_size, AMFMetadata *metadata)
{
    AVIOContext *in;
    int ret;

    if(!buffer)
        return 0;
    if(buffer_size <= 0)
        return 0;

    in = avio_alloc_context(buffer, buffer_size,
        0, NULL, NULL, NULL, NULL);
    if(!in)
        return AVERROR(ENOMEM);

    ret = amf_metadata_parse(in, metadata);
    av_free(in);

    return ret;
}
