#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <getopt.h>
#include <curl/curl.h>
#include <search.h>
#include "web_crawler_helpers.h"

#define MAX_URL_LENGTH 256
#define MAX_THREADS 10
#define MAX_VISITED_URLS 500
#define SEED_URL "http://ece252-1.uwaterloo.ca/lab4/"

// Data structures for URLs
char *urls_frontier[MAX_VISITED_URLS];
char *found_png_urls[MAX_VISITED_URLS];
ENTRY visited_urls[MAX_VISITED_URLS];

// Counters for URLs
int frontier_count = 0;
int found_count = 0;
int visited_count = 0;

// Mutexes for data structures
pthread_mutex_t frontier_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t found_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t visited_mutex = PTHREAD_MUTEX_INITIALIZER;

// MAIN
int main(int argc, char *argv[])
{
    int num_threads = 1;
    int num_png_urls = 50;
    char *logfile = NULL;
    char *start_url = SEED_URL;

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

    // Initialize hash table
    hcreate(MAX_VISITED_URLS);

    return 0;
}