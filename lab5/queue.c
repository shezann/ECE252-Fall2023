#include "queue.h"

void queue_init(queue_t *q) {
    q->head = NULL;
    q->tail = NULL;
    q->size = 0;
}

void queue_cleanup(queue_t *q) {
    node_t *curr = q->head;
    while (curr != NULL) {
        node_t *next = curr->next;
        free(curr->value);
        free(curr);
        curr = next;
    }
}

void queue_enqueue(queue_t *q, const char* value) {
    q->size += 1;
    node_t *new_node = malloc(sizeof(node_t));
    new_node->value = malloc(strlen(value) + 1); // Make a copy of the data
    strcpy(new_node->value, value);
    new_node->next = NULL;

    if (q->head == NULL) {
        q->head = new_node;
        q->tail = new_node;
    } else {
        q->tail->next = new_node;
        q->tail = new_node;
    }
}

char* queue_dequeue(queue_t *q) {
    if (q->head == NULL) {
        return NULL;
    }

    q->size -= 1;

    node_t *head = q->head;
    q->head = q->head->next;
    if (q->head == NULL) {
        q->tail = NULL;
    }
    char* value = head->value;
    free(head);

    return value;
}

const char* queue_peek(queue_t *q) {
    if (q->head == NULL) {
        return NULL;
    }

    const char* value = q->head->value;

    return value;
}

size_t queue_size(queue_t *q) {
    return q->size;
}

int queue_is_empty(queue_t *q) {
    return q->size == 0;
}

char** queue_to_array(queue_t *q) {
    char** array = malloc(sizeof(char*) * q->size);

    node_t *curr = q->head;
    for (int i = 0; i < q->size; i++) {
        array[i] = malloc(strlen(curr->value) + 1);
        strcpy(array[i], curr->value);
        curr = curr->next;
    }

    return array;
}

void queue_clear(queue_t *q) {
    node_t *curr = q->head;
    while (curr != NULL) {
        node_t *next = curr->next;
        free(curr->value);
        free(curr);
        curr = next;
    }
    q->head = NULL;
    q->tail = NULL;
    q->size = 0;
}