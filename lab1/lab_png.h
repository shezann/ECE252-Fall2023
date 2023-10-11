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
#include "zutil.h"
#include "crc.h"

/******************************************************************************
 * STRUCTURES and TYPEDEFS 
 *****************************************************************************/

typedef unsigned char U8;
typedef unsigned int  U32;
typedef unsigned long int U64;

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
}* chunk_p;

/* A simple PNG file format, three chunks only*/
typedef struct simple_PNG {
    chunk_p p_IHDR;
    chunk_p p_IDAT;  /* only handles one IDAT chunk */  
    chunk_p p_IEND;

    U8* p_png_buffer; /* The buffer to hold the */
    U64 buf_length;   /* The length of the buffer */
}* simple_PNG_p;


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
extern const U8 TYPE_IDAT[];
extern const U8 TYPE_IEND[];

/******************************************************************************
 * FUNCTION PROTOTYPES 
 *****************************************************************************/

/**
 * @brief Write a PNG to a given directory.
 * 
 * @param[in] p_png The pointer to the PNG struct.
 * @param[in] directory The path to the output file.
*/
void write_png(simple_PNG_p p_png, const char* filename);

/**
 * @brief Create a PNG struct.
 * 
 * @param[in] path The path to the file.
 * @return A pointer to the PNG struct. 
 * @exception Returns NULL on errors (e.g. allocation failure, file not found, bad formatted).
 * @note The caller is responsible to call the destory_png method to prevent memory leak.
 * @see destory_png
*/
simple_PNG_p create_png(char* path);

/**
 * @brief Destory a PNG structure.
 * 
 * @param[in] p_png The pointer to the PNG struct to destroy.
 * @see create_png
*/
void destroy_png(simple_PNG_p p_png);

/**
 * @brief Verify if the given path is a PNG file.
 * 
 * @param[in] path The path to the file.
*/
int is_png(const char* path);

/**
 * @brief Loads the content of a file into a buffer. 
 * 
 * @param[in] path          Path to the file.
 * @param[in] p_buf_length  Pointer to be updated to store the buffer's length. 
 * @return      A pointer to the buffer containing the file's content.
 * @exception   Returns NULL on errors (e.g., allocation failure, file not found).
 * @note        The caller is responsible to free the returned pointer to prevent memory leak.
 */
U8* read_buf(const char* path, U64* p_buf_length);

/**
 * @brief Read a PNG chunk from the provided buffer.
 * 
 * 
 * @param[in] p_chunk_start             Pointer to the starting position of the chunk within the buffer.
 * @param[in] buf_length                The length of the buffer.
 * @param[in] p_chunk                   Pointer to the empty chunk structure.
 * @param[out] p_next_chunk_start       Pointer to be updated with the starting position of the next chunk.
 * @param[out] p_remained_buffer_size   Pointer to be updated with the remaining size of the buffer after reading the chunk.
 * @return Returns 1 if the chunk is read successfully; otherwise, returns 0.
 */
int read_chunk( U8* p_chunk_start, 
                U64 buf_length, 
                chunk_p p_chunk, 
                U8** p_next_chunk_start, 
                U64* p_remained_buffer_size);


/**
 * @brief Read the IHDR data of a PNG file.
 * 
 * @param[in] p_data    Pointer to the starting position of the IHDR data.
 * @return              Returns a pointer to data_IHDR containing the IHDR data. 
 * @note                The caller is responsible to free the returned pointer to prevent memory leak.
*/
data_IHDR_p read_IHDR(U8* p_data);

/**
 * @brief Create an empty chunk.
 * 
 * @return Return a new chunk_p with everything initialized to 0 or NULL.
 * @note   The caller is responsible to call the destroy_chunk method to prevent memory leak.
 * @see destroy_chunk
*/
chunk_p create_chunk();

/**
 * @brief Destroy a chunk structure.
 * 
 * This function frees all fields within the chunk structure and the structure itself. 
 * However, it does not free the p_data field, as this is not allocated by the chunk methods.
 * 
 * @param[in] p_chunk Pointer to the chunk to be destroyed.
 * @see create_chunk
 */
void destroy_chunk(chunk_p p_chunk);

/**
 * @brief Checks the CRC code of a given chunk.
 * 
 * @param[in] p_chunk   Pointer to the chunk whose CRC needs to be verified.
 * @return              Returns the CRC code.
 */
U32 verify_crc(chunk_p p_chunk);


/**
 * @brief Convert 4 bytes from big-endian format to an unsigned integer.
 * 
 * @param[in] p_chunk_start Pointer to the start of the 4-byte sequence.
 * @return The decoded unsigned integer.
 */
U32 read_big_endian_u32(U8* p_chunk_start);

/**
 * @brief Write an unsigned integer into the buffer in big-endian format
 * 
 * @param[in] p_chunk_start Pointer to the start of the 4-byte sequence.
 * @param value The unsigned integer.
*/
void write_big_endian_u32(U8* p_chunk_start, U32 value);