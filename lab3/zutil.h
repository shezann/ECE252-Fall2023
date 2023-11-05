#pragma once

/* INCLUDES */
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "zlib.h"
#include "lab_png.h"

/* DEFINES */
#if defined(MSDOS) || defined(OS2) || defined(WIN32) || defined(__CYGWIN__)
#  include <fcntl.h>
#  include <io.h>
#  define SET_BINARY_MODE(file) setmode(fileno(file), O_BINARY)
#else
#  define SET_BINARY_MODE(file)
#endif

#define CHUNK 16384  /* =256*64 on the order of 128K or 256K should be used */

typedef unsigned char U8;
typedef unsigned long int U64;

/* FUNCTION PROTOTYPES */

/**
 * @brief deflate in memory data from source to dest.
 *         The memory areas must not overlap.
 * @param dest_len, U64* output parameter, points to length of deflated data
 * @param source U8* source buffer, contains data to be deflated
 * @param source_len U64 length of source data
 * @param level int compression levels (https://www.zlib.net/manual.html)
 *    Z_NO_COMPRESSION, Z_BEST_SPEED, Z_BEST_COMPRESSION, Z_DEFAULT_COMPRESSION
 * @return dest U8* output buffer, caller is responsible to free, should be big enough
 *         to hold the deflated data
 * NOTE: 1. the compressed data length may be longer than the input data length,
 *          especially when the input data size is very small.
 */
U8 *mem_def(U64 *dest_len, U8 *source,  U64 source_len, int level);

/**
 * @brief inflate in memory data from source to dest 
 * 
 * @param dest_len, U64* output parameter, length of inflated data
 * @param source U8* source buffer, contains zlib data to be inflated
 * @param source_len U64 length of source data
 * 
 * @return dest U8* output buffer, caller supplies, should be big enough
 *         to hold the deflated data
 */
U8 *mem_inf(U64 *dest_len, U8 *source,  U64 source_len);