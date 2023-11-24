#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <getopt.h>
#include <curl/curl.h>
#include <sys/time.h>
#include <search.h>

#include "web_crawler.h"
#include "bfs.h"

int main(int argc, char *argv[])
{
    struct timeval start, end;
    long seconds, useconds;
    double total_time = 0.0;

    gettimeofday(&start, NULL);

    int num_threads = 1;
    int num_png_urls = 50;
    char *logfile = NULL;
    char *start_url = SEED_URL;

    curl_global_init(CURL_GLOBAL_DEFAULT);

    xmlInitParser();

    int opt;
    while ((opt = getopt(argc, argv, "t:m:v:")) != -1)
    {
        switch (opt)
        {
        case 't':
            num_threads = atoi(optarg);
            break;
        case 'm':
            num_png_urls = atoi(optarg);
            break;
        case 'v':
            logfile = optarg;
            break;
        default:
            fprintf(stderr, "Usage: %s [-t num_threads] [-m num_png_urls] [-v logfile] \n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    if (optind < argc)
    {
        start_url = argv[optind];
    }

    set_logfile(logfile);


    /* Insert your code here */ 

    bfs_t breath_first_search;
    bfs_init(&breath_first_search, num_threads, num_png_urls, web_crawler);

    queue_t* ret = bfs_start(&breath_first_search, start_url);

    FILE* fp = fopen("png_urls.txt", "w");
    while (queue_is_empty(ret) == 0) {
        char* url = queue_dequeue(ret);
        fprintf(fp, "%s\n", url);
        // printf("png: %s\n", url);
        free(url);
    }
    fclose(fp);
    queue_cleanup(ret);
    free(ret);

    bfs_cleanup(&breath_first_search);

    // search_return_t *ret = NULL;
    // ret = web_crawler(start_url);
    // for (int i = 0; i < ret->n_children; i++)
    // {
    //     printf("herf: %s\n", (char *)ret->children[i]);
    //     free(ret->children[i]);
    // }
    // free(ret->children);
    // if (ret->effective_url != NULL)
    // {
    //     printf("effective url: %s\n", ret->effective_url);
    //     free(ret->effective_url);
    // }
    // free(ret);

    // Cleanup phase
    xmlCleanupParser();

    curl_global_cleanup();

    gettimeofday(&end, NULL);

    seconds = end.tv_sec - start.tv_sec;
    useconds = end.tv_usec - start.tv_usec;
    total_time = seconds + useconds / 1E6;

    printf("%s execution time: %f seconds\n", argv[0], total_time);
    return 0;
}