#ifndef PREVIEW_H
#define PREVIEW_H

#include <stdio.h>
#include <stdbool.h>
#include <stddef.h>

typedef enum {
    FQ_FILE_TYPE_TEXT,
    FQ_FILE_TYPE_IMAGE,
    FQ_FILE_TYPE_VIDEO,
    FQ_FILE_TYPE_AUDIO,
    FQ_FILE_TYPE_ARCHIVE,
    FQ_FILE_TYPE_UNKNOWN
} fq_file_type_t;

int preview_text_file(const char *filepath, size_t max_lines, FILE *output);

fq_file_type_t detect_file_type(const char *filepath);

const char* file_type_to_string(fq_file_type_t type);

bool is_text_file(const char *filepath);

int preview_file_summary(const char *filepath, FILE *output);

#endif
