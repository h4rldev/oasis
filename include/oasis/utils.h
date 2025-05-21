#ifndef UTILS_H
#define UTILS_H

#include <clay.h>

void handle_error(Clay_ErrorData error_data);

char *serialize_error_text(Clay_ErrorType error_type);

Clay_Color hex_to_color(const char *hex);

char *clay_color_to_hex(Clay_Color color);

#endif
