#ifndef DECODE_H
#define DECODE_H

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <miniaudio.h>

#include <oasis/utils.h>

typedef struct {
  ma_format format;
  ma_uint32 channels;
  ma_uint32 sample_rate;
} audio_data_t;

/**
 * Decodes an audio file to PCM.
 *
 * @param filename The filename of the audio file to decode.
 * @param frames The array of frames to store the decoded audio in.
 * @param frame_count The number of frames decoded.
 * @return OASIS_SUCCESS if the decoding was successful, OASIS_ERROR otherwise.
 */
oasis_result_t decode_to_pcm(const char *filename, AVFrame ***frames,
                             int *frame_count);

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
                                    uint8_t **pcm, int *pcm_size,
                                    audio_data_t *audio_data);

#endif
