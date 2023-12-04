#pragma once

#include <stdio.h>
#include <curl/multi.h>

#include "queue.h"
#include "hashmap.h"
#include "web_crawler.h"

/**
 * @brief Breadth-first search tree with multithreading
 */

#ifndef min
#define min(a,b) \
    ({ __typeof__ (a) _a = (a); \
        __typeof__ (b) _b = (b); \
        _a < _b ? _a : _b; })
#endif

#define MAX_WAIT_MSECS 30*1000 /* Wait max. 30 seconds */

typedef struct _bfs_t {
    queue_t frontier;
    queue_t search_results;
    hashmap_t visited;

    size_t max_conntions;
    size_t max_results;
} bfs_t;

void bfs_init(bfs_t* bfs, size_t max_results, size_t max_connections);

void bfs_cleanup(bfs_t* bfs);

queue_t* bfs_start(bfs_t* bfs, const char* root);