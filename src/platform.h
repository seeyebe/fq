#ifndef PLATFORM_H
#define PLATFORM_H

#include "compat.h"
#include <windows.h>
#include <stdbool.h>
#include <stdint.h>
#include <strsafe.h>

typedef struct {
    HANDLE handle;
    bool valid;
} auto_handle_t;

static inline auto_handle_t auto_handle_init(HANDLE h) {
    auto_handle_t ah = { h, h != NULL && h != INVALID_HANDLE_VALUE };
    return ah;
}

static inline void auto_handle_close(auto_handle_t *ah) {
    if (ah && ah->valid && ah->handle != NULL && ah->handle != INVALID_HANDLE_VALUE) {
        CloseHandle(ah->handle);
        ah->handle = NULL;
        ah->valid = false;
    }
}

static inline HRESULT safe_strcpy(char *dest, size_t dest_size, const char *src) {
    return StringCchCopyA(dest, dest_size, src);
}

static inline HRESULT safe_strcat(char *dest, size_t dest_size, const char *src) {
    return StringCchCatA(dest, dest_size, src);
}

int utf8_to_wide(const char *utf8_str, wchar_t **wide_str);
int wide_to_utf8(const wchar_t *wide_str, char **utf8_str);
void free_converted_string(void *str);

HRESULT make_long_path(const char *path, wchar_t **long_path);

typedef struct platform_dir_iter platform_dir_iter_t;
typedef struct {
    char *name;
    wchar_t *name_wide;
    uint64_t size;
    FILETIME mtime;
    bool is_directory;
    bool is_symlink;
} platform_file_info_t;

platform_dir_iter_t* platform_opendir(const char *utf8_path);
bool platform_readdir(platform_dir_iter_t *iter, platform_file_info_t *info);
void platform_closedir(platform_dir_iter_t *iter);
void platform_free_file_info(platform_file_info_t *info);

#endif