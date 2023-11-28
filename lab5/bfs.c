#include "bfs.h"

void bfs_init(bfs_t* bfs, size_t n_threads, size_t max_results, search_return_t* (*work)(void*)) {
    bfs->work = work;
    bfs->max_results = max_results;
    bfs->waiting_threads = 0;
    bfs->work_done = 0;
    bfs->n_threads = n_threads;

    queue_init(&bfs->frontier);
    queue_init(&bfs->search_results);
    hashmap_init(&bfs->visited, 100);
    
    pthread_mutex_init(&bfs->mutex, NULL);
    pthread_cond_init(&bfs->not_empty, NULL);
}

void bfs_cleanup(bfs_t* bfs) {
    queue_cleanup(&bfs->frontier);
    queue_cleanup(&bfs->search_results);
    hashmap_cleanup(&bfs->visited);

    pthread_mutex_destroy(&bfs->mutex);
    pthread_cond_destroy(&bfs->not_empty);
}

int _process_search_result(bfs_t* bfs , const search_return_t* ret) {
    pthread_mutex_lock(&bfs->mutex);

    if (bfs->work_done == 1) {
        pthread_mutex_unlock(&bfs->mutex);
        return 1;
    }

    // Enqueue all the children
    if (ret->effective_url != NULL) {
        queue_enqueue(&bfs->search_results, ret->effective_url);
        if (queue_size(&bfs->search_results) == bfs->max_results) { 
            // If the maximum number of results is reached, then quit
            bfs->work_done = 1;
            pthread_cond_broadcast(&bfs->not_empty); // Broadcast to release all threads
            pthread_mutex_unlock(&bfs->mutex); 
            return 1;
        }
    } else if (ret->n_children > 0) {
        for (int i = 0; i < ret->n_children; i++) {
            const char* child = ret->children[i];
            if (hashmap_contains(&bfs->visited, child) == 0) { // If the child is not visited, then enqueue it
                hashmap_put(&bfs->visited, child);
                queue_enqueue(&bfs->frontier, child);
            }
        }
        pthread_cond_broadcast(&bfs->not_empty); // Broadcast to release all threads that there is work to be done
    }
    
    pthread_mutex_unlock(&bfs->mutex); 

    return 0;
}

void* _bfs_concurrent_search(bfs_t* bfs) {
    while (1) {
        // Lock the queue mutex to check if the queue is empty
        pthread_mutex_lock(&bfs->mutex);
        
        // If the queue is empty
        if (queue_is_empty(&bfs->frontier)) {
            bfs->waiting_threads++; // Increment the number of waiting threads
            
            if (bfs->waiting_threads == bfs->n_threads) { // If all threads are waiting, then quit this thread
                bfs->work_done = 1;
                pthread_cond_broadcast(&bfs->not_empty); // Broadcast to release all threads
                pthread_mutex_unlock(&bfs->mutex);
                break;
            }
            
            pthread_cond_wait(&bfs->not_empty, &bfs->mutex); // Wait for the queue to be non-empty

            if (bfs->work_done == 1) { // If the work is done, then quit this thread
                
                pthread_mutex_unlock(&bfs->mutex);
                
                break;
            }
            
            bfs->waiting_threads -= 1;
        }
        
        char* node = queue_dequeue(&bfs->frontier);

        if (node == NULL) { // If the queue is empty, then probably another thread dequeued the last element
            pthread_mutex_unlock(&bfs->mutex);
            continue;
        }

        pthread_mutex_unlock(&bfs->mutex); // Unlock the queue mutex as we already have a node to work on

        // Work on the node concurrently
        search_return_t* ret = bfs->work((void*) node);
        
        if (ret != NULL) {

            int should_stop = _process_search_result(bfs, ret);
            if (ret->effective_url != NULL) {
                free(ret->effective_url);
            }
            for (int i = 0; i < ret->n_children; i++) {
                free(ret->children[i]);
            }
            free(ret->children);
            free(ret);

            if (should_stop == 1) {
                free(node);
                return NULL;
            }
        }
        free(node);
    
        // The thread will continue to wait for work to be done

    }

    return NULL;

}

queue_t* bfs_start(bfs_t* bfs, const char* root) {
    if (bfs->n_threads == 0) {
        return NULL;
    }

    // Enqueue the root node
    queue_enqueue(&bfs->frontier, root);
    hashmap_put(&bfs->visited, root);

    pthread_t* threads = malloc(sizeof(pthread_t) * bfs->n_threads);

    for (int i = 0; i < bfs->n_threads; i++) { // Create all the threads
        pthread_create(&threads[i], NULL, (void* (*)(void*)) _bfs_concurrent_search, bfs);
    }

    for (int i = 0; i < bfs->n_threads; i++) { // Wait for all the threads to finish
        pthread_join(threads[i], NULL);
    }

    free(threads);

    // Copy the search results to a new queue
    queue_t* search_results = malloc(sizeof(queue_t));
    queue_init(search_results);

    while (!queue_is_empty(&bfs->search_results)) {
        char* search_result = queue_dequeue(&bfs->search_results);
        queue_enqueue(search_results, search_result);
        free(search_result);
    }

    return search_results;
}