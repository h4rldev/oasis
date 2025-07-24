#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/channel_layout.h>
#include <libavutil/frame.h>
#include <libavutil/opt.h>
#include <libswresample/swresample.h>
#include <oasis/utils.h>

#include <bsd/string.h>
#include <time.h>

#include <oasis/audio/decode.h>
#include <oasis/utils.h>

char *get_audio_codec_name(enum AVCodecID codec_id) {
  static char codec[100];
  const char *codec_name = avcodec_get_name(codec_id);

  strlcpy(codec, codec_name, sizeof(codec));

  return codec;
}

char *get_sample_format_name(enum AVSampleFormat sample_format) {
  static char sample_format_buf[100];
  const char *sample_format_name = av_get_sample_fmt_name(sample_format);

  strlcpy(sample_format_buf, sample_format_name, sizeof(sample_format_name));

  return sample_format_buf;
}

oasis_result_t decode_to_pcm(const char *filename, AVFrame ***frames,
                             int *frame_count, audio_data_t *audio_data) {
  oasis_log(NULL, LOG_LEVEL_DEBUG, "Decoding audio file %s", filename);
  oasis_log(NULL, LOG_LEVEL_DEBUG, "Benchmarking...");
  clock_t start = clock();

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

  oasis_log(NULL, LOG_LEVEL_INFO, "Opening input file %s", filename);
  if (avformat_open_input(&fmt_ctx, filename, NULL, NULL) != 0) {
    oasis_log(NULL, LOG_LEVEL_ERROR, "Failed to open input file %s", filename);
    return OASIS_ERROR;
  }

  oasis_log(NULL, LOG_LEVEL_DEBUG, "Finding stream info");
  if (avformat_find_stream_info(fmt_ctx, NULL) < 0) {
    avformat_close_input(&fmt_ctx);
    oasis_log(NULL, LOG_LEVEL_ERROR, "Failed to find stream info");
    return OASIS_ERROR;
  }

  oasis_log(NULL, LOG_LEVEL_DEBUG, "Finding audio stream");
  for (unsigned i = 0; i < fmt_ctx->nb_streams; i++) {
    if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
      audio_stream_index = i;
      break;
    }
  }

  oasis_log(NULL, LOG_LEVEL_DEBUG, "Audio stream index: %d", audio_stream_index);
  if (audio_stream_index == -1) {
    oasis_log(NULL, LOG_LEVEL_ERROR, "No audio stream found");
    avformat_close_input(&fmt_ctx);
    return OASIS_ERROR_FILE_NOT_FOUND;
  }

  oasis_log(NULL, LOG_LEVEL_DEBUG, "Finding decoder for codec %s",
            get_audio_codec_name(
              fmt_ctx->streams[audio_stream_index]->codecpar->codec_id));
  AVCodecParameters *codecpar = fmt_ctx->streams[audio_stream_index]->codecpar;
  const AVCodec *codec = avcodec_find_decoder(codecpar->codec_id);
  if (!codec) {
    oasis_log(NULL, LOG_LEVEL_ERROR, "Failed to find decoder for codec %s",
              get_audio_codec_name(codecpar->codec_id));
    avformat_close_input(&fmt_ctx);
    return OASIS_ERROR_UNSUPPORTED_FORMAT;
  }

  oasis_log(NULL, LOG_LEVEL_DEBUG, "Allocating codec context");
  codec_ctx = avcodec_alloc_context3(codec);
  if (!codec_ctx) {
    oasis_log(NULL, LOG_LEVEL_ERROR, "Failed to allocate codec context");
    avcodec_free_context(&codec_ctx);
    avformat_close_input(&fmt_ctx);
    return OASIS_ERROR_MEMORY_ALLOCATION;
  }

  oasis_log(NULL, LOG_LEVEL_DEBUG,
            "Copying codec parameters to decoder context");
  if (avcodec_parameters_to_context(codec_ctx, codecpar) < 0) {
    oasis_log(NULL, LOG_LEVEL_ERROR,
              "Failed to copy codec parameters to decoder context");
    avcodec_free_context(&codec_ctx);
    avformat_close_input(&fmt_ctx);
    return OASIS_ERROR;
  }

  oasis_log(NULL, LOG_LEVEL_DEBUG, "Opening codec");
  if (avcodec_open2(codec_ctx, codec, NULL) < 0) {
    oasis_log(NULL, LOG_LEVEL_ERROR, "Failed to open codec");
    avcodec_free_context(&codec_ctx);
    avformat_close_input(&fmt_ctx);
    return OASIS_ERROR_UNSUPPORTED_FORMAT;
  }

  oasis_log(NULL, LOG_LEVEL_DEBUG, "Allocating packet");
  pkt = av_packet_alloc();
  if (!pkt) {
    oasis_log(NULL, LOG_LEVEL_ERROR, "Failed to allocate packet");
    avcodec_free_context(&codec_ctx);
    avformat_close_input(&fmt_ctx);
    return OASIS_ERROR_MEMORY_ALLOCATION;
  }

  oasis_log(NULL, LOG_LEVEL_DEBUG, "Allocating frame");
  frame = av_frame_alloc();
  if (!frame) {
    oasis_log(NULL, LOG_LEVEL_ERROR, "Failed to allocate frame");
    av_packet_free(&pkt);
    avcodec_free_context(&codec_ctx);
    avformat_close_input(&fmt_ctx);
    return OASIS_ERROR_MEMORY_ALLOCATION;
  }

  oasis_log(NULL, LOG_LEVEL_DEBUG, "Allocating frame array");
  AVFrame **frame_arr = malloc(initial_capacity * sizeof(AVFrame *));
  if (!frame_arr) {
    oasis_log(NULL, LOG_LEVEL_ERROR, "Failed to allocate frame array");
    av_packet_free(&pkt);
    av_frame_free(&frame);
    avcodec_free_context(&codec_ctx);
    avformat_close_input(&fmt_ctx);
    return OASIS_ERROR_MEMORY_ALLOCATION;
  }

  oasis_log(NULL, LOG_LEVEL_DEBUG, "Reading frames");
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
          oasis_log(NULL, LOG_LEVEL_DEBUG, "Reallocating frame array");
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

  oasis_log(NULL, LOG_LEVEL_DEBUG, "Sending NULL packet to flush decoder");
  avcodec_send_packet(codec_ctx, NULL);
  while (1) {
    ret = avcodec_receive_frame(codec_ctx, frame);
    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
      break;
    else if (ret < 0) {
      oasis_log(NULL, LOG_LEVEL_ERROR, "Error receiving frame: %s",
                av_err2str(ret));
      goto clean_frames;
    }
    if (count >= initial_capacity) {
      oasis_log(NULL, LOG_LEVEL_DEBUG, "Reallocating frame array");
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


  /*
   AV_SAMPLE_FMT_U8,          ///< unsigned 8 bits
   AV_SAMPLE_FMT_S16,         ///< signed 16 bits
   AV_SAMPLE_FMT_S32,         ///< signed 32 bits
   AV_SAMPLE_FMT_FLT,         ///< float
   AV_SAMPLE_FMT_DBL,         ///< double

   AV_SAMPLE_FMT_U8P,         ///< unsigned 8 bits, planar
   AV_SAMPLE_FMT_S16P,        ///< signed 16 bits, planar
   AV_SAMPLE_FMT_S32P,        ///< signed 32 bits, planar
   AV_SAMPLE_FMT_FLTP,        ///< float, planar
   AV_SAMPLE_FMT_DBLP,        ///< double, planar
   AV_SAMPLE_FMT_S64,         ///< signed 64 bits
   AV_SAMPLE_FMT_S64P,
   */

  // Get codec id
  switch (codec_ctx->sample_fmt) {
  case AV_SAMPLE_FMT_U8:
    audio_data->codec_id = AV_CODEC_ID_PCM_U8;
    break;
  case AV_SAMPLE_FMT_S16:
    audio_data->codec_id = AV_CODEC_ID_PCM_S16LE;
    break;
  case AV_SAMPLE_FMT_S32:
    audio_data->codec_id = AV_CODEC_ID_PCM_S32LE;
    break;
  case AV_SAMPLE_FMT_FLT:
    audio_data->codec_id = AV_CODEC_ID_PCM_F32LE;
    break;
  case AV_SAMPLE_FMT_DBL:
    audio_data->codec_id = AV_CODEC_ID_PCM_F64LE;
    break;
  case AV_SAMPLE_FMT_U8P:
    audio_data->codec_id = AV_CODEC_ID_PCM_S8_PLANAR;
    break;
  case AV_SAMPLE_FMT_S16P:
    audio_data->codec_id = AV_CODEC_ID_PCM_S16LE_PLANAR;
    break;
  case AV_SAMPLE_FMT_S32P:
    audio_data->codec_id = AV_CODEC_ID_PCM_S32LE_PLANAR;
    break;
  case AV_SAMPLE_FMT_FLTP:
    audio_data->codec_id = fmt_ctx->streams[audio_stream_index]->codecpar->codec_id;
    break;
  case AV_SAMPLE_FMT_DBLP:
    audio_data->codec_id = fmt_ctx->streams[audio_stream_index]->codecpar->codec_id;
    break;
  case AV_SAMPLE_FMT_S64:
    audio_data->codec_id = AV_CODEC_ID_PCM_S64LE;
    break;
  case AV_SAMPLE_FMT_S64P:
    audio_data->codec_id = fmt_ctx->streams[audio_stream_index]->codecpar->codec_id;
    break;
  default:
    oasis_log(NULL, LOG_LEVEL_ERROR, "Unsupported sample format");
    final_result = OASIS_ERROR_UNSUPPORTED_FORMAT;
    goto clean_frames;
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

  clock_t end = clock();
  double elapsed_time = (double)(end - start) / CLOCKS_PER_SEC;
  oasis_log(NULL, LOG_LEVEL_INFO, "Finished decoding audio file in %f seconds",
            elapsed_time);
  return final_result;
}

oasis_result_t append_frames_to_pcm(AVFrame **frames, int frame_count,
                                    audio_data_t *audio_data) {
  clock_t start = clock();
  oasis_log(NULL, LOG_LEVEL_INFO, "Benchmarking...");

  oasis_log(NULL, LOG_LEVEL_DEBUG, "Appending frames to PCM");
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
  audio_data->sample_format = sample_format;

  oasis_log(NULL, LOG_LEVEL_DEBUG, "Audio data format: %s",
            get_sample_format_name(sample_format));

  oasis_log(NULL, LOG_LEVEL_DEBUG, "Calculating total samples");
  for (int i = 0; i < frame_count; i++) {
    total_samples += frames[i]->nb_samples;
  }

  oasis_log(NULL, LOG_LEVEL_DEBUG, "Calculating bytes per sample");
  bytes_per_sample = av_get_bytes_per_sample(sample_format);

  oasis_log(NULL, LOG_LEVEL_DEBUG, "Bytes per sample: %d", bytes_per_sample);

  oasis_log(NULL, LOG_LEVEL_DEBUG, "Calculating total bytes");
  int total_bytes = total_samples * bytes_per_sample * channels;
  audio_data->pcm_data = malloc(total_bytes);
  if (!audio_data->pcm_data) {
    oasis_log(NULL, LOG_LEVEL_ERROR, "Failed to allocate memory for PCM");
    return OASIS_ERROR;
  }

  oasis_log(NULL, LOG_LEVEL_DEBUG, "Copying frames to PCM");
  int offset = 0;
  for (int i = 0; i < frame_count; i++) {
    AVFrame *frame = frames[i];
    int samples = frame->nb_samples;

    if (av_sample_fmt_is_planar(sample_format)) {
      for (int sample = 0; sample < samples; sample++) {
        for (int channel = 0; channel < channels; channel++) {
          memcpy(audio_data->pcm_data + offset,
                 frame->extended_data[channel] + sample * bytes_per_sample,
                 bytes_per_sample);
          offset += bytes_per_sample;
        }
      }
    } else {
      int frame_bytes = samples * bytes_per_sample * channels;
      memcpy(audio_data->pcm_data + offset, frame->extended_data[0],
             frame_bytes);
      offset += frame_bytes;
    }
  }

  oasis_log(NULL, LOG_LEVEL_DEBUG, "PCM size: %d", audio_data->pcm_size);
  oasis_log(NULL, LOG_LEVEL_DEBUG, "Finished appending frames to PCM");

  clock_t end = clock();
  double elapsed_time = (double)(end - start) / CLOCKS_PER_SEC;
  oasis_log(NULL, LOG_LEVEL_INFO,
            "Finished appending frames to PCM in %f "
            "seconds",
            elapsed_time);

  audio_data->pcm_size = total_bytes;
  return OASIS_SUCCESS;
}
