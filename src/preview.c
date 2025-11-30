#include "preview.h"
#include "platform.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <windows.h>

fq_file_type_t detect_file_type(const char *filepath) {
    if (!filepath) return FQ_FILE_TYPE_UNKNOWN;

    if (has_extension(filepath, text_extensions)) {
        return FQ_FILE_TYPE_TEXT;
    }
    if (has_extension(filepath, image_extensions)) {
        return FQ_FILE_TYPE_IMAGE;
    }
    if (has_extension(filepath, video_extensions)) {
        return FQ_FILE_TYPE_VIDEO;
    }
    if (has_extension(filepath, audio_extensions)) {
        return FQ_FILE_TYPE_AUDIO;
    }
    if (has_extension(filepath, archive_extensions)) {
        return FQ_FILE_TYPE_ARCHIVE;
    }

    if (is_text_file(filepath)) {
        return FQ_FILE_TYPE_TEXT;
    }

    return FQ_FILE_TYPE_UNKNOWN;
}

const char* file_type_to_string(fq_file_type_t type) {
    switch (type) {
        case FQ_FILE_TYPE_TEXT: return "Text";
        case FQ_FILE_TYPE_IMAGE: return "Image";
        case FQ_FILE_TYPE_VIDEO: return "Video";
        case FQ_FILE_TYPE_AUDIO: return "Audio";
        case FQ_FILE_TYPE_ARCHIVE: return "Archive";
        default: return "Unknown";
    }
}

bool is_text_file(const char *filepath) {
    FILE *file = fopen(filepath, "rb");
    if (!file) return false;

    unsigned char buffer[512];
    size_t bytes_read = fread(buffer, 1, sizeof(buffer), file);
    fclose(file);

    if (bytes_read == 0) return true;

    if (bytes_read >= 3 && buffer[0] == 0xEF && buffer[1] == 0xBB && buffer[2] == 0xBF) {
        return true;
    }

    if (bytes_read >= 2 && ((buffer[0] == 0xFF && buffer[1] == 0xFE) ||
                           (buffer[0] == 0xFE && buffer[1] == 0xFF))) {
        return false;
    }

    for (size_t i = 0; i < bytes_read; i++) {
        if (buffer[i] == 0) return false;
    }

    size_t printable = 0;
    for (size_t i = 0; i < bytes_read; i++) {
        if (isprint(buffer[i]) || isspace(buffer[i])) {
            printable++;
        }
    }

    return (printable * 100 / bytes_read) >= 95;
}

int preview_text_file(const char *filepath, size_t max_lines, FILE *output) {
    FILE *file = fopen(filepath, "r");
    if (!file) {
        fprintf(output, "  [Error: Cannot open file]\n");
        return -1;
    }

    char line[1024];
    size_t line_count = 0;
    bool has_more = false;

    while (line_count < max_lines && fgets(line, sizeof(line), file)) {
        size_t len = strlen(line);
        if (len > 0 && line[len-1] == '\n') {
            line[len-1] = '\0';
        }

        if (strlen(line) > 100) {
            line[97] = '.';
            line[98] = '.';
            line[99] = '.';
            line[100] = '\0';
        }

        fprintf(output, "  %s\n", line);
        line_count++;
    }

    if (fgets(line, sizeof(line), file)) {
        has_more = true;
    }

    fclose(file);

    if (has_more) {
        fprintf(output, "  [...more lines...]\n");
    } else if (line_count == 0) {
        fprintf(output, "  [Empty file]\n");
    }

    return 0;
}

int preview_file_summary(const char *filepath, FILE *output) {
    fq_file_type_t type = detect_file_type(filepath);

    HANDLE hFile = CreateFileA(filepath, GENERIC_READ, FILE_SHARE_READ, NULL,
                              OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        fprintf(output, "  [Error: Cannot access file]\n");
        return -1;
    }

    LARGE_INTEGER fileSize;
    if (!GetFileSizeEx(hFile, &fileSize)) {
        CloseHandle(hFile);
        fprintf(output, "  [Error: Cannot get file size]\n");
        return -1;
    }

    CloseHandle(hFile);

    char size_str[64];
    if (fileSize.QuadPart < 1024) {
        snprintf(size_str, sizeof(size_str), "%lld bytes", fileSize.QuadPart);
    } else if (fileSize.QuadPart < 1024 * 1024) {
        snprintf(size_str, sizeof(size_str), "%.1f KB", fileSize.QuadPart / 1024.0);
    } else if (fileSize.QuadPart < 1024 * 1024 * 1024) {
        snprintf(size_str, sizeof(size_str), "%.1f MB", fileSize.QuadPart / (1024.0 * 1024.0));
    } else {
        snprintf(size_str, sizeof(size_str), "%.1f GB", fileSize.QuadPart / (1024.0 * 1024.0 * 1024.0));
    }

    fprintf(output, "  Type: %s, Size: %s\n", file_type_to_string(type), size_str);

    return 0;
}
