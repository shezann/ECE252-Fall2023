#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <curl/curl.h>
#include <sys/time.h>
#include <search.h>

#include "web_crawler.h"
#include "bfs.h"

#define MAX_URL_LENGTH 2048

struct MemoryStruct {
    char *memory;
    size_t size;
};

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

// Function to perform cURL request and return the result
char *perform_curl_request(const char *url) {
    CURL *curl;
    CURLcode res;

    struct MemoryStruct chunk;

    chunk.memory = malloc(1);  // will be grown as needed
    chunk.size = 0;            // no data at this point

    curl = curl_easy_init();

    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

        res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        }

        curl_easy_cleanup(curl);
    }

    return chunk.memory;
}

int main(int argc, char *argv[]) {
    struct timeval start, end;
    long seconds, useconds;
    double total_time = 0.0;

    gettimeofday(&start, NULL);

    int num_png_urls = 50;
    char *logfile = NULL;
    char *start_url = SEED_URL;

    int opt;
    while ((opt = getopt(argc, argv, "m:v:")) != -1) {
        switch (opt) {
        case 'm':
            num_png_urls = atoi(optarg);
            break;
        case 'v':
            logfile = optarg;
            break;
        default:
            fprintf(stderr, "Usage: %s [-m num_png_urls] [-v logfile] \n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    if (optind < argc) {
        start_url = argv[optind];
    }

    set_logfile(logfile);

    // Initialize cURL
    curl_global_init(CURL_GLOBAL_DEFAULT);

    // Initialize BFS
    bfs_t breath_first_search;
    bfs_init(&breath_first_search, 1, num_png_urls, web_crawler);

    // Start BFS with a single thread
    queue_t *ret = bfs_start(&breath_first_search, start_url);

    // Open a file to write PNG URLs
    FILE *png_fp = fopen("png_urls.txt", "w");
    while (queue_is_empty(ret) == 0) {
        char *url = queue_dequeue(ret);

        // Perform cURL request to get the content of the URL
        char *url_content = perform_curl_request(url);

        // Parse the content to find PNG URLs and write to the file
        // (You need to implement this part in your web_crawler.c)

        fprintf(png_fp, "%s\n", url);
        free(url_content);
        free(url);
    }
    fclose(png_fp);

    // Clean up
    queue_cleanup(ret);
    free(ret);

    bfs_cleanup(&breath_first_search);

    // Cleanup phase
    curl_global_cleanup();

    gettimeofday(&end, NULL);

    seconds = end.tv_sec - start.tv_sec;
    useconds = end.tv_usec - start.tv_usec;
    total_time = seconds + useconds / 1E6;

    printf("%s execution time: %f seconds\n", argv[0], total_time);
    return 0;
}