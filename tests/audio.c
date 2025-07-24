// This include is necessary to prevent linker errors during compilation
#include "oasis/utils.h"
#define CLAY_IMPLEMENTATION
#include <clay.h>

#include <dirent.h>
#include <sys/stat.h>
#include <bsd/string.h>
#include <oasis/audio/decode.h>
#include <oasis/audio/playback.h>
#include <unity/unity.h>

char *base_path = "./resources/test_files/";
static char **paths = {0};
static int amount_of_paths = 0;

int get_path_count(void) {
  DIR *dir = opendir(base_path);
  struct stat path_stat;

  if (!dir) {
    oasis_log(NULL, LOG_LEVEL_ERROR, "Failed to open directory %s", base_path
    );
    return 0;
  }

  struct dirent *entry;
  int count = 0;

  while ((entry = readdir(dir)) != NULL) {
    if (entry->d_name[0] == '.')
      continue;

    char full_path[256];
    snprintf(full_path, 256, "%s%s", base_path, entry->d_name);

    stat(full_path, &path_stat);
    if (S_ISDIR(path_stat.st_mode)) {
      count++;
    }
  }

  closedir(dir);
  return count;
}

int populate_paths(void) {
  DIR *dir = opendir(base_path);
  int i = 0;
  struct stat path_stat;

  if (!dir) {
    oasis_log(NULL, LOG_LEVEL_ERROR, "Failed to open directory %s", base_path
    );
    return 0;
  }

  struct dirent *entry;

  while ((entry = readdir(dir)) != NULL) {
    if (entry->d_name[0] == '.')
      continue;

    char full_path[256];
    snprintf(full_path, 256, "%s%s", base_path, entry->d_name);

    stat(full_path, &path_stat);
    if (S_ISDIR(path_stat.st_mode)) {
      if (paths[i] != NULL) {
        strlcpy(paths[i], full_path, strlen(full_path) + 1);
        i++;
      } else {
        oasis_log(NULL, LOG_LEVEL_ERROR, "Path %d is NULL", i);
      }
    }
  }

  closedir(dir);
  return i;
}

oasis_result_t test_decode_a_folder(const char *path) {
  DIR *dir = opendir(path);
  if (!dir) {
    oasis_log(NULL, LOG_LEVEL_ERROR, "Failed to open directory %s", path);
    return 0;
  }

  struct dirent *entry;
  struct stat path_stat;
  int success_count = 0;

  while ((entry = readdir(dir)) != NULL) {
    if (entry->d_name[0] == '.')
      continue;

    char full_path[512];
    snprintf(full_path, 512, "%s/%s", path, entry->d_name);
    stat(full_path, &path_stat);

    if (!S_ISREG(path_stat.st_mode)) {
      continue;
    }

    oasis_log(NULL, LOG_LEVEL_INFO, "Decoding file: %s", full_path);
    AVFrame **frames = NULL;
    int frame_count = 0;
    audio_data_t audio_data = {0};
    oasis_result_t result =
      decode_to_pcm(full_path, &frames, &frame_count, &audio_data);

    if (result != OASIS_SUCCESS) {
      oasis_log(NULL, LOG_LEVEL_ERROR, "Failed to decode %s: %s",
                full_path, serialize_oasis_error_text(result));
    } else {
      oasis_log(NULL, LOG_LEVEL_INFO, "Successfully decoded %s", full_path);
      success_count++;
    }
    free(audio_data.pcm_data);
    for (int i = 0; i < frame_count; i++) {
      av_frame_free(&frames[i]);
    }
    free(frames);
  }

  closedir(dir);

  if (success_count == 0) {
    oasis_log(NULL, LOG_LEVEL_ERROR, "No files decoded successfully in %s", path);
    return OASIS_ERROR;
  } else {
    oasis_log(NULL, LOG_LEVEL_INFO, "Successfully decoded %d files in %s",
              success_count, path);
    return OASIS_SUCCESS;
  }
}

oasis_result_t test_playback_a_folder(const char *path) {
  DIR *dir = opendir(path);
  if (!dir) {
    oasis_log(NULL, LOG_LEVEL_ERROR, "Failed to open directory %s", path);
    return 0;
  }

  struct dirent *entry;
  struct stat path_stat;
  int success_count = 0;

  while ((entry = readdir(dir)) != NULL) {
    if (entry->d_name[0] == '.')
      continue;

    char full_path[512];
    snprintf(full_path, 512, "%s/%s", path, entry->d_name);
    stat(full_path, &path_stat);

    if (!S_ISREG(path_stat.st_mode)) {
      continue;
    }

    oasis_log(NULL, LOG_LEVEL_INFO, "Playing back file: %s", full_path);
    if (playback_play(full_path) != OASIS_SUCCESS) {
      oasis_log(NULL, LOG_LEVEL_ERROR, "Failed to play %s", full_path);
    } else {
      oasis_log(NULL, LOG_LEVEL_INFO, "Successfully played %s", full_path);
      success_count++;
    }
  }

  closedir(dir);

  if (success_count == 0) {
    oasis_log(NULL, LOG_LEVEL_ERROR, "No files played successfully in %s", path);
    return OASIS_ERROR;
  } else {
    oasis_log(NULL, LOG_LEVEL_INFO, "Successfully played back %d files in %s",
              success_count, path);
    return OASIS_SUCCESS;
  }
}


void setUp(void) {
  amount_of_paths = get_path_count();
  int i = 0;

  paths = (char**)malloc(amount_of_paths * 1024);
  if (!paths) {
    oasis_log(NULL, LOG_LEVEL_ERROR, "Failed to allocate memory for paths");
    return;
  }


  for (;i < amount_of_paths; i++) {
    paths[i] = (char*)malloc(1024);
    if (!paths[i]) {
      oasis_log(NULL, LOG_LEVEL_ERROR, "Failed to allocate memory for path %d",
                i);
      return;
    }
  }

  if (populate_paths() == 0) {
    oasis_log(NULL, LOG_LEVEL_ERROR, "Failed to populate paths");
    return;
  }
}

void tearDown(void) {
  for (int i = 0; i < amount_of_paths; i++) {
    if (paths[i]) {
      free(paths[i]);
      paths[i] = NULL;
    }
  }
  free(paths);
  paths = NULL;
  amount_of_paths = 0;
  oasis_log(NULL, LOG_LEVEL_INFO, "Tear down complete, paths freed");
}

void test_audio_decode(void) {
  int success_count = 0;

  for (int i = 0; i < amount_of_paths; i++) {
    if (test_decode_a_folder(paths[i]) != OASIS_SUCCESS) {
      oasis_log(NULL, LOG_LEVEL_ERROR, "Failed to decode folder %s", paths[i]);
    } else {
      oasis_log(NULL, LOG_LEVEL_INFO, "Successfully decoded folder %s", paths[i]);
      success_count++;
    }
  }

  if (success_count == 0) {
    oasis_log(NULL, LOG_LEVEL_ERROR, "No folders decoded successfully");
    TEST_FAIL_MESSAGE("No folders decoded successfully");
  } else {
    oasis_log(NULL, LOG_LEVEL_INFO, "Successfully decoded %d folders", success_count);
    TEST_MESSAGE("Successfully decoded folders");
    TEST_PASS();
  }
}

void test_audio_playback(void) {
  int success_count = 0;

  for (int i = 0; i < amount_of_paths; i++) {
    if (test_playback_a_folder(paths[i]) != OASIS_SUCCESS) {
      oasis_log(NULL, LOG_LEVEL_ERROR, "Failed to play folder %s", paths[i]);
    } else {
      oasis_log(NULL, LOG_LEVEL_INFO, "Successfully played folder %s", paths[i]);
      success_count++;
    }
  }

  if (success_count == 0) {
    oasis_log(NULL, LOG_LEVEL_ERROR, "No folders played successfully");
    TEST_FAIL_MESSAGE("No folders played successfully");
  } else {
    oasis_log(NULL, LOG_LEVEL_INFO, "Successfully played %d folders", success_count);
    TEST_MESSAGE("Successfully played folders");
    TEST_PASS();
  }
}

void test_audio_play(void) {}

int main(void) {
  UNITY_BEGIN()
    ;
  RUN_TEST(test_audio_decode);
  RUN_TEST(test_audio_playback);

  UNITY_END();
}
