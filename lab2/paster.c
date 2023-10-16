#include "paster.h"

PNG_segments_t global_PNG_segments = {
    .p_pngs = {NULL}, // Initialize all pointers to NULL
    .num_pngs = 0,
    .mutex = PTHREAD_MUTEX_INITIALIZER
};

void cleanup_png_segments() {
    for (int i=0; i<NUM_SEGMENTS; i++) {
        if (global_PNG_segments.p_pngs[i]) {
            destroy_png(global_PNG_segments.p_pngs[i]);
        }
    }
    // Destroy the mutex
    pthread_mutex_destroy(&global_PNG_segments.mutex);
}

void *fetch_png(void* arg) {
    // TODO: Implement this function


    CURL* handle = get_handle((char*)arg);

    // Lock the mutex before accessing global_PNG_segments
    pthread_mutex_lock(&global_PNG_segments.mutex);

    // Do something with the handle
    printf("Fetching %s\n", (char*)arg);

    // Unlock the mutex after accessing global_PNG_segments
    pthread_mutex_unlock(&global_PNG_segments.mutex);

    // Cleanup the handle
    curl_easy_cleanup(handle);

    // Free the arg
    free(arg);


    return NULL;
} 

simple_PNG_p paster(int nthreads, int image_id) {
    curl_init();

    /* Create threads to fetch the PNGs */
    pthread_t threads[nthreads];
    for (int i=0; i<nthreads; i++) {
        // Create a thread to fetch the PNG using different URLs
        char* url = malloc(sizeof(char) * 64);
        sprintf(url, "%simg=%d", URL_LIST[i % NUM_URLS], image_id);
        if (pthread_create(&threads[i], NULL, fetch_png, (void *) url) != 0) {
            perror("pthread_create: failed to create thread.");
            return NULL;
        }
    }

    /* Wait for all threads to finish */
    for (int i=0; i<nthreads; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("pthread_join: failed to join thread.");
            return NULL;
        }
    }
    // All threads are finished
    curl_cleanup();

    /* Concatenate the PNGs */
    simple_PNG_p p_png = concatenate_pngs(global_PNG_segments.p_pngs, NUM_SEGMENTS);
    if (p_png == NULL) {
        perror("concat_pngs: returned NULL.");
        return NULL;
    }

    return p_png;
}

int main(int argc, char** argv) {
    int c;              // Used for getopt
    int nthreads = 1;   // Default number of threads
    int image_id = 1;   // Default image id

    char* str = "option requires an argument"; // Used for error message
    char* filename = "all.png";                // Default filename

    // Parse the command line arguments
    while ((c = getopt(argc, argv, "t:n:")) != -1) {
        switch (c)
        {
        case 't': // Number of threads
            nthreads = strtoul(optarg, NULL, 10);
            if (nthreads <= 0) {
                fprintf(stderr, "%s: %s > 0 -- 't'\n", argv[0], str);
                // Free the PNG segments
                cleanup_png_segments();
                return -1;
            }
            break;
        case 'n': // Image id
            image_id = strtoul(optarg, NULL, 10);
            if (image_id <= 0 || image_id > 3) {
                fprintf(stderr, "%s: %s 1, 2, or 3 -- 'n'\n", argv[0], str);
                // Free the PNG segments
                cleanup_png_segments();
                return -1;
            }
            break;
        default: // Invalid option
            printf("Usage: %s [-t <number of threads>] [-n <image number>]\n", argv[0]);
            // Free the PNG segments
            cleanup_png_segments();
            return -1;
        }
    }
    // Fetch and concatenate the PNGs
    simple_PNG_p p_png = paster(nthreads, image_id);
    if (p_png == NULL) {
        perror("paster: returned NULL.\n");
        // Free the PNG segments
        cleanup_png_segments();
        return -1;
    }
    write_png(p_png, filename);

    // Free the new png
    destroy_png(p_png);

    // Free the PNG segments
    cleanup_png_segments();

    return 0;
}