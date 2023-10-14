#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#include "catpng.h"
#include "urlutils.h"
#include "lab_png.h"

#define NUM_SEGMENTS 50

#define CONCAT(a, b) a##b

typedef struct PNG_segments {
    simple_PNG_p p_pngs[NUM_SEGMENTS]; /* Hold all png segments */
    int num_pngs;                      /* Number of png segments fetched */
    pthread_mutex_t mutex;           /* Mutex for accessing pngs */
} PNG_segments_t, *PNG_segments_p;

// Global variable
extern PNG_segments_t global_PNG_segments; // defined in paster.c

/**
 * @brief      Destroy the PNG_segments_t struct
 * @note       The global variable global_PNG_segments is destroyed.
 *             Should be called at the end of the program.
 */
void cleanup_png_segments();

/**
 * @brief      Fetch the PNG to the PNG_segments_t struct
 * 
 * @param[in]  arg  The url of the PNG (char*)
 * @return     NULL
*/
void* fetch_png(void* arg);

/**
 * @brief      Start nthreads threads to fetch the PNGs
 *             from URLs and concatenate them into one PNG
 * 
 * @param[in] nthreads  The number of threads to fetch the PNGs
 * @param[in] image_id  The image id 1 <= image_id <= 3
 * @return    The concatenated PNG
 * @note     The caller is responsible to call the destory_png method to prevent memory leak.
*/
simple_PNG_p paster(int nthreads, int image_id);

