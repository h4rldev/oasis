#ifndef UTILS_H
#define UTILS_H

#include <clay.h>
#include <stdio.h>

/**
 * The return codes used by oasis.
 */
typedef enum {
  // Basic return codes
  OASIS_SUCCESS = 0,
  OASIS_ERROR = -1,

  // Error codes
  OASIS_ERROR_INVALID_ARGUMENT = -2,
  OASIS_ERROR_FFMPEG_MEMORY_ALLOCATION = -3,
  OASIS_ERROR_MEMORY_ALLOCATION = -4, // General memory allocation error
  OASIS_ERROR_FILE_NOT_FOUND = -5,
  OASIS_ERROR_DIRECTORY_NOT_FOUND = -6,
  OASIS_ERROR_FILE_NOT_MEDIA = -7,
  OASIS_ERROR_UNSUPPORTED_FORMAT = -8,
  OASIS_ERROR_UNSUPPORTED_CODEC = -9, // Codec not supported
  OASIS_ERROR_UNSUPPORTED_OPERATION = -10, // Operation not supported

  // Unknown error
  OASIS_ERROR_UNKNOWN = -11
} oasis_result_t;

/**
 * The log levels used by oasis_log.
 */
typedef enum {
  LOG_LEVEL_ERROR = 0,
  LOG_LEVEL_INFO = 1,
  LOG_LEVEL_WARN = 2,
  LOG_LEVEL_DEBUG = 3,
} log_level;

/**
 * Logs a message to the console.
 *
 * @param file: The file to write the message to. If NULL, stderr will be used.
 * @param level: The level of the message, if somehow it's not anything
 * recognized, it will be treated as an info message.
 * @param format: The format string.
 * @param ...: The arguments to the format string.
 * @return OASIS_SUCCESS if the message was logged successfully, OASIS_ERROR
 * otherwise.
 */
oasis_result_t oasis_log(FILE *file, log_level level, const char *format, ...);

/**
 * Handles an error from clay.
 *
 * @param error_data: The error data.
 */
void handle_error(Clay_ErrorData error_data);

/**
 * Serializes an error type to a string.
 *
 * @param error_type: The error type to serialize.
 * @return A string representation of the error type.
 */
char *serialize_clay_error_text(Clay_ErrorType error_type);

/**
 * Serializes an oasis error type to a string.
 *
 * @param error_type: The error type to serialize.
 * @return A string representation of the error type.
 */
char *serialize_oasis_error_text(oasis_result_t error_type);

/**
 * Converts a hex string to a color.
 *
 * @param hex: The hex string to convert.
 * @return The color.
 */
Clay_Color hex_to_color(const char *hex);

/**
 * Converts a hex color to a string.
 *
 * @param color: The color to convert.
 * @return A string representation of the color.
 */
char *clay_color_to_hex(Clay_Color color);
#endif
