#include "thread_pool.h"
#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

typedef struct work_item {
    work_function_t work_func;
    void *user_data;
    struct work_item *next;
} work_item_t;

struct thread_pool {
    HANDLE *threads;
    size_t thread_count;

    CRITICAL_SECTION queue_lock;
    work_item_t *queue_head;
    work_item_t *queue_tail;
    HANDLE work_semaphore;

    atomic_size_t active_work_items;
    atomic_size_t completed_work_items;
    atomic_size_t total_submitted;
    atomic_size_t queued_work_items;

    thread_pool_config_t config;
};

static DWORD WINAPI thread_pool_worker(LPVOID param) {
    thread_pool_t *pool = (thread_pool_t*)param;

    for (;;) {
        if (pool->config.stop_flag && atomic_load(pool->config.stop_flag)) {
            break;
        }

        DWORD wait_result = WaitForSingleObject(pool->work_semaphore, 100);
        if (wait_result == WAIT_TIMEOUT) {
            continue;
        }
        if (wait_result != WAIT_OBJECT_0) {
            break;
        }

        work_item_t *item = NULL;
        EnterCriticalSection(&pool->queue_lock);
        if (pool->queue_head) {
            item = pool->queue_head;
            pool->queue_head = item->next;
            if (!pool->queue_head) {
                pool->queue_tail = NULL;
            }
        }
        LeaveCriticalSection(&pool->queue_lock);

        if (!item) {
            continue;
        }

        atomic_fetch_sub(&pool->queued_work_items, 1);
        atomic_fetch_add(&pool->active_work_items, 1);

        item->work_func(NULL, item->user_data);

        atomic_fetch_add(&pool->completed_work_items, 1);
        atomic_fetch_sub(&pool->active_work_items, 1);

        free(item);
    }

    return 0;
}

thread_pool_t* thread_pool_create(const thread_pool_config_t *config) {
    if (!config) return NULL;

    thread_pool_t *pool = (thread_pool_t*)calloc(1, sizeof(thread_pool_t));
    if (!pool) return NULL;

    pool->config = *config;

    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    size_t hw_threads = sysinfo.dwNumberOfProcessors > 0 ? sysinfo.dwNumberOfProcessors : 4;
    pool->thread_count = config->max_threads > 0 ? config->max_threads : hw_threads;

    atomic_init(&pool->active_work_items, 0);
    atomic_init(&pool->completed_work_items, 0);
    atomic_init(&pool->total_submitted, 0);
    atomic_init(&pool->queued_work_items, 0);

    InitializeCriticalSection(&pool->queue_lock);
    pool->work_semaphore = CreateSemaphoreA(NULL, 0, LONG_MAX, NULL);
    if (!pool->work_semaphore) {
        DeleteCriticalSection(&pool->queue_lock);
        free(pool);
        return NULL;
    }

    pool->threads = (HANDLE*)calloc(pool->thread_count, sizeof(HANDLE));
    if (!pool->threads) {
        CloseHandle(pool->work_semaphore);
        DeleteCriticalSection(&pool->queue_lock);
        free(pool);
        return NULL;
    }

    for (size_t i = 0; i < pool->thread_count; i++) {
        pool->threads[i] = CreateThread(NULL, 0, thread_pool_worker, pool, 0, NULL);
        if (!pool->threads[i]) {
            pool->thread_count = i;
            break;
        }
    }

    if (pool->thread_count == 0) {
        CloseHandle(pool->work_semaphore);
        DeleteCriticalSection(&pool->queue_lock);
        free(pool->threads);
        free(pool);
        return NULL;
    }

    return pool;
}

bool thread_pool_submit(thread_pool_t *pool, work_function_t work_func, void *user_data) {
    if (!pool || !work_func) return false;

    if (pool->config.stop_flag && atomic_load(pool->config.stop_flag)) {
        return false;
    }

    work_item_t *item = (work_item_t*)malloc(sizeof(work_item_t));
    if (!item) return false;

    item->work_func = work_func;
    item->user_data = user_data;
    item->next = NULL;

    EnterCriticalSection(&pool->queue_lock);
    if (pool->queue_tail) {
        pool->queue_tail->next = item;
        pool->queue_tail = item;
    } else {
        pool->queue_head = pool->queue_tail = item;
    }
    LeaveCriticalSection(&pool->queue_lock);

    atomic_fetch_add(&pool->queued_work_items, 1);
    atomic_fetch_add(&pool->total_submitted, 1);
    ReleaseSemaphore(pool->work_semaphore, 1, NULL);

    return true;
}

bool thread_pool_wait_completion(thread_pool_t *pool, DWORD timeout_ms) {
    if (!pool) return false;

    DWORD start = GetTickCount();
    for (;;) {
        size_t active = atomic_load(&pool->active_work_items);
        size_t queued = atomic_load(&pool->queued_work_items);

        if (active == 0 && queued == 0) {
            return true;
        }

        if (timeout_ms != INFINITE) {
            DWORD elapsed = GetTickCount() - start;
            if (elapsed >= timeout_ms) {
                return false;
            }
        }

        if (pool->config.progress_cb) {
            size_t completed = atomic_load(&pool->completed_work_items);
            if (!pool->config.progress_cb(completed, active, pool->config.progress_user_data)) {
                if (pool->config.stop_flag) {
                    atomic_store(pool->config.stop_flag, true);
                }
                return false;
            }
        }

        Sleep(10);
    }
}

void thread_pool_destroy(thread_pool_t *pool) {
    if (!pool) return;

    if (pool->config.stop_flag) {
        atomic_store(pool->config.stop_flag, true);
    }

    for (size_t i = 0; i < pool->thread_count; i++) {
        ReleaseSemaphore(pool->work_semaphore, 1, NULL);
    }

    WaitForMultipleObjects((DWORD)pool->thread_count, pool->threads, TRUE, 5000);

    for (size_t i = 0; i < pool->thread_count; i++) {
        if (pool->threads[i]) {
            CloseHandle(pool->threads[i]);
        }
    }

    CloseHandle(pool->work_semaphore);
    DeleteCriticalSection(&pool->queue_lock);

    work_item_t *item = pool->queue_head;
    while (item) {
        work_item_t *next = item->next;
        free(item);
        item = next;
    }

    free(pool->threads);
    free(pool);
}

bool thread_pool_get_stats(thread_pool_t *pool, thread_pool_stats_t *stats) {
    if (!pool || !stats) return false;

    stats->active_threads = atomic_load(&pool->active_work_items);
    stats->queued_work_items = atomic_load(&pool->queued_work_items);
    stats->completed_work_items = atomic_load(&pool->completed_work_items);
    stats->total_submitted = atomic_load(&pool->total_submitted);

    return true;
}
