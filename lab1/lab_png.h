/**
 * @brief  micros and structures for a simple PNG file 
 *
 * Copyright 2018-2020 Yiqing Huang
 *
 * This software may be freely redistributed under the terms of MIT License
 */
#pragma once

/******************************************************************************
 * INCLUDE HEADER FILES
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "crc.h"

/******************************************************************************
 * STRUCTURES and TYPEDEFS 
 *****************************************************************************/
typedef unsigned char U8;
typedef unsigned int  U32;
typedef unsigned long int U64;

/******************************************************************************
 * DEFINED MACROS 
 *****************************************************************************/

#define PNG_SIG_SIZE    8 /* number of bytes of png image signature data */
#define CHUNK_LEN_SIZE  4 /* chunk length field size in bytes */          
#define CHUNK_TYPE_SIZE 4 /* chunk type field size in bytes */
#define CHUNK_CRC_SIZE  4 /* chunk CRC field size in bytes */
#define DATA_IHDR_SIZE 13 /* IHDR chunk data field size */

extern const U8 PNG_SIGNITURE[];
extern const U8 TYPE_IHDR[];

/* note that there are 13 Bytes valid data, compiler will padd 3 bytes to make
   the structure 16 Bytes due to alignment. So do not use the size of this
   structure as the actual data size, use 13 Bytes (i.e DATA_IHDR_SIZE macro).
 */
typedef struct data_IHDR {// IHDR chunk data 
    U32 width;        /* width in pixels, big endian   */
    U32 height;       /* height in pixels, big endian  */
    U8  bit_depth;    /* num of bits per sample or per palette index.
                         valid values are: 1, 2, 4, 8, 16 */
    U8  color_type;   /* =0: Grayscale; =2: Truecolor; =3 Indexed-color
                         =4: Greyscale with alpha; =6: Truecolor with alpha */
    U8  compression;  /* only method 0 is defined for now */
    U8  filter;       /* only method 0 is defined for now */
    U8  interlace;    /* =0: no interlace; =1: Adam7 interlace */
} *data_IHDR_p;

typedef struct chunk {
    U32 length;  /* length of data in the chunk, host byte order */
    U8  type[4]; /* chunk type */
    U8  *p_data; /* pointer to location where the actual data are */
    U32 crc;     /* CRC field  */

    data_IHDR_p p_data_IHDR; /* Save the IHDR data if this is an IHDR chunk */
    U8 is_corrupted; /* If the data is corrupted */
} *chunk_p;

/* A simple PNG file format, three chunks only*/
typedef struct simple_PNG {
    chunk_p p_IHDR;
    chunk_p p_IDAT;  /* only handles one IDAT chunk */  
    chunk_p p_IEND;

    U8* p_png_buffer;
    U64 buf_length;
} *simple_PNG_p;

/******************************************************************************
 * FUNCTION PROTOTYPES 
 *****************************************************************************/

simple_PNG_p create_png(char* path);

void destroy_png(simple_PNG_p p_png);

/**
 * 
*/
int is_png(char* path);

/**
 * 
*/
U8* read_buf(char* path, U64* p_buf_length);

int read_chunk( U8* p_chunk_start, 
                U64 buf_length, 
                chunk_p p_chunk, 
                U8** p_next_chunk_start, 
                U64* p_remained_buffer_size);

data_IHDR_p read_IHDR(U8* p_data);

/**
 * Create an empty chunk
 * 
 * @return return a new chunk_p with everything initialized to 0 or NULL
*/
chunk_p create_chunk();

/**
 * Destory a chunk structure: This method will free everything 
 * inside chunk structure, and also the p_chunk. However, it will not 
 * free the p_chunk->p_data as it's not allocated inside the chunk methods.
 * 
 * @param p_chunk The chunk to destroy
*/
void destory_chunk(chunk_p p_chunk);

void verify_crc(chunk_p p_chunk);

U32 read_big_endianness_u32(U8* p_chunck_start);