#ifndef UTILS_H
#define UTILS_H

#include <clay.h>

int handle_error(Clay_ErrorData *error_data);

char *serialize_error_text(Clay_ErrorType error_type);

#endif
