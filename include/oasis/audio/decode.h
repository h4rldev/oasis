#ifndef DECODE_H
#define DECODE_H

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

#include <oasis/utils.h>

typedef struct {
  uint8_t *pcm_data;
  size_t pcm_size;
  size_t pcm_position;
  int sample_rate;
  int channels;
  enum AVSampleFormat sample_format;
  enum AVCodecID codec_id;
} audio_data_t;

/**
 * Gets the name of the audio codec.
 *
 * @param codec_id The codec ID of the audio codec.
 * @return The name of the audio codec.
 */
char *get_audio_codec_name(enum AVCodecID codec_id);

/**
 * Gets the name of the sample format.
 *
 * @param sample_format The sample format.
 * @return The name of the sample format.
 */
char *get_sample_format_name(enum AVSampleFormat sample_format);

/**
 * Decodes an audio file to PCM.
 *
 * @param filename The filename of the audio file to decode.
 * @param frames The array of frames to store the decoded audio in.
 * @param frame_count The number of frames decoded.
 * @return OASIS_SUCCESS if the decoding was successful, OASIS_ERROR otherwise.
 */
oasis_result_t decode_to_pcm(const char *filename, AVFrame ***frames,
                             int *frame_count, audio_data_t *audio_data);

/**
 * Appends the frames to the PCM.
 *
 * @param frames The array of frames to append to the PCM.
 * @param frame_count The number of frames to append.
 * @param pcm The pointer to the PCM to append to.
 * @param pcm_size The size of the PCM.
 * @return OASIS_SUCCESS if the appending was successful, OASIS_ERROR otherwise.
 */
oasis_result_t append_frames_to_pcm(AVFrame **frames, int frame_count,
                                    audio_data_t *audio_data);

#endif
