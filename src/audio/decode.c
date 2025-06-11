#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/channel_layout.h>
#include <libavutil/frame.h>
#include <libavutil/opt.h>
#include <libswresample/swresample.h>
#include <oasis/utils.h>

#include <bsd/string.h>

#include <oasis/audio/decode.h>
#include <oasis/utils.h>

static char *get_audio_codec_name(enum AVCodecID codec_id) {
  static char codec[100];
  const char *codec_name = avcodec_get_name(codec_id);

  strlcpy(codec, codec_name, sizeof(codec));

  return codec;
}

static ma_format ffmpeg_to_miniaudio_format(enum AVSampleFormat fmt) {
  switch (fmt) {
  case AV_SAMPLE_FMT_U8:
    return ma_format_u8;
  case AV_SAMPLE_FMT_S16:
    return ma_format_s16;
  case AV_SAMPLE_FMT_S32:
    return ma_format_s32;
  case AV_SAMPLE_FMT_FLT:
    return ma_format_f32;
  case AV_SAMPLE_FMT_DBL:
    return ma_format_f32; // miniaudio doesn't support f64 directly
  case AV_SAMPLE_FMT_S64:
    return ma_format_s32; // miniaudio doesn't support s64 directly

  // For planar formats, we'll convert to interleaved in our processing
  case AV_SAMPLE_FMT_U8P:
    return ma_format_u8;
  case AV_SAMPLE_FMT_S16P:
    return ma_format_s16;
  case AV_SAMPLE_FMT_S32P:
    return ma_format_s32;
  case AV_SAMPLE_FMT_FLTP:
    return ma_format_f32;
  case AV_SAMPLE_FMT_DBLP:
    return ma_format_f32;
  case AV_SAMPLE_FMT_S64P:
    return ma_format_s32;

  default:
    return ma_format_unknown;
  }
}

oasis_result_t decode_to_pcm(const char *filename, AVFrame ***frames,
                             int *frame_count) {
  if (!filename || !frames) {
    oasis_log(NULL, LOG_LEVEL_ERROR,
              "Invalid arguments, either filename or frames is NULL");
    return OASIS_ERROR;
  }
  *frames = NULL;
  *frame_count = 0;

  int initial_capacity = 64;

  AVFormatContext *fmt_ctx = NULL;
  AVCodecContext *codec_ctx = NULL;
  AVPacket *pkt = NULL;
  AVFrame *frame = NULL;
  int ret = 0, audio_stream_index = -1;
  oasis_result_t final_result = OASIS_SUCCESS;

  if (avformat_open_input(&fmt_ctx, filename, NULL, NULL) != 0) {
    oasis_log(NULL, LOG_LEVEL_ERROR, "Failed to open input file %s", filename);
    return OASIS_ERROR;
  }

  if (avformat_find_stream_info(fmt_ctx, NULL) < 0) {
    avformat_close_input(&fmt_ctx);
    oasis_log(NULL, LOG_LEVEL_ERROR, "Failed to find stream info");
    return OASIS_ERROR;
  }

  for (unsigned i = 0; i < fmt_ctx->nb_streams; i++) {
    if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
      audio_stream_index = i;
      break;
    }
  }

  if (audio_stream_index == -1) {
    oasis_log(NULL, LOG_LEVEL_ERROR, "No audio stream found");
    avformat_close_input(&fmt_ctx);
    return OASIS_ERROR;
  }

  AVCodecParameters *codecpar = fmt_ctx->streams[audio_stream_index]->codecpar;
  const AVCodec *codec = avcodec_find_decoder(codecpar->codec_id);
  if (!codec) {
    oasis_log(NULL, LOG_LEVEL_ERROR, "Failed to find decoder for codec %s",
              get_audio_codec_name(codecpar->codec_id));
    avformat_close_input(&fmt_ctx);
    return OASIS_ERROR;
  }

  codec_ctx = avcodec_alloc_context3(codec);
  if (!codec_ctx) {
    oasis_log(NULL, LOG_LEVEL_ERROR, "Failed to allocate codec context");
    avcodec_free_context(&codec_ctx);
    avformat_close_input(&fmt_ctx);
    return OASIS_ERROR;
  }

  if (avcodec_parameters_to_context(codec_ctx, codecpar) < 0) {
    oasis_log(NULL, LOG_LEVEL_ERROR,
              "Failed to copy codec parameters to decoder context");
    avcodec_free_context(&codec_ctx);
    avformat_close_input(&fmt_ctx);
    return OASIS_ERROR;
  }

  if (avcodec_open2(codec_ctx, codec, NULL) < 0) {
    oasis_log(NULL, LOG_LEVEL_ERROR, "Failed to open codec");
    avcodec_free_context(&codec_ctx);
    avformat_close_input(&fmt_ctx);
    return OASIS_ERROR;
  }

  pkt = av_packet_alloc();
  if (!pkt) {
    oasis_log(NULL, LOG_LEVEL_ERROR, "Failed to allocate packet");
    avcodec_free_context(&codec_ctx);
    avformat_close_input(&fmt_ctx);
    return OASIS_ERROR;
  }

  frame = av_frame_alloc();
  if (!frame) {
    oasis_log(NULL, LOG_LEVEL_ERROR, "Failed to allocate frame");
    av_packet_free(&pkt);
    avcodec_free_context(&codec_ctx);
    avformat_close_input(&fmt_ctx);
    return OASIS_ERROR;
  }

  AVFrame **frame_arr = malloc(initial_capacity * sizeof(AVFrame *));
  if (!frame_arr) {
    oasis_log(NULL, LOG_LEVEL_ERROR, "Failed to allocate frame array");
    av_packet_free(&pkt);
    av_frame_free(&frame);
    avcodec_free_context(&codec_ctx);
    avformat_close_input(&fmt_ctx);
    return OASIS_ERROR;
  }

  int count = 0;
  while (av_read_frame(fmt_ctx, pkt) >= 0) {
    if (pkt->stream_index == audio_stream_index) {

      ret = avcodec_send_packet(codec_ctx, pkt);
      if (ret < 0)
        break;

      while (ret >= 0) {
        ret = avcodec_receive_frame(codec_ctx, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
          break;
        else if (ret < 0) {
          oasis_log(NULL, LOG_LEVEL_ERROR, "Error receiving frame: %s",
                    av_err2str(ret));
          goto clean_frames;
        }

        if (count >= initial_capacity) {
          initial_capacity *= 2;
          AVFrame **new_frame_arr =
            realloc(frame_arr, initial_capacity * sizeof(AVFrame *));
          if (!new_frame_arr) {
            oasis_log(NULL, LOG_LEVEL_ERROR,
                      "Failed to reallocate frame array");
            goto clean_frames;
          }
          frame_arr = new_frame_arr;
        }

        frame_arr[count] = av_frame_clone(frame);
        if (!frame_arr[count]) {
          oasis_log(NULL, LOG_LEVEL_ERROR, "Failed to clone frame");
          goto clean_frames;
        };
        count++;
      }
    }
    av_packet_unref(pkt);
  }

  avcodec_send_packet(codec_ctx, NULL);
  while (1) {
    ret = avcodec_receive_frame(codec_ctx, frame);
    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
      break;
    else if (ret < 0) {
      oasis_log(NULL, LOG_LEVEL_INFO, "Is it here?\n");
      oasis_log(NULL, LOG_LEVEL_ERROR, "Error receiving frame: %s\n",
                av_err2str(ret));
      goto clean_frames;
    }
    if (count >= initial_capacity) {
      initial_capacity *= 2;
      AVFrame **new_frame_arr =
        realloc(frame_arr, initial_capacity * sizeof(AVFrame *));
      if (!new_frame_arr) {
        oasis_log(NULL, LOG_LEVEL_ERROR, "Failed to reallocate frame array");
        goto clean_frames;
      }
      frame_arr = new_frame_arr;
    }
    frame_arr[count] = av_frame_clone(frame);
    if (!frame_arr[count]) {
      oasis_log(NULL, LOG_LEVEL_ERROR, "Failed to clone frame");
      goto clean_frames;
    };
    count++;
  }

  *frames = frame_arr;
  *frame_count = count;
  goto done;

clean_frames:
  if (frame_arr) {
    for (int i = 0; i < count; i++) {
      av_frame_free(&frame_arr[i]);
    }
    free(frame_arr);
  }
  final_result = OASIS_ERROR;

done:
  av_packet_free(&pkt);
  av_frame_free(&frame);
  avcodec_free_context(&codec_ctx);
  avformat_close_input(&fmt_ctx);

  return final_result;
}

oasis_result_t append_frames_to_pcm(AVFrame **frames, int frame_count,
                                    uint8_t **pcm, int *pcm_size,
                                    audio_data_t *audio_data) {
  if (!frames || !frame_count) {
    oasis_log(NULL, LOG_LEVEL_ERROR,
              "Invalid arguments, frames or frame_count is NULL");
    return OASIS_ERROR;
  }

  int total_samples = 0, bytes_per_sample = 0,
      channels = frames[0]->ch_layout.nb_channels;
  enum AVSampleFormat sample_format = frames[0]->format;

  audio_data->channels = channels;
  audio_data->sample_rate = frames[0]->sample_rate;
  audio_data->format = ffmpeg_to_miniaudio_format(sample_format);

  for (int i = 0; i < frame_count; i++) {
    total_samples += frames[i]->nb_samples;
  }

  bytes_per_sample = av_get_bytes_per_sample(sample_format);

  int total_bytes = total_samples * bytes_per_sample * channels;
  *pcm = malloc(total_bytes);
  if (!*pcm) {
    oasis_log(NULL, LOG_LEVEL_ERROR, "Failed to allocate memory for PCM");
    return OASIS_ERROR;
  }

  int offset = 0;
  for (int i = 0; i < frame_count; i++) {
    AVFrame *frame = frames[i];
    int samples = frame->nb_samples;

    if (av_sample_fmt_is_planar(sample_format)) {
      for (int sample = 0; sample < samples; sample++) {
        for (int channel = 0; channel < channels; channel++) {
          memcpy(*pcm + offset,
                 frame->extended_data[channel] + sample * bytes_per_sample,
                 bytes_per_sample);
          offset += bytes_per_sample;
        }
      }
    } else {
      int frame_bytes = samples * bytes_per_sample * channels;
      memcpy(*pcm + offset, frame->extended_data[0], frame_bytes);
      offset += frame_bytes;
    }
  }

  *pcm_size = total_bytes;
  return OASIS_SUCCESS;
}
