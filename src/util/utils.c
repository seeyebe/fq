#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// File extension arrays for type detection
const char* text_extensions[] = {
    "txt", "md", "c", "cpp", "h", "hpp", "cs", "java", "py", "js", "ts", "html", "htm",
    "css", "xml", "json", "yaml", "yml", "ini", "cfg", "conf", "log", "sql", "sh", "bat",
    "ps1", "php", "rb", "go", "rs", "kt", "scala", "pl", "r", "matlab", "tex", "rtf", NULL
};

const char* image_extensions[] = {
    "jpg", "jpeg", "png", "gif", "bmp", "tiff", "tif", "svg", "webp", "ico", "psd", "ai", NULL
};

const char* video_extensions[] = {
    "mp4", "avi", "mkv", "mov", "wmv", "flv", "webm", "m4v", "3gp", "mpg", "mpeg", NULL
};

const char* audio_extensions[] = {
    "mp3", "wav", "flac", "aac", "ogg", "wma", "m4a", "opus", "aiff", NULL
};

const char* archive_extensions[] = {
    "zip", "rar", "7z", "tar", "gz", "bz2", "xz", "cab", "msi", "deb", "rpm", NULL
};

bool has_extension(const char *filepath, const char **extensions) {
    const char *ext = strrchr(filepath, '.');
    if (!ext) return false;

    ext++;

    for (int i = 0; extensions[i] != NULL; i++) {
        if (_stricmp(ext, extensions[i]) == 0) {
            return true;
        }
    }
    return false;
}

int parse_size_arg(const char *arg, uint64_t *size) {
    if (!arg || !size) return -1;

    char *endptr;
    *size = strtoull(arg, &endptr, 10);

    if (*endptr != '\0') {
        uint64_t multiplier = 1;
        switch (*endptr) {
            case 'K': case 'k': multiplier = 1024; break;
            case 'M': case 'm': multiplier = 1024 * 1024; break;
            case 'G': case 'g': multiplier = 1024 * 1024 * 1024; break;
            case 'T': case 't': multiplier = 1024ULL * 1024 * 1024 * 1024; break;
            default: return -1;
        }
        *size *= multiplier;
    }

    return 0;
}

int parse_size_with_operator(const char *arg, uint64_t *size, char *operator) {
    if (!arg || !size || !operator) return -1;

    *operator = '=';
    const char *size_str = arg;

    if (arg[0] == '+') {
        *operator = '+';
        size_str = arg + 1;
    } else if (arg[0] == '-') {
        *operator = '-';
        size_str = arg + 1;
    }

    return parse_size_arg(size_str, size);
}


int parse_date_string(const char *date_str, FILETIME *file_time) {
    if (!date_str || !file_time) {
        return -1;
    }

    int year, month, day;
    if (sscanf(date_str, "%d-%d-%d", &year, &month, &day) != 3) {
        return -1;
    }

    if (year < 1601 || year > 3000 || month < 1 || month > 12 || day < 1 || day > 31) {
        return -1;
    }

    SYSTEMTIME st = {0};
    st.wYear = year;
    st.wMonth = month;
    st.wDay = day;
    st.wHour = 0;
    st.wMinute = 0;
    st.wSecond = 0;
    st.wMilliseconds = 0;

    if (!SystemTimeToFileTime(&st, file_time)) {
        return -1;
    }

    return 0;
}

void format_filetime_iso(const FILETIME *file_time, char *buffer, size_t buffer_size) {
    if (!file_time || !buffer || buffer_size < 20) {
        return;
    }

    SYSTEMTIME st;
    if (!FileTimeToSystemTime(file_time, &st)) {
        strcpy(buffer, "invalid-date");
        return;
    }

    snprintf(buffer, buffer_size, "%04d-%02d-%02dT%02d:%02d:%02d",
             st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
}