#include "bfs.h"

void bfs_init(bfs_t* bfs, size_t max_results, size_t max_connections) {
    bfs->max_results = max_results;
    bfs->max_conntions = max_connections;

    queue_init(&bfs->frontier);
    queue_init(&bfs->search_results);
    hashmap_init(&bfs->visited, 100);
}

void bfs_cleanup(bfs_t* bfs) {
    queue_cleanup(&bfs->frontier);
    queue_cleanup(&bfs->search_results);
    hashmap_cleanup(&bfs->visited);
}

void _search_one_cycle(bfs_t* bfs, CURLM* curl_multi, size_t* num_png_found) {
    CURLMsg* msg = NULL;
    RECV_BUF* private_recv_buf = NULL;
    int still_running = 0;
    // Transfer
    curl_multi_perform(curl_multi, &still_running);
    
    do {
        int numfds = 0;
        int rc = curl_multi_wait(curl_multi, NULL, 0, MAX_WAIT_MSECS, &numfds);
        if (rc != CURLM_OK) {
            fprintf(stderr, "curl_multi_wait() failed, code %d.\n", rc);
            break;
        }
        curl_multi_perform(curl_multi, &still_running);
    } while (still_running);


    // Process the messages
    int msgs_left = 0;
    while ((msg = curl_multi_info_read(curl_multi, &msgs_left))) {
        if (msg->msg != CURLMSG_DONE) {
            fprintf(stderr, "error: after curl_multi_info_read(), CURLMsg=%d\n", msg->msg);
            continue;
        }

        // int return_code = msg->data.result;
        // if (return_code != CURLE_OK) {
        //     fprintf(stderr, "CURL error code: %d\n", msg->data.result);
        //     continue;
        // }
        
        // Get the saved RECV_BUF
        curl_easy_getinfo(msg->easy_handle, CURLINFO_PRIVATE, &private_recv_buf);
        
        if (*num_png_found >= bfs->max_results) {
            recv_buf_cleanup(private_recv_buf);
            free(private_recv_buf);  
            curl_multi_remove_handle(curl_multi, msg->easy_handle);
            curl_easy_cleanup(msg->easy_handle);         
            continue;
        }

        // Process the data
        queue_t new_urls;
        queue_init(&new_urls);
        
        char* effective_url = process_data(msg->easy_handle, private_recv_buf, &new_urls);
        
        if (effective_url != NULL) { // Is a PNG
            (*num_png_found)++;
            queue_enqueue(&bfs->search_results, effective_url);
            free(effective_url);
        } else {
            char* new_url; // Get the new URLs
            while ((new_url = queue_dequeue(&new_urls)) != NULL) {
                if (hashmap_contains(&bfs->visited, new_url) == 0) {
                    hashmap_put(&bfs->visited, new_url);
                    queue_enqueue(&bfs->frontier, new_url);
                }
                free(new_url);
            }

            queue_cleanup(&new_urls);
        }
        
        recv_buf_cleanup(private_recv_buf);
        free(private_recv_buf);
        
        curl_multi_remove_handle(curl_multi, msg->easy_handle);
        curl_easy_cleanup(msg->easy_handle);
    }
}


queue_t* bfs_start(bfs_t* bfs, const char* root) {
    queue_enqueue(&bfs->frontier, root);
    CURLM* curl_multi;
    CURL* curl_handle;
    size_t num_png_found = 0;
    while (queue_is_empty(&bfs->frontier) == 0) {
        
        curl_multi = curl_multi_init();

        // Get connections from frontier
        int num_opened_connections = min(bfs->max_conntions, bfs->frontier.size);

        RECV_BUF** recv_bufs = malloc(sizeof(RECV_BUF*) * num_opened_connections);
 
        char* url;
        for (int i = 0; i < num_opened_connections; i++) {
            url = queue_dequeue(&bfs->frontier);
            recv_bufs[i] = malloc(sizeof(RECV_BUF)); // All of these RECV_BUFs will be freed in the curl_multi_info_read loop
            
            curl_handle = easy_handle_init(recv_bufs[i], url);
           
            curl_multi_add_handle(curl_multi, curl_handle);
            
            _log(url);
            
            free(url);
        }

        // Perform bfs
        _search_one_cycle(bfs, curl_multi, &num_png_found);
        
        free(recv_bufs);

        curl_multi_cleanup(curl_multi);
    }

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