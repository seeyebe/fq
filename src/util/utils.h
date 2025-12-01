#ifndef UTILS_H
#define UTILS_H

#include "../platform/compat.h"
#include <windows.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

int parse_date_string(const char *date_str, FILETIME *file_time);
void format_filetime_iso(const FILETIME *file_time, char *buffer, size_t buffer_size);

int parse_size_arg(const char *arg, uint64_t *size);
int parse_size_with_operator(const char *arg, uint64_t *size, char *operator);

extern const char* text_extensions[];
extern const char* image_extensions[];
extern const char* video_extensions[];
extern const char* audio_extensions[];
extern const char* archive_extensions[];

bool has_extension(const char *filepath, const char **extensions);

#endif
