#include "paster.h"

PNG_segments_t global_PNG_segments = {
    .p_pngs = {NULL},
    .num_pngs = 0,
    .mutex = PTHREAD_MUTEX_INITIALIZER
};

