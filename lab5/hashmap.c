#include "hashmap.h"

#include <string.h>

void hashmap_init(hashmap_t *map, size_t n_buckets) {
    map->n_buckets = n_buckets;
    map->buckets = malloc(sizeof(node_t*) * map->n_buckets);

    for (int i = 0; i < map->n_buckets; i++) {
        map->buckets[i] = NULL;
    }
}  

void hashmap_cleanup(hashmap_t *map) {
    for (int i = 0; i < map->n_buckets; i++) {
        node_t *curr = map->buckets[i];
        while (curr != NULL) {
            node_t *next = curr->next;
            if (curr->value != NULL)
                free(curr->value);
            free(curr);
            curr = next;
        }
    }
    free(map->buckets);
}

size_t _hash(size_t n_buckets, const char* value) {
    size_t hash = 5381; // djb2 hash function
    int ch;
    while ((ch = *value++)) {
        hash = ((hash << 5) + hash) + ch;
    }
    return hash % n_buckets;
}

void hashmap_put(hashmap_t *map, const char* value) {
    if (hashmap_contains(map, value) == 1) {
        return;
    }

    // Insert new node at head of list
    size_t hash = _hash(map->n_buckets, (char*) value);

    map->size += 1;
    node_t *new_node = malloc(sizeof(node_t));
    new_node->value = malloc(strlen(value) + 1); // Make a copy of the string
    strcpy(new_node->value, value);
    new_node->next = map->buckets[hash];
    map->buckets[hash] = new_node;
}

int hashmap_contains(hashmap_t *map, const char* value) {
    size_t hash = _hash(map->n_buckets, (char*) value);

    node_t *curr = map->buckets[hash];
    while (curr != NULL) {
        if (strcmp(curr->value, value) == 0) {
            return 1;
        }
        curr = curr->next;
    }

    return 0;
}

void hashmap_remove(hashmap_t *map, const char* value) {
    size_t hash = _hash(map->n_buckets, (char*) value);

    node_t *curr = map->buckets[hash];
    node_t *prev = NULL;
    while (curr != NULL) {
        if (strcmp(curr->value, value) == 0) {
            if (prev == NULL) {
                map->buckets[hash] = curr->next;
            } else {
                prev->next = curr->next;
            }
            free(curr->value);
            free(curr);
            map->size -= 1;
            return;
        }
        prev = curr;
        curr = curr->next;
    }

}

size_t hashmap_size(hashmap_t *map) {
    return map->size;
}