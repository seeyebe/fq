#include "compat.h"
#include "search.h"
#include "cli.h"
#include "utils.h"
#include "criteria.h"
#include "output.h"
#include "pattern.h"
#include "platform.h"
#include "preview.h"
#include "thread_pool.h"
#include "version.h"
#include "regex/re.h"
#include "regex/regex.h"
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdatomic.h>


#include <time.h>

typedef struct {
    cli_options_t *options;
    search_criteria_t *criteria;
    time_t start_time;
    int progress_shown;
    size_t last_processed;
    size_t last_results;
} streamed_state_t;

static bool streamed_result_callback(const search_result_t *result, void *user_data) {
    streamed_state_t *state = (streamed_state_t*)user_data;
    if (state->options->json_output) {
        return true;
    }
    if (state->criteria->preview_mode) {
        rq_file_type_t type = detect_file_type(result->path);
        printf("%s\n", result->path);
        if (type == RQ_FILE_TYPE_TEXT) {
            preview_text_file(result->path, state->criteria->preview_lines, stdout);
        } else {
            preview_file_summary(result->path, stdout);
        }
        printf("\n");
    } else {
        printf("%s\n", result->path);
    }
    fflush(stdout);
    return true;
}

static bool streamed_progress_callback(size_t processed_files, size_t queued_dirs, size_t total_results, void *user_data) {
    (void)queued_dirs;
    streamed_state_t *state = (streamed_state_t*)user_data;
    time_t now = time(NULL);
    if (!state->progress_shown && total_results == 0 && !state->options->show_stats) {
        if (difftime(now, state->start_time) >= 5.0) {
            fprintf(stderr, "Processed: %zu files, Found: %zu results...\n", processed_files, total_results);
            state->progress_shown = 1;
        }
    }
    state->last_processed = processed_files;
    state->last_results = total_results;
    return true;
}

int main(int argc, char *argv[]) {
    search_criteria_t criteria;
    cli_options_t options = {0};
    search_result_t *results = NULL;
    size_t result_count = 0;
    int exit_code = 0;

    if (parse_command_line(argc, argv, &criteria, &options) != 0) {
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
        fprintf(stderr, "Error: Root directory '%s' does not exist or cannot be accessed\n", criteria.root_path);
        exit_code = 1;
        goto cleanup;
    }
    platform_closedir(test_dir);

    fprintf(stderr, "Searching in '%s' for '%s'...\n", criteria.root_path, criteria.search_term ? criteria.search_term : "*");


    streamed_state_t stream_state = {0};
    stream_state.options = &options;
    stream_state.criteria = &criteria;
    stream_state.start_time = time(NULL);
    stream_state.progress_shown = 0;
    stream_state.last_processed = 0;
    stream_state.last_results = 0;

    int search_result = search_files_advanced(&criteria, &results, &result_count,
        streamed_result_callback, &stream_state,
        streamed_progress_callback, &stream_state);

    fprintf(stderr, "\n");

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
    } else {
        if (result_count > 0) {
            fprintf(stderr, "Found %zu results.\n", result_count);
        } else {
            fprintf(stderr, "No results found.\n");
        }
    }

cleanup:
    free_search_results(results);
    criteria_cleanup(&criteria);
    return exit_code;
}
