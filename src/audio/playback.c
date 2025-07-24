#define _POSIX_C_SOURCE 199309L

#include <oasis/audio/decode.h>
#include <oasis/audio/playback.h>
#include <oasis/utils.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#include <libavdevice/avdevice.h>

static const char *get_audio_device_name(const char *format_name) {
  if (strcmp(format_name, "oss") == 0) {
    return "/dev/dsp"; // OSS uses device files
  }
  return NULL;
}

oasis_result_t playback_play(const char *filename) {
  oasis_result_t final_result = OASIS_SUCCESS;
  static struct termios oldt, newt;

  audio_data_t audio_data;

  AVFormatContext *output_format_ctx = NULL;
  AVStream *out_stream = NULL;
  int ret;

  if (!filename) {
    oasis_log(NULL, LOG_LEVEL_ERROR, "Invalid arguments, filename is NULL");
    return OASIS_ERROR;
  }

  struct stat st;
  if (stat(filename, &st) == -1) {
    oasis_log(NULL, LOG_LEVEL_ERROR, "Failed to stat file %s, does it exist?",
              filename);
    return OASIS_ERROR;
  }

  AVFrame **frames = NULL;
  int frame_count = 0;
  oasis_result_t result =
    decode_to_pcm(filename, &frames, &frame_count, &audio_data);
  if (result != OASIS_SUCCESS) {
    return result;
  }

  result = append_frames_to_pcm(frames, frame_count, &audio_data);
  if (result != OASIS_SUCCESS) {
    final_result = result;
    goto free_pcm;
  }

  avdevice_register_all();

  const char *try_output_formats[] = {"pulse", "alsa", "oss", NULL};
  const char *output_format_name = NULL;
  const AVOutputFormat *output_format = NULL;

  for (int i = 0; try_output_formats[i] != NULL; i++) {
    output_format = av_guess_format(try_output_formats[i], NULL, NULL);
    if (output_format) {
      output_format_name = try_output_formats[i];
      oasis_log(NULL, LOG_LEVEL_INFO, "Using audio output: %s",
                output_format_name);
      break;
    }
  }

  if (!output_format) {
    oasis_log(NULL, LOG_LEVEL_ERROR, "No supported audio output format found");
    return OASIS_ERROR;
  }

  const char *output_device_name = get_audio_device_name(output_format_name);

  ret = avformat_alloc_output_context2(&output_format_ctx, output_format,
                                       output_format_name, output_device_name);
  if (ret < 0 || !output_format_ctx) {
    char errbuf[AV_ERROR_MAX_STRING_SIZE];
    av_strerror(ret, errbuf, sizeof(errbuf));
    oasis_log(NULL, LOG_LEVEL_ERROR, "Failed to allocate output context: %s",
              errbuf);
    return OASIS_ERROR;
  }

  out_stream = avformat_new_stream(output_format_ctx, NULL);
  if (!out_stream) {
    oasis_log(NULL, LOG_LEVEL_ERROR, "Failed to create output stream");
    avformat_free_context(output_format_ctx);
    return OASIS_ERROR;
  }

  // Set up stream parameters based on your PCM data
  out_stream->codecpar->codec_type = AVMEDIA_TYPE_AUDIO;
  out_stream->codecpar->codec_id = audio_data.codec_id;
  oasis_log(NULL, LOG_LEVEL_DEBUG, "Setting codec ID to %s",
            get_audio_codec_name(audio_data.codec_id));
  out_stream->codecpar->sample_rate = audio_data.sample_rate;
  oasis_log(NULL, LOG_LEVEL_DEBUG, "Setting sample rate to %d",
            audio_data.sample_rate);

  out_stream->codecpar->ch_layout.nb_channels = audio_data.channels;
  if (audio_data.channels == 1) {
    av_channel_layout_from_mask(&out_stream->codecpar->ch_layout,
                                AV_CH_LAYOUT_MONO);
    oasis_log(NULL, LOG_LEVEL_DEBUG, "Setting channel layout to mono");
  } else if (audio_data.channels == 2) {
    av_channel_layout_from_mask(&out_stream->codecpar->ch_layout,
                                AV_CH_LAYOUT_STEREO);
    oasis_log(NULL, LOG_LEVEL_DEBUG, "Setting channel layout to stereo");
  } else {
    av_channel_layout_default(&out_stream->codecpar->ch_layout,
                              audio_data.channels);
    oasis_log(NULL, LOG_LEVEL_DEBUG, "Setting channel layout to %d",
              audio_data.channels);
  }

  out_stream->codecpar->format = audio_data.sample_format;
  oasis_log(NULL, LOG_LEVEL_DEBUG, "Setting sample format to %s",
            get_sample_format_name(audio_data.sample_format));

  int bytes_per_sample = av_get_bytes_per_sample(audio_data.sample_format);
  out_stream->codecpar->frame_size = 0; // Let pulseaudio decide the frame size

  int64_t bit_rate =
    audio_data.sample_rate * audio_data.channels * bytes_per_sample * 8;

  out_stream->codecpar->bit_rate = bit_rate;
  oasis_log(NULL, LOG_LEVEL_DEBUG, "Setting bit rate to %d", bit_rate);

  AVDictionary *options = NULL;
  av_dict_set(&options, "device", "default", 0);

  if (output_device_name != NULL) {
    ret =
      avio_open(&output_format_ctx->pb, output_device_name, AVIO_FLAG_WRITE);
    if (ret < 0) {
      char errbuf[AV_ERROR_MAX_STRING_SIZE];
      av_strerror(ret, errbuf, sizeof(errbuf));
      oasis_log(NULL, LOG_LEVEL_ERROR, "Failed to open output device: %s",
                errbuf);
      avformat_free_context(output_format_ctx);
      return OASIS_ERROR;
    }
  }

  ret = avformat_write_header(output_format_ctx, &options);
  av_dict_free(&options);
  if (ret < 0) {
    char errbuf[AV_ERROR_MAX_STRING_SIZE];
    av_strerror(ret, errbuf, sizeof(errbuf));
    oasis_log(NULL, LOG_LEVEL_ERROR, "Failed to write header: %s", errbuf);
    avio_closep(&output_format_ctx->pb);
    avformat_free_context(output_format_ctx);
    final_result = OASIS_ERROR;
    goto free_pcm;
  }

  oasis_log(NULL, LOG_LEVEL_INFO, "Playing audio file %s, press 'q' to stop",
            filename);

  tcgetattr(STDIN_FILENO, &oldt);
  /*now the settings will be copied*/
  newt = oldt;

  newt.c_lflag &= ~(ICANON | ECHO);

  /*Those new settings will be set to STDIN
  TCSANOW tells tcsetattr to change attributes immediately. */
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);
  fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO, F_GETFL) | O_NONBLOCK);

  int frame_size = out_stream->codecpar->frame_size;
  if (frame_size <= 0) {
    frame_size = 1024; // Default frame size
  }

  int chunk_size = frame_size * audio_data.channels * bytes_per_sample;

  int playing = 1;
  audio_data.pcm_position = 0;
  int64_t pts = 0;
  int packets_written = 0;

  while (playing && audio_data.pcm_position < audio_data.pcm_size) {
    AVPacket *packet = av_packet_alloc();
    if (!packet) {
      oasis_log(NULL, LOG_LEVEL_ERROR, "Failed to allocate packet");
      break;
    }

    int remaining = audio_data.pcm_size - audio_data.pcm_position;
    int current_chunk = (remaining < chunk_size) ? remaining : chunk_size;

    // Allocate packet data
    ret = av_new_packet(packet, current_chunk);
    if (ret < 0) {
      char errbuf[AV_ERROR_MAX_STRING_SIZE];
      av_strerror(ret, errbuf, sizeof(errbuf));
      oasis_log(NULL, LOG_LEVEL_ERROR, "Failed to allocate packet: %s", errbuf);
      av_packet_free(&packet);
      break;
    }

    // Copy PCM data to packet
    memcpy(packet->data, audio_data.pcm_data + audio_data.pcm_position,
           current_chunk);

    // Set packet parameters correctly
    packet->stream_index = 0;
    packet->pts = pts;
    packet->dts = pts;

    // Calculate duration in samples
    int samples_in_chunk =
      current_chunk / (audio_data.channels * bytes_per_sample);
    packet->duration = samples_in_chunk;

    // Update PTS for next packet
    pts += samples_in_chunk;

    // Write packet
    ret = av_interleaved_write_frame(output_format_ctx, packet);
    if (ret < 0) {
      char errbuf[AV_ERROR_MAX_STRING_SIZE];
      av_strerror(ret, errbuf, sizeof(errbuf));
      oasis_log(NULL, LOG_LEVEL_ERROR, "Error writing PCM frame: %s", errbuf);
      av_packet_free(&packet);
      playing = 0;
      break;
    }

    packets_written++;

    // Log progress every 50 packets
    if (packets_written % 50 == 0) {
      double progress =
        (double)audio_data.pcm_position / audio_data.pcm_size * 100.0;
      oasis_log(NULL, LOG_LEVEL_DEBUG, "Progress: %.1f%% (%d packets written)",
                progress, packets_written);
    }

    av_packet_free(&packet);
    audio_data.pcm_position += current_chunk;

    // Check for quit input (non-blocking)
    char ch = 0;
    if (read(STDIN_FILENO, &ch, 1) > 0) {
      switch (ch) {
      case 'q':
      case 'Q':
        oasis_log(NULL, LOG_LEVEL_INFO, "Stopping playback");
        playing = 0;
        break;
      case 'p':
      case 'P':
        oasis_log(NULL, LOG_LEVEL_DEBUG, "Pausing playback");
        while (1) {
          char pause_ch = 0;
          if (read(STDIN_FILENO, &pause_ch, 1) > 0) {
            if (pause_ch == 'p' || pause_ch == 'P') {
              oasis_log(NULL, LOG_LEVEL_DEBUG, "Resuming playback");
              break; // Resume playback
            } else if (pause_ch == 'q' || pause_ch == 'Q') {
              oasis_log(NULL, LOG_LEVEL_DEBUG, "Stopping playback");
              playing = 0;
              break; // Stop playback
            }
          }
        }
        break;
      case 'r':
      case 'R':
        oasis_log(NULL, LOG_LEVEL_DEBUG, "Restarting playback");
        audio_data.pcm_position = 0;
        packets_written = 0;
        pts = 0; // Reset PTS
        av_write_trailer(output_format_ctx);
        ret = avformat_write_header(output_format_ctx, NULL);
        if (ret < 0) {
          char errbuf[AV_ERROR_MAX_STRING_SIZE];
          av_strerror(ret, errbuf, sizeof(errbuf));
          oasis_log(NULL, LOG_LEVEL_ERROR, "Failed to write header: %s", errbuf);
          final_result = OASIS_ERROR;
          playing = 0;
          break;
        }
        break;
      case 's':
      case 'S':
        oasis_log(NULL, LOG_LEVEL_DEBUG, "Skipping forward 5 seconds");
        audio_data.pcm_position += 5 * audio_data.sample_rate *
                                    audio_data.channels * bytes_per_sample;
        if (audio_data.pcm_position >= audio_data.pcm_size) {
          oasis_log(NULL, LOG_LEVEL_DEBUG, "Reached end of audio data");
          audio_data.pcm_position = audio_data.pcm_size; // Clamp to end
          playing = 0; // Stop playback if we reach the end
        }
        pts = audio_data.pcm_position / (audio_data.channels * bytes_per_sample);
        packets_written = 0; // Reset packet count after skipping
        av_write_trailer(output_format_ctx);
        ret = avformat_write_header(output_format_ctx, NULL);
        if (ret < 0) {
          char errbuf[AV_ERROR_MAX_STRING_SIZE];
          av_strerror(ret, errbuf, sizeof(errbuf));
          oasis_log(NULL, LOG_LEVEL_ERROR, "Failed to write header: %s", errbuf);
          final_result = OASIS_ERROR;
          playing = 0;
          break;
        }
        break;
      case 'b':
      case 'B':
        oasis_log(NULL, LOG_LEVEL_DEBUG, "Skipping backward 5 seconds");
        if (audio_data.pcm_position < (size_t)5 * audio_data.sample_rate *
                                        audio_data.channels * bytes_per_sample) {
          oasis_log(NULL, LOG_LEVEL_DEBUG, "Cannot skip backward beyond start");
          audio_data.pcm_position = 0; // Clamp to start
        } else {
          audio_data.pcm_position -= 5 * audio_data.sample_rate *
                                    audio_data.channels * bytes_per_sample;
        }

        if (audio_data.pcm_position == 0) {
          oasis_log(NULL, LOG_LEVEL_DEBUG, "Reached start of audio data");
          audio_data.pcm_position = 0; // Clamp to start
        }

        pts = audio_data.pcm_position / (audio_data.channels * bytes_per_sample);
        packets_written = 0; // Reset packet count after skipping
        av_write_trailer(output_format_ctx);
        ret = avformat_write_header(output_format_ctx, NULL);
        if (ret < 0) {
          char errbuf[AV_ERROR_MAX_STRING_SIZE];
          av_strerror(ret, errbuf, sizeof(errbuf));
          oasis_log(NULL, LOG_LEVEL_ERROR, "Failed to write header: %s", errbuf);
          final_result = OASIS_ERROR;
          playing = 0;
          break;
        }
        break;
      case 'h':
      case 'H':
        oasis_log(NULL, LOG_LEVEL_INFO,
                  "Playback controls:\n"
                  "  q/Q: Quit playback\n"
                  "  p/P: Pause/Resume playback\n"
                  "  r/R: Restart playback\n"
                  "  s/S: Skip forward 5 seconds\n"
                  "  b/B: Skip backward 5 seconds\n"
                  "  h/H: Show this help message");
        break;
      case '\n':
      case '\r':
        // Ignore newline characters
        break;
      default:
        oasis_log(NULL, LOG_LEVEL_WARN, "Unknown command '%c', ignoring", ch);
        break;
      }
    }

    // Reduce the delay - 10ms might be too much
    nanosleep((const struct timespec[]){{0, 1000000}},
              NULL); // 1ms instead of 10ms
  }
  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
  fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO, F_GETFL) & ~O_NONBLOCK);

  av_write_trailer(output_format_ctx);
  avio_closep(&output_format_ctx->pb);
  avformat_free_context(output_format_ctx);

free_pcm:
  free(audio_data.pcm_data);
  for (int i = 0; i < frame_count; i++) {
    av_frame_free(&frames[i]);
  }
  free(frames);
  return final_result;
}
