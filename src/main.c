#include "platform/compat.h"
#include "core/search.h"
#include "cli/cli.h"
#include "util/utils.h"
#include "core/criteria.h"
#include "output/output.h"
#include "core/pattern.h"
#include "platform/platform.h"
#include "output/preview.h"
#include "platform/thread_pool.h"
#include "cli/version.h"
#include "regex/re.h"
#include "regex/regex.h"
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdatomic.h>
#include <windows.h>
#include <shellapi.h>

#ifndef _MSC_VER
int _fileno(FILE *);
#endif


#include <time.h>

#define OUTPUT_BUFFER_SIZE 65536
static char g_output_buffer[OUTPUT_BUFFER_SIZE];
static size_t g_output_pos = 0;
static size_t g_results_since_flush = 0;
#define FLUSH_THRESHOLD 64

static void output_flush(void) {
    if (g_output_pos > 0) {
        fwrite(g_output_buffer, 1, g_output_pos, stdout);
        g_output_pos = 0;
    }
}

static void output_write(const char *str, size_t len) {
    if (g_output_pos + len > OUTPUT_BUFFER_SIZE) {
        output_flush();
    }
    // If single write is larger than buffer, write directly
    if (len > OUTPUT_BUFFER_SIZE) {
        fwrite(str, 1, len, stdout);
        return;
    }
    memcpy(g_output_buffer + g_output_pos, str, len);
    g_output_pos += len;
}

static void output_puts(const char *str) {
    output_write(str, strlen(str));
}

typedef struct {
    cli_options_t *options;
    search_criteria_t *criteria;
    time_t start_time;
    int progress_shown;
    size_t last_processed;
    size_t last_results;
    bool use_color;
} streamed_state_t;

static bool enable_vt_mode(void) {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE || hOut == NULL) return false;
    DWORD mode = 0;
    if (!GetConsoleMode(hOut, &mode)) return false;
    #ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
        #define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
    #endif
    if (mode & ENABLE_VIRTUAL_TERMINAL_PROCESSING) return true;
    return SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
}

static bool should_use_color(const cli_options_t *options) {
    if (!options) return false;
    if (options->json_output || options->output_file) return false;
    switch (options->color_mode) {
        case COLOR_ALWAYS:
            return true;
        case COLOR_NEVER:
            return false;
        case COLOR_AUTO:
        default:
            return _isatty(_fileno(stdout)) != 0;
    }
}

static void print_path_colored(const search_result_t *result, bool use_color) {
    if (!result) return;

    if (!use_color) {
        output_puts(result->path);
        output_write("\n", 1);
        return;
    }

    const char *color = result->is_directory ? "\x1b[36m" : "\x1b[32m";
    output_puts(color);
    output_puts(result->path);
    output_puts("\x1b[0m\n");
}

static char** convert_wargv_to_utf8(int argc, wchar_t *wargv[]) {
    if (argc <= 0) return NULL;
    char **argv = (char**)calloc((size_t)argc, sizeof(char*));
    if (!argv) return NULL;

    for (int i = 0; i < argc; i++) {
        if (wide_to_utf8(wargv[i], &argv[i]) < 0) {
            for (int j = 0; j < i; j++) {
                free(argv[j]);
            }
            free(argv);
            return NULL;
        }
    }
    return argv;
}

static void free_utf8_argv(int argc, char **argv) {
    if (!argv) return;
    for (int i = 0; i < argc; i++) {
        free(argv[i]);
    }
    free(argv);
}

static bool streamed_result_callback(const search_result_t *result, void *user_data) {
    streamed_state_t *state = (streamed_state_t*)user_data;
    if (state->options->json_output) {
        return true;
    }
    if (state->criteria->preview_mode) {
        // Preview mode needs immediate output
        output_flush();
        print_path_colored(result, state->use_color);
        output_flush();
        if (!result->is_directory) {
            fq_file_type_t type = detect_file_type(result->path);
            if (type == FQ_FILE_TYPE_TEXT) {
                preview_text_file(result->path, state->criteria->preview_lines, stdout);
            } else {
                preview_file_summary(result->path, stdout);
            }
        } else {
            fprintf(stdout, "  [Directory]\n");
        }
        printf("\n");
        fflush(stdout);
    } else {
        print_path_colored(result, state->use_color);
        g_results_since_flush++;
        if (g_results_since_flush >= FLUSH_THRESHOLD) {
            output_flush();
            g_results_since_flush = 0;
        }
    }
    return true;
}

static bool streamed_progress_callback(size_t processed_files, size_t queued_dirs, size_t total_results, void *user_data) {
    (void)queued_dirs;
    streamed_state_t *state = (streamed_state_t*)user_data;
    time_t now = time(NULL);
    if (!state->progress_shown && total_results == 0 && !state->options->show_stats && !state->options->quiet) {
        if (difftime(now, state->start_time) >= 5.0) {
            fprintf(stderr, "Processed: %zu files, Found: %zu results...\n", processed_files, total_results);
            state->progress_shown = 1;
        }
    }
    state->last_processed = processed_files;
    state->last_results = total_results;
    return true;
}

int main(int argc, char *argv_placeholder[]) {
    (void)argc;
    search_criteria_t criteria;
    cli_options_t options = {0};
    search_result_t *results = NULL;
    size_t result_count = 0;
    int exit_code = 0;
    (void)argv_placeholder;

    int wargc = 0;
    wchar_t **wargv = CommandLineToArgvW(GetCommandLineW(), &wargc);
    if (!wargv) {
        fprintf(stderr, "Error: failed to parse command line\n");
        return 1;
    }

    char **argv = convert_wargv_to_utf8(wargc, wargv);

    if (!argv) {
        fprintf(stderr, "Error: argument conversion failed\n");
        LocalFree(wargv);
        return 1;
    }

    if (parse_command_line(wargc, argv, &criteria, &options) != 0) {
        if (!options.show_help && !options.show_version) {
            fprintf(stderr, "Error: Invalid command line arguments\n");
            exit_code = 1;
        }
        goto cleanup;
    }

    if (options.show_help) {
        print_usage(argv[0]);
        goto cleanup;
    }

    if (options.show_version) {
        print_version();
        goto cleanup;
    }

    if (!criteria_validate(&criteria)) {
        fprintf(stderr, "Error: Invalid search criteria\n");
        exit_code = 1;
        goto cleanup;
    }

    platform_dir_iter_t *test_dir = platform_opendir(criteria.root_path);
    if (!test_dir) {
        fprintf(stderr, "Error: '%s': No such file or directory\n", criteria.root_path);
        exit_code = 1;
        goto cleanup;
    }
    platform_closedir(test_dir);

    streamed_state_t stream_state = {0};
    stream_state.options = &options;
    stream_state.criteria = &criteria;
    stream_state.start_time = time(NULL);
    stream_state.progress_shown = 0;
    stream_state.last_processed = 0;
    stream_state.last_results = 0;
    stream_state.use_color = false;

    bool color_ok = should_use_color(&options);
    if (color_ok) {
        color_ok = enable_vt_mode();
    }
    stream_state.use_color = color_ok;

    int search_result = search_files_advanced(&criteria, &results, &result_count,
        streamed_result_callback, &stream_state,
        streamed_progress_callback, &stream_state);

    // Final flush of any remaining buffered output
    output_flush();

    if (search_result == -2) {
        fprintf(stderr,
                "Warning: Search timed out after %" PRIu64 " ms\n",
                (uint64_t)criteria.timeout_ms);
    } else if (search_result != 0) {
        fprintf(stderr, "Error: Search operation failed\n");
        exit_code = 1;
        goto cleanup;
    }

    if (options.json_output) {
        if (output_results(results, result_count, &options, &criteria) != 0) {
            fprintf(stderr, "Error: Failed to output results\n");
            exit_code = 1;
            goto cleanup;
        }
    }
    // No summary output like fd - just silent

cleanup:
    free_search_results(results);
    criteria_cleanup(&criteria);
    free_utf8_argv(wargc, argv);
    LocalFree(wargv);
    return exit_code;
}
