#include <oasis/utils.h>
#include "test_utils.h"

#include <dirent.h>
#include <sys/stat.h>
#include <bsd/string.h>

bool is_directory(const char *path) {
  // Check if the path is a directory
  struct stat path_stat;
  if (stat(path, &path_stat) != 0) {
    return false; // Error checking the path
  }
  return S_ISDIR(path_stat.st_mode);
}
