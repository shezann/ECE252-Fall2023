#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <getopt.h>
#include <curl/curl.h>
#include <search.h>
#include "web_crawler_helpers.c"

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

void *process_url(void *url)
{
    CURL *curl_handle;
    RECV_BUF recv_buf;
    char *url_str = (char *)url;
    ENTRY e;
    ENTRY *ep;

    // Initialize the receive buffer
    if (recv_buf_init(&recv_buf, BUF_SIZE) != 0)
    {
        fprintf(stderr, "Failed to initialize receive buffer\n");
        return NULL;
    }

    // Initialize the curl handle
    curl_handle = easy_handle_init(&recv_buf, url_str);
    if (curl_handle == NULL)
    {
        fprintf(stderr, "Failed to initialize curl handle\n");
        recv_buf_cleanup(&recv_buf);
        return NULL;
    }

    // Send a GET request to the URL
    CURLcode res = curl_easy_perform(curl_handle);
    if (res != CURLE_OK)
    {
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        cleanup(curl_handle, &recv_buf);
        return NULL;
    }

    // Check the HTTP status code and handle the response
    if (strstr(recv_buf.buf, CT_HTML) != NULL)
    {
        // If the Content-Type is text/html, extract all URLs and add them to the URLs frontier
        find_http(recv_buf.buf, recv_buf.size, 1, url_str);
    }
    else if (strstr(recv_buf.buf, CT_PNG) != NULL)
    {
        // If the Content-Type is image/png, add the URL to the found PNG URLs
        pthread_mutex_lock(&found_mutex);
        if (found_count < MAX_VISITED_URLS)
        {
            found_png_urls[found_count++] = strdup(url_str);
        }
        pthread_mutex_unlock(&found_mutex);
    }

    if (strstr(recv_buf.buf, CT_HTML) != NULL)
    {
        // If the Content-Type is text/html, extract all URLs and add them to the URLs frontier
        extract_urls(recv_buf.buf, url_str);
    }

    // Mark the URL as visited
    e.key = url_str;
    hsearch_r(e, ENTER, &ep, &visited_urls);

    cleanup(curl_handle, &recv_buf);
    return NULL;
}

void add_url_to_frontier(char *url)
{
    // Lock the frontier_mutex before accessing the urls_frontier array and frontier_count
    pthread_mutex_lock(&frontier_mutex);

    if (frontier_count < MAX_VISITED_URLS)
    {
        urls_frontier[frontier_count++] = strdup(url);
    }
    else
    {
        fprintf(stderr, "URLs frontier is full.\n");
    }

    // Unlock the frontier_mutex after accessing the urls_frontier array and frontier_count
    pthread_mutex_unlock(&frontier_mutex);
}

void extract_urls(char *html, const char *base_url)
{
    htmlDocPtr doc;
    xmlXPathContextPtr context;
    xmlXPathObjectPtr result;
    xmlNodeSetPtr nodeset;
    xmlChar *xpath = (xmlChar *)"//a/@href";
    int i;

    // Parse the HTML into a document
    doc = htmlReadMemory(html, strlen(html), base_url, NULL, HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING);
    if (doc == NULL)
    {
        fprintf(stderr, "Failed to parse HTML.\n");
        return;
    }

    // Create an XPath evaluation context
    context = xmlXPathNewContext(doc);
    if (context == NULL)
    {
        fprintf(stderr, "Failed to create XPath context.\n");
        xmlFreeDoc(doc);
        return;
    }

    // Evaluate the XPath expression
    result = xmlXPathEvalExpression(xpath, context);
    if (result == NULL)
    {
        fprintf(stderr, "Failed to evaluate XPath expression.\n");
        xmlXPathFreeContext(context);
        xmlFreeDoc(doc);
        return;
    }

    // Iterate over the result and add each URL to the URLs frontier
    nodeset = result->nodesetval;
    for (i = 0; i < nodeset->nodeNr; i++)
    {
        xmlChar *href = xmlNodeListGetString(doc, nodeset->nodeTab[i]->xmlChildrenNode, 1);
        char *url = xmlBuildURI(href, (xmlChar *)base_url); // Resolve relative URLs

        // Add the URL to the URLs frontier
        pthread_mutex_lock(&frontier_mutex);
        if (frontier_count < MAX_VISITED_URLS)
        {
            urls_frontier[frontier_count++] = strdup(url);
        }
        pthread_mutex_unlock(&frontier_mutex);

        // Add the URL to the URLs frontier
        add_url_to_frontier(url);

        xmlFree(href);
        xmlFree(url);
    }

    // Clean up
    xmlXPathFreeObject(result);
    xmlXPathFreeContext(context);
    xmlFreeDoc(doc);
}

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

    // Add the start URL to the URLs frontier
    add_url_to_frontier(start_url);

    // Create the worker threads
    pthread_t threads[num_threads];
    for (int i = 0; i < num_threads; i++)
    {
        if (pthread_create(&threads[i], NULL, process_url, start_url) != 0)
        {
            fprintf(stderr, "Failed to create thread\n");
            exit(EXIT_FAILURE);
        }
    }

    // Join the worker threads
    for (int i = 0; i < num_threads; i++)
    {
        if (pthread_join(threads[i], NULL) != 0)
        {
            fprintf(stderr, "Failed to join thread\n");
            exit(EXIT_FAILURE);
        }
    }

    // Print the found PNG URLs
    for (int i = 0; i < found_count; i++)
    {
        printf("%s\n", found_png_urls[i]);
    }

    return 0;
}