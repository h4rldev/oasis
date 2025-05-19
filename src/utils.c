#include <clay.h>
#include <stdio.h>

int handle_error(Clay_ErrorData *error_data) {
  fprintf(stderr, "Error: %s\n", error_data->errorText.chars);
  return error_data->errorType;
}

char *serialize_error_text(Clay_ErrorType error_type) {
  switch (error_type) {
  CLAY_ERROR_TYPE_TEXT_MEASUREMENT_CAPACITY_EXCEEDED:
    return "Text measurement capacity exceeded";
  CLAY_ERROR_TYPE_ARENA_CAPACITY_EXCEEDED:
    return "Arena capacity exceeded";
  CLAY_ERROR_TYPE_ELEMENTS_CAPACITY_EXCEEDED:
    return "Elements capacity exceeded";
  CLAY_ERROR_TYPE_DUPLICATE_ID:
    return "Duplicate ID";
  CLAY_ERROR_TYPE_FLOATING_CONTAINER_PARENT_NOT_FOUND:
    return "Floating container parent not found";
  CLAY_ERROR_TYPE_INTERNAL_ERROR:
    return "Internal error";
  default:
    return "Unknown error";
  }
}
