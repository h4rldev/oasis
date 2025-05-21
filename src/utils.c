#include <clay.h>
#include <stdio.h>
#include <string.h>

#include <oasis/utils.h>

void handle_error(Clay_ErrorData error_data) {
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
