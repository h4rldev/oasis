#include <oasis/audio/decode.h>
#include <oasis/audio/metadata.h>
#include <oasis/utils.h>

#include <libavutil/dict.h>
#include <libavutil/samplefmt.h>

audio_metadata_t *_audio_metadata(AVDictionary *metadata) {
  audio_metadata_t *result = NULL;

  if (!metadata) {
    return NULL;
  }



  return result;
}
