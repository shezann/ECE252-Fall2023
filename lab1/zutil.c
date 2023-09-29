#include <stdio.h>
#include "zutil.h"

U8* mem_def(U64 *dest_len, U8 *source, U64 source_len, int level) {
    z_stream strm;          /* output buffer for deflate()            */
    int ret;                /* zlib return code                       */
    U64 max_compressed_len; /* accumulated deflated data length       */
    U8* compressed_data;    

    /* Initialize deflate structure */ 
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;


    ret = deflateInit(&strm, level);
    if (ret != Z_OK) {
        perror("zlib return code is not ok.\n");
        return NULL;
    } 

    /* Calculate the maximum compressed length */ 
    max_compressed_len = deflateBound(&strm, source_len);
    compressed_data = (U8 *)malloc(max_compressed_len);
    if (!compressed_data) {
        (void) deflateEnd(&strm);
        return NULL;
    }

    /* set input data stream */ 
    strm.avail_in = source_len;
    strm.next_in = source;
    strm.avail_out = max_compressed_len;
    strm.next_out = compressed_data;
    
    ret = deflate(&strm, Z_FINISH);
    if (ret == Z_STREAM_ERROR) { /* Check if there if any error */
        perror("Error when compressing the memory.\n");
        return NULL;
    } 

    *dest_len = max_compressed_len - strm.avail_out; /* Retruns the dest length */

    /* clean up and return */
    (void) deflateEnd(&strm);
    if (ret == Z_STREAM_END) { /* stream will be complete */
        return compressed_data;
    } else {
        free(compressed_data);
        return NULL;
    }
}

U8 *mem_inf(U64 *dest_len, U8 *source, U64 source_len) {
    z_stream strm;          /* pass info. to and from zlib routines   */
    int ret;                /* output buffer for inflate()            */
    U8 *decompressed_data;
    U64 decompressed_data_len = source_len * 2;  /* initial size */ 

    /* allocate inflate state 8 */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;

    ret = inflateInit(&strm);
    if (ret != Z_OK) {
        perror("zlib return code is not ok.\n");
        return NULL;
    } 
    
    decompressed_data = malloc(decompressed_data_len);
    if (!decompressed_data) {
        (void)inflateEnd(&strm);
        return NULL;
    }

    /* set input data stream */
    strm.avail_in = source_len;
    strm.next_in = source;

    /* run inflate() on input until output buffer not full */
    do {
        /* If the output buffer is larger the expexcte, double the buffer length */
        if (strm.total_out >= decompressed_data_len) {
            decompressed_data_len *= 2;
            decompressed_data = realloc(decompressed_data, decompressed_data_len);
        }

        strm.avail_out = decompressed_data_len - strm.total_out;
        strm.next_out = decompressed_data + strm.total_out;

        /* zlib format is self-terminating, no need to flush */
        ret = inflate(&strm, Z_NO_FLUSH);
        assert(ret != Z_STREAM_ERROR);  // Stream state should be consistent
    } while (ret == Z_OK);

    *dest_len = strm.total_out;
    (void)inflateEnd(&strm);

    if (ret == Z_STREAM_END) {
        return decompressed_data;
    } else {
        free(decompressed_data);
        return NULL;
    }
}