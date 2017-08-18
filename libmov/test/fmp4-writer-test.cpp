#include "fmp4-writer.h"
#include "mov-format.h"
#include "mpeg4-aac.h"
#include "flv-proto.h"
#include "flv-reader.h"
#include "flv-parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

static uint8_t s_buffer[2 * 1024 * 1024];
static int s_width, s_height;

static int onFLV(void* param, int codec, const void* data, size_t bytes, uint32_t pts, uint32_t dts, int flags)
{
	fmp4_writer_t* mov = (fmp4_writer_t*)param;
	static int s_aac_track = -1;
	static int s_avc_track = -1;

	switch (codec)
	{
	case FLV_AUDIO_AAC:
		return fmp4_writer_write(mov, s_aac_track, data, bytes, pts, dts, 1 == flags ? MOV_AV_FLAG_KEYFREAME : 0);

	case FLV_AUDIO_MP3:
		assert(0);
		break;

	case FLV_VIDEO_H264:
		return fmp4_writer_write(mov, s_avc_track, data, bytes, pts, dts, flags);

	case FLV_VIDEO_AVCC:
		if (-1 == s_avc_track)
		{
			s_avc_track = fmp4_writer_add_video(mov, MOV_OBJECT_H264, s_width, s_height, data, bytes);
		}
		break;

	case FLV_AUDIO_ASC:
		if (-1 == s_aac_track)
		{
			struct mpeg4_aac_t aac;
			mpeg4_aac_audio_specific_config_load((const uint8_t*)data, bytes, &aac);
			int rate = mpeg4_aac_audio_frequency_to((enum mpeg4_aac_frequency)aac.sampling_frequency_index);
			s_aac_track = fmp4_writer_add_audio(mov, MOV_OBJECT_AAC, aac.channel_configuration, 16, rate, data, bytes);
		}
		break;

	default:
		// nothing to do
		assert(0);
	}

	printf("\n");
	return 0;
}

void fmp4_writer_test(int w, int h, const char* inflv, const char* outmp4)
{
	int r, type;
	uint32_t timestamp;
	void* flv = flv_reader_create(inflv);
	fmp4_writer_t* mov = fmp4_writer_create(outmp4, 0);
	flv_parser_t* parser = flv_parser_create(onFLV, mov);

	s_width = w;
	s_height = h;
	while ((r = flv_reader_read(flv, &type, &timestamp, s_buffer, sizeof(s_buffer))) > 0)
	{
		r = flv_parser_input(parser, type, s_buffer, r, timestamp);
		assert(r >= 0);
	}

	fmp4_writer_destroy(mov);
	flv_reader_destroy(flv);
	flv_parser_destroy(parser);
}
