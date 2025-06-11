#ifndef PLAYBACK_H
#define PLAYBACK_H

#include <oasis/utils.h>

/**
 * Plays an audio file.
 *
 * @param filename The filename of the audio file to play.
 * @return OASIS_SUCCESS if the playback was successful, OASIS_ERROR otherwise.
 */

oasis_result_t playback_play(const char *filename);

#endif
