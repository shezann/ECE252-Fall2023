#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "urlutils.h"
#include "lab_png.h"
#include "shm_utils.h"

void consume(size_t time_to_sleep);

void produce(const char* baseurl, int image_id);