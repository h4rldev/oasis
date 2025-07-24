#ifndef METADATA_H
#define METADATA_H

#include "libavutil/dict.h"
#include <libavcodec/packet.h>
#include <libavutil/samplefmt.h>

typedef struct {
  int sample_rate;
  int bitrate;
  int channels;
  char *codec_name;
  char *title;
  char **artists;
  char *album;
  AVPacket *cover_art;
} audio_metadata_t;

audio_metadata_t *get_audio_metadata(AVDictionary *metadata);

#endif
