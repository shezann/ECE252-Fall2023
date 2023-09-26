#include "catpng.h"

U64 calculate_buffer_size(U32 png_width, U32 png_height) {
    return png_height * (png_width * 4 + 1);
}

simple_PNG_p concatenate_pngs(simple_PNG_p* pngs, U64 arr_size) {
    // Allocate a buffer that is big enough to store all the pngs
    U64 buf_size = 0;
    U32 width = pngs[0]->p_IHDR->p_data_IHDR->width; /* Assume all pngs of the same width */
    U32 height = 0;
    for (int i=0; i<arr_size; i++) {
        height += pngs[i]->p_IHDR->p_data_IHDR->height; /* Concatenate all pngs */
    }
    U64 buf_size = calculate_buffer_size(width, height);
    U8* uncompressed_buf = malloc(buf_size * sizeof(U8));

    if (uncompressed_buf == NULL) {
        perror("Not enough memory to allocate the buffer.\n");
        return NULL;
    }

    // Inflate all the png data
    U64 buf_pointer = 0; /* A pointer to the data of the next png */
    for (int i=0; i<arr_size; i++) {
        chunk_p p_IDAT = pngs[i]->p_IDAT;
        U64 uncompressed_size = 0;
        mem_inf(uncompressed_buf + buf_pointer, &uncompressed_size, 
                p_IDAT->p_data, p_IDAT->length); // TODO: Catch any potential expection here.
        buf_pointer += uncompressed_size;
    }

    // Deflate the new png data
    U8* compressed_buf = malloc(buf_size * sizeof(U8)); /* Allocate a bufffer for the deflated data */

    U64 compressed_size = 0;
    mem_def(compressed_buf, &compressed_size, uncompressed_buf, buf_size, Z_DEFAULT_COMPRESSION);

    // Free the uncompressed buffer
    free(uncompressed_buf);

    // Build the new PNG
    simple_PNG_p new_png = malloc(sizeof(struct simple_PNG));

    // Calculate the new png buffer size
    U64 chunk_common = CHUNK_CRC_SIZE + CHUNK_LEN_SIZE + CHUNK_TYPE_SIZE;
    U64 new_png_buf_size = 13 + chunk_common + /* IHDR size */
                            compressed_size + chunk_common + /* IDAT size */
                            chunk_common; /* IEND size */

    new_png->p_png_buffer = malloc(new_png_buf_size);
    memcpy(new_png->p_png_buffer, PNG_SIGNITURE, PNG_SIG_SIZE); /* Write the signiture */
    
    
    return NULL;
}