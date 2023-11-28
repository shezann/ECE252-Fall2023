#pragma once

#include <pthread.h>
#include <stdio.h>

#include "queue.h"
#include "hashmap.h"

/**
 * @brief Breadth-first search tree with multithreading
 */

#ifndef min
#define min(a,b) \
    ({ __typeof__ (a) _a = (a); \
        __typeof__ (b) _b = (b); \
        _a < _b ? _a : _b; })
#endif

typedef struct search_return_t {
    void** children;
    int n_children;

    char* effective_url;
} search_return_t;

typedef struct _bfs_t {
    queue_t frontier;
    queue_t search_results;
    hashmap_t visited;

    search_return_t* (*work)(void*);
    size_t n_threads;
    size_t waiting_threads;
    size_t max_results;

    int work_done;

    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
} bfs_t;

void bfs_init(bfs_t* bfs, size_t n_threads, size_t max_results, search_return_t* (*work)(void*));

void bfs_cleanup(bfs_t* bfs);

void* _bfs_concurrent_search(bfs_t* bfs);

queue_t* bfs_start(bfs_t* bfs, const char* root);