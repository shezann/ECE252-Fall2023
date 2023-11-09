#include "paster.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <curl/curl.h>

PNG_segments_t global_PNG_segments = {
    .p_pngs = {NULL}, // Initialize all pointers to NULL
    .num_pngs = 0,
    .mutex = PTHREAD_MUTEX_INITIALIZER};

void cleanup_png_segments()
{
    for (int i = 0; i < NUM_SEGMENTS; i++)
    {
        if (global_PNG_segments.p_pngs[i])
        {
            destroy_png(global_PNG_segments.p_pngs[i]);
        }
    }
    // Destroy the mutex
    pthread_mutex_destroy(&global_PNG_segments.mutex);
}

void *fetch_png(void *arg)
{
    CURL *handle = get_handle((char *)arg);

    while (1)
    {
        // Check if all png segments get fetched
        pthread_mutex_lock(&global_PNG_segments.mutex);
        if (global_PNG_segments.num_pngs >= 50)
        {
            pthread_mutex_unlock(&global_PNG_segments.mutex);
            break;
        }
        pthread_mutex_unlock(&global_PNG_segments.mutex);

        Recv_buf_p p_recv_buf = fetch_url(handle);
        if (p_recv_buf == NULL)
        {
            fprintf(stderr, "fetch_url(): returned NULL %s\n", (char *)arg);
            destroy_recv_buf(p_recv_buf);
            continue;
        }

        // Lock the mutex before accessing the global_PNG_segments
        pthread_mutex_lock(&global_PNG_segments.mutex);

        if (global_PNG_segments.p_pngs[p_recv_buf->seq] == NULL)
        {
            // Debug info
            // for (int i=0; i<8; i++) {
            //     printf("%x ", p_recv_buf->buf[i]);
            // }
            // printf("seq=%d num_recieved=%d\n", p_recv_buf->seq, global_PNG_segments.num_pngs + 1);

            // Make a copy of the p_recv_buf->buf
            unsigned char *png_buf = malloc(p_recv_buf->size * sizeof(unsigned char *));
            if (png_buf != NULL)
            {
                memcpy(png_buf, p_recv_buf->buf, p_recv_buf->size);
                // Create a new PNG struct and save it to the global variable
                global_PNG_segments.p_pngs[p_recv_buf->seq] = create_png_from_buf(png_buf, p_recv_buf->size);
                global_PNG_segments.num_pngs++;
            }
            else
            {
                perror("malloc: not enough memory to allocate a new buffer for PNG");
            }
        }

        pthread_mutex_unlock(&global_PNG_segments.mutex);

        destroy_recv_buf(p_recv_buf);
    }

    // Cleanup the handle
    curl_easy_cleanup(handle);

    // Free the arg
    free(arg);

    return NULL;
}

simple_PNG_p paster(int nthreads, int image_id)
{
    curl_init();

    /* Create threads to fetch the PNGs */
    pthread_t *threads = malloc(sizeof(pthread_t) * min(nthreads, 100)); // Allocate 100 threads at a time
    if (threads == NULL)
    {
        perror("malloc: not enough memory to create threads.\n");
        curl_cleanup();
        return NULL;
    }

    int nthreads_created = 0; // Number of threads created successfully. Ideally, it should be equal to nthreads.
    for (int i = 0; i < nthreads; i++)
    {
        // Create a thread to fetch the PNG using different URLs
        char *url = malloc(sizeof(char) * 64);
        sprintf(url, "%simg=%d", URL_LIST[i % NUM_URLS], image_id);
        if (pthread_create(&threads[i], NULL, fetch_png, (void *)url) != 0)
        {
            perror("pthread_create: failed to create more threads.\n");
            free(url);
            break;
        }
        nthreads_created++;

        // Reallocate the threads array if it is full. To prevent from segmentation fault.
        if (nthreads_created % 100 == 0)
        {
            pthread_t *new_threads = realloc(threads, sizeof(pthread_t) * (nthreads_created + 100));
            if (new_threads == NULL)
            {
                perror("realloc: not enough memory to create more threads.\n");
                break;
            }
            threads = new_threads;
        }
    }

    /* Wait for all threads to finish */
    for (int i = 0; i < nthreads_created; i++)
    {
        if (pthread_join(threads[i], NULL) != 0)
        {
            fprintf(stderr, "pthread_join: failed to join thread %d.\n", i);
        }
    }
    // All threads are finished
    curl_cleanup();

    /* Concatenate the PNGs */
    simple_PNG_p p_png = concatenate_pngs(global_PNG_segments.p_pngs, NUM_SEGMENTS);
    if (p_png == NULL)
    {
        perror("concat_pngs: returned NULL.");
        return NULL;
    }

    return p_png;
}

// int main(int argc, char **argv)
// {
//     int c;            // Used for getopt
//     int nthreads = 1; // Default number of threads
//     int image_id = 1; // Default image id

//     char *str = "option requires an argument"; // Used for error message
//     char *filename = "all.png";                // Default filename

//     // Parse the command line arguments
//     while ((c = getopt(argc, argv, "t:n:")) != -1)
//     {
//         switch (c)
//         {
//         case 't': // Number of threads
//             nthreads = strtoul(optarg, NULL, 10);
//             if (nthreads <= 0)
//             {
//                 fprintf(stderr, "%s: %s > 0 -- 't'\n", argv[0], str);
//                 // Free the PNG segments
//                 cleanup_png_segments();
//                 return -1;
//             }
//             break;
//         case 'n': // Image id
//             image_id = strtoul(optarg, NULL, 10);
//             if (image_id <= 0 || image_id > 3)
//             {
//                 fprintf(stderr, "%s: %s 1, 2, or 3 -- 'n'\n", argv[0], str);
//                 // Free the PNG segments
//                 cleanup_png_segments();
//                 return -1;
//             }
//             break;
//         default: // Invalid option
//             printf("Usage: %s [-t <number of threads>] [-n <image number>]\n", argv[0]);
//             // Free the PNG segments
//             cleanup_png_segments();
//             return -1;
//         }
//     }
//     // Fetch and concatenate the PNGs
//     simple_PNG_p p_png = paster(nthreads, image_id);
//     if (p_png == NULL)
//     {
//         perror("paster: returned NULL.\n");
//         // Free the PNG segments
//         cleanup_png_segments();
//         return -1;
//     }
//     write_png(p_png, filename);

//     // Free the new png
//     destroy_png(p_png);

//     // Free the PNG segments
//     cleanup_png_segments();

//     return 0;
// }