#include <pthread.h>
#include <stdio.h>

#include "queue.h"
#include "hashmap.h"

/**
 * @brief Breadth-first search tree with multithreading
 */

typedef struct search_return_t {
    void** children;
    char** search_results;
    int n_children;
    int n_search_results;
} search_return_t;

typedef struct _bfs_t {
    queue_t queue;
    hashmap_t visited;

    queue_t search_results;

    search_return_t* (*work)(void*);
    size_t n_threads;
    size_t waiting_threads;

    int work_done;

    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
} bfs_t;

void bfs_init(bfs_t* bfs, size_t n_threads, search_return_t* (*work)(void*));

void bfs_cleanup(bfs_t* bfs);

void* _bfs_concurrent_search(bfs_t* bfs);

queue_t* bfs_start(bfs_t* bfs, char* root);