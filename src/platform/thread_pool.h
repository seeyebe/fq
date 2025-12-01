#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include "compat.h"
#include <windows.h>
#include <stdbool.h>
#include <stddef.h>


typedef struct thread_pool thread_pool_t;

typedef void (*work_function_t)(void *context, void *user_data);

typedef bool (*progress_callback_t)(size_t processed_files, size_t queued_dirs, void *user_data);

typedef struct {
    size_t max_threads;
    size_t queue_size_hint;
    progress_callback_t progress_cb;
    void *progress_user_data;
    atomic_bool *stop_flag;
} thread_pool_config_t;

// Create a new thread pool with the given config
thread_pool_t* thread_pool_create(const thread_pool_config_t *config);

// Submit work to be done - returns false if queue is full or pool is shutting down
bool thread_pool_submit(thread_pool_t *pool, work_function_t work_func, void *user_data);

// Wait for all work to finish, with optional timeout
bool thread_pool_wait_completion(thread_pool_t *pool, DWORD timeout_ms);

// Clean up and destroy the pool
void thread_pool_destroy(thread_pool_t *pool);

typedef struct {
    size_t active_threads;
    size_t queued_work_items;
    size_t completed_work_items;
    size_t total_submitted;
} thread_pool_stats_t;

bool thread_pool_get_stats(thread_pool_t *pool, thread_pool_stats_t *stats);

#endif