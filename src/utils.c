#include <clay.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <oasis/utils.h>

oasis_result oasis_log(FILE *file, log_level level, const char *format, ...) {
  char log_buffer[2048];
  char va_buffer[1024];
  char time_buffer[32];
  char prefix[32];

  time_t now = time(NULL);
  struct tm *time_info = localtime(&now);
  strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %H:%M:%S", time_info);

  va_list args;

  switch (level) {
  case LOG_LEVEL_DEBUG:
    strcpy(prefix, "DEBUG");
    break;
  case LOG_LEVEL_INFO:
    strcpy(prefix, "INFO");
    break;
  case LOG_LEVEL_WARN:
    strcpy(prefix, "WARN");
    break;
  case LOG_LEVEL_ERROR:
    strcpy(prefix, "ERROR");
    break;
  default:
    strcpy(prefix, "INFO");
    break;
  }

  if (file == NULL) {
    file = stderr;
  }

  va_start(args, format);
  vsnprintf(va_buffer, sizeof(va_buffer), format, args);
  snprintf(log_buffer, sizeof(log_buffer), "[%s] OASIS - %s: %s", time_buffer,
           prefix, va_buffer);

  if (fprintf(file, "%s", log_buffer) <= 0) {
    oasis_log(NULL, LOG_LEVEL_ERROR, "Error writing log message");
    va_end(args);
    return OASIS_ERROR;
  }

  va_end(args);
  return OASIS_SUCCESS;
}

void handle_error(Clay_ErrorData error_data) {
  const char *error_text = error_data.errorText.chars;

  if (oasis_log(NULL, LOG_LEVEL_ERROR, "%s", error_text != OASIS_SUCCESS))
    fprintf(stderr, "Error: %s\n", error_data.errorText.chars);
}

char *serialize_error_text(Clay_ErrorType error_type) {
  switch (error_type) {
  case CLAY_ERROR_TYPE_TEXT_MEASUREMENT_CAPACITY_EXCEEDED:
    return "Text measurement capacity exceeded";
  case CLAY_ERROR_TYPE_ARENA_CAPACITY_EXCEEDED:
    return "Arena capacity exceeded";
  case CLAY_ERROR_TYPE_ELEMENTS_CAPACITY_EXCEEDED:
    return "Elements capacity exceeded";
  case CLAY_ERROR_TYPE_DUPLICATE_ID:
    return "Duplicate ID";
  case CLAY_ERROR_TYPE_FLOATING_CONTAINER_PARENT_NOT_FOUND:
    return "Floating container parent not found";
  case CLAY_ERROR_TYPE_INTERNAL_ERROR:
    return "Internal error";
  default:
    return "Unknown error";
  }
}

Clay_Color hex_to_color(const char *hex) {
  if (strlen(hex) != 7) {
    return (Clay_Color){0, 0, 0, 0};
  }

  char r[3];
  char g[3];
  char b[3];
  char a[3];

  r[0] = hex[1];
  r[1] = hex[2];
  r[2] = '\0';

  g[0] = hex[3];
  g[1] = hex[4];
  g[2] = '\0';

  b[0] = hex[5];
  b[1] = hex[6];
  b[2] = '\0';

  a[0] = hex[7];
  a[1] = hex[8];
  a[2] = '\0';

  return (Clay_Color){
    .r = strtol(r, NULL, 16),
    .g = strtol(g, NULL, 16),
    .b = strtol(b, NULL, 16),
    .a = strtol(a, NULL, 16),
  };
}

char *clay_color_to_hex(Clay_Color color) {
  static char hex[32];
  snprintf(hex, 32, "#%02X%02X%02X%02X", (int)color.r, (int)color.g,
           (int)color.b, (int)color.a);
  return hex;
}
