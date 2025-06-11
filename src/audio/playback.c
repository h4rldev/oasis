#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>

#include <oasis/audio/decode.h>
#include <oasis/audio/playback.h>
#include <oasis/utils.h>

typedef struct {
  uint8_t *pcm;       // PCM data buffer
  size_t pcmSize;     // Total size in bytes
  size_t currentPos;  // Current position in the buffer
  ma_format format;   // Sample format
  ma_uint32 channels; // Channel count
  bool isPlaying;     // Playback state flag
} playback_data_t;

void data_callback(ma_device *pDevice, void *pOutput, const void *pInput,
                   ma_uint32 frameCount) {
  playback_data_t *pData = (playback_data_t *)pDevice->pUserData;
  if (pData == NULL || !pData->isPlaying) {
    // If no data or playback stopped, output silence
    memset(pOutput, 0,
           frameCount * pData->channels *
             ma_get_bytes_per_sample(pData->format));
    return;
  }

  // Calculate how many bytes each frame occupies
  size_t bytesPerFrame =
    pData->channels * ma_get_bytes_per_sample(pData->format);

  // Calculate remaining frames and how many we can copy
  size_t remainingBytes = pData->pcmSize - pData->currentPos;
  size_t remainingFrames = remainingBytes / bytesPerFrame;
  size_t framesToCopy =
    (frameCount < remainingFrames) ? frameCount : remainingFrames;
  size_t bytesToCopy = framesToCopy * bytesPerFrame;

  // Copy available data to output buffer
  if (framesToCopy > 0) {
    memcpy(pOutput, pData->pcm + pData->currentPos, bytesToCopy);
    pData->currentPos += bytesToCopy;
  }

  // If we've reached the end of the buffer, either stop or loop
  if (pData->currentPos >= pData->pcmSize) {
    // Option 1: Stop playback
    // pData->isPlaying = false;

    // Option 2: Loop playback (uncomment to enable)
    pData->currentPos = 0;

    // Fill any remaining output with silence
    size_t remainingOutputBytes = (frameCount - framesToCopy) * bytesPerFrame;
    if (remainingOutputBytes > 0) {
      memset((uint8_t *)pOutput + bytesToCopy, 0, remainingOutputBytes);
    }
  }
}

oasis_result_t playback_play(const char *filename) {
  oasis_result_t final_result = OASIS_SUCCESS;

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
  oasis_result_t result = decode_to_pcm(filename, &frames, &frame_count);
  if (result != OASIS_SUCCESS) {
    return result;
  }

  uint8_t *pcm = NULL;
  int pcm_size = 0;
  audio_data_t audio_data;
  result =
    append_frames_to_pcm(frames, frame_count, &pcm, &pcm_size, &audio_data);
  if (result != OASIS_SUCCESS) {
    final_result = result;
    goto free_pcm;
  }

  ma_device_config config;
  ma_device device;

  playback_data_t data = {.pcm = pcm,
                          .pcmSize = pcm_size,
                          .currentPos = 0,
                          .format = audio_data.format,
                          .channels = audio_data.channels,
                          .isPlaying = true};

  config = ma_device_config_init(ma_device_type_playback);
  device.playback.format = audio_data.format;
  device.playback.channels = audio_data.channels;
  device.sampleRate = audio_data.sample_rate;
  config.dataCallback = data_callback;
  config.pUserData = &data;

  if (ma_device_init(NULL, &config, &device) != MA_SUCCESS) {
    oasis_log(NULL, LOG_LEVEL_ERROR, "Failed to initialize device");
    final_result = OASIS_ERROR;
    goto free_pcm;
  }

  if (ma_device_start(&device) != MA_SUCCESS) {
    oasis_log(NULL, LOG_LEVEL_ERROR, "Failed to start device");
    final_result = OASIS_ERROR;
    goto free_pcm;
  }

  oasis_log(NULL, LOG_LEVEL_INFO, "Playing audio file %s, press 'q' to stop",
            filename);

  while (getchar() != 'q')
    ;

  data.isPlaying = false;
  ma_device_uninit(&device);

free_pcm:
  free(pcm);
  for (int i = 0; i < frame_count; i++) {
    av_frame_free(&frames[i]);
  }
  free(frames);
  return final_result;
}
