#pragma once

#include <pthread.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief This is a queue data structure that is NOT thread-safe and is implemented using a double linked list.
*/

typedef struct _node_t{
    char* value;
    struct _node_t *next;
} node_t;

typedef struct _queue_t {
    node_t *head;
    node_t *tail;
    size_t size;
} queue_t;

/**
 * @brief Initializes a queue with a given capacity.
 * 
 * @param q The queue to initialize.
*/
void queue_init(queue_t *q);

void queue_cleanup(queue_t *q);

/**
 * @brief Enqueues a value into the queue.
 * 
 * @param q The queue to enqueue into.
 * @param value The value to enqueue.
*/
void queue_enqueue(queue_t *q, const char* value);

/**
 * @brief Dequeues a value from the queue.
 * 
 * @param q The queue to dequeue from.
 * 
 * @return The value dequeued from the queue.
*/
char* queue_dequeue(queue_t *q);

/**
 * @brief Returns the size of the queue.
 * 
 * @param q The queue to get the size of.
*/
size_t queue_size(queue_t *q);

int queue_is_empty(queue_t *q);

/**
 * @brief Returns the value at the head of the queue.
 * 
 * @param q The queue to peek into.
*/
const char* queue_peek(queue_t *q);

/**
 * Cleans up the queue.
 * 
 * @param q The queue to clean up.
*/
void queue_cleanup(queue_t *q);

char** queue_to_array(queue_t *q);