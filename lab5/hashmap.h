#pragma once

#include <pthread.h>
#include <stdlib.h>

#include "queue.h"

typedef struct _hashmap_t {
    node_t** buckets;
    size_t n_buckets;
    size_t size;
} hashmap_t;

size_t _hash(size_t n_buckets, const char* value);

void hashmap_init(hashmap_t *map, size_t n_buckets);

void hashmap_put(hashmap_t *map, const char* value);

size_t hashmap_size(hashmap_t *map);

int hashmap_contains(hashmap_t *map, const char* value);

void hashmap_remove(hashmap_t *map, const char* value);

void hashmap_cleanup(hashmap_t *map);