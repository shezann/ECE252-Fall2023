#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <sys/time.h>
#include <search.h>

#include "web_crawler.h"

// ... (other includes and definitions)

struct MemoryStruct {
    char *memory;
    size_t size;
};

// ... (other function definitions)

// Callback function for cURL to write received data
static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;

    mem->memory = realloc(mem->memory, mem->size + realsize + 1);
    if (mem->memory == NULL) {
        // out of memory
        fprintf(stderr, "Not enough memory (realloc returned NULL)\n");
        exit(EXIT_FAILURE);
    }

    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}

// ... (other function definitions)

// Updated web_crawler function for asynchronous I/O
search_return_t *web_crawler(void *arg) {
    const char *url = (const char *)arg;

    _log(url);

    // Initialize cURL multi-handle
    CURLM *multi_handle;
    CURL *curl_handle;
    CURLMsg *msg;
    int still_running;

    // Initialize curl
    curl_global_init(CURL_GLOBAL_DEFAULT);
    multi_handle = curl_multi_init();

    // Initialize memory structure for receiving data
    struct MemoryStruct recv_buf;
    recv_buf.memory = malloc(1);
    recv_buf.size = 0;

    // Initialize curl handle
    curl_handle = easy_handle_init(&recv_buf, url);

    // Add curl handle to multi-handle
    curl_multi_add_handle(multi_handle, curl_handle);

    // Perform asynchronous requests
    do {
        curl_multi_perform(multi_handle, &still_running);

        // Check for completed transfers
        while ((msg = curl_multi_info_read(multi_handle, NULL))) {
            if (msg->msg == CURLMSG_DONE) {
                CURL *completed = msg->easy_handle;

                // Process completed request
                search_return_t *ret = malloc(sizeof(search_return_t));
                ret->effective_url = process_data(completed, &recv_buf, NULL);
                ret->n_children = 0;  // No children in this case
                ret->children = NULL;

                // Clean up completed request
                cleanup(completed, &recv_buf);

                // Remove the completed handle from the multi-handle
                curl_multi_remove_handle(multi_handle, completed);
                curl_easy_cleanup(completed);
                free(recv_buf.memory);

                return ret;
            }
        }

        // Wait for activity on any of the sockets associated with the handles
        struct timeval timeout;
        int rc;
        fd_set fdread, fdwrite, fdexcep;
        int maxfd = -1;

        FD_ZERO(&fdread);
        FD_ZERO(&fdwrite);
        FD_ZERO(&fdexcep);

        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        curl_multi_fdset(multi_handle, &fdread, &fdwrite, &fdexcep, &maxfd);

        rc = select(maxfd + 1, &fdread, &fdwrite, &fdexcep, &timeout);

    } while (still_running);

    // Clean up
    curl_multi_cleanup(multi_handle);
    curl_global_cleanup();

    // The function returns NULL if there's an error or if there are no children
    return NULL;
}