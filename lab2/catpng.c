#include "catpng.h"

U64 calculate_buffer_size(U32 png_width, U32 png_height) {
    return png_height * (png_width * 4 + 1);
}

simple_PNG_p concatenate_pngs(simple_PNG_p* pngs, U64 arr_size) {
    if (arr_size == 0) {
        return NULL;
    }
    // Allocate a buffer that is big enough to store all the pngs
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
        U8* p_uncompressed = mem_inf(&uncompressed_size, 
                p_IDAT->p_data, p_IDAT->length); // TODO: Catch any potential expection here.
        memcpy(uncompressed_buf + buf_pointer, p_uncompressed, uncompressed_size);
        free(p_uncompressed);
        // printf("Uncompressed: %ld; Expected: %ld\n", uncompressed_size, calculate_buffer_size(pngs[i]->p_IHDR->p_data_IHDR->width, pngs[i]->p_IHDR->p_data_IHDR->height));
        buf_pointer += uncompressed_size;
    }

    /* Allocate a bufffer for the deflated data */
    U64 compressed_size = 0;
    U8* compressed_buf = mem_def(&compressed_size, uncompressed_buf, buf_size, Z_DEFAULT_COMPRESSION);
    // printf("Compressed size: %ld\n", compressed_size);
    // Free the uncompressed buffer
    free(uncompressed_buf);

    // Build the new PNG
    simple_PNG_p new_png = malloc(sizeof(struct simple_PNG));

    // Calculate the new png buffer size
    U64 chunk_common = CHUNK_CRC_SIZE + CHUNK_LEN_SIZE + CHUNK_TYPE_SIZE;
    U64 new_png_buf_size =  PNG_SIG_SIZE + 
                            13 + chunk_common + /* IHDR size */
                            compressed_size + chunk_common + /* IDAT size */
                            chunk_common; /* IEND size */

    new_png->p_png_buffer = malloc(new_png_buf_size);
    new_png->buf_length = new_png_buf_size;
    memcpy(new_png->p_png_buffer, PNG_SIGNITURE, PNG_SIG_SIZE); /* Write the signiture */

    // IHDR
    write_big_endian_u32(new_png->p_png_buffer + 8, 13); /* Chunk length */
    memcpy(new_png->p_png_buffer + 12, TYPE_IHDR, CHUNK_TYPE_SIZE); /* Chunk type */
    write_big_endian_u32(new_png->p_png_buffer + 16, width); /* Width */
    write_big_endian_u32(new_png->p_png_buffer + 20, height); /* Height */
    buf_pointer = 24;
    new_png->p_png_buffer[buf_pointer++] = 8; /* Bit depth */
    new_png->p_png_buffer[buf_pointer++] = 6; /* Colour type */
    new_png->p_png_buffer[buf_pointer++] = 0; /* Compression method */
    new_png->p_png_buffer[buf_pointer++] = 0; /* Filter method */
    new_png->p_png_buffer[buf_pointer++] = 0; /* Interlace method */

    write_big_endian_u32(new_png->p_png_buffer + buf_pointer, crc(new_png->p_png_buffer + 12, 17));
    buf_pointer += 4;

    // IDAT
    write_big_endian_u32(new_png->p_png_buffer + buf_pointer, compressed_size); /* Chunk length */
    buf_pointer += 4;
    U64 crc_pointer = buf_pointer;
    memcpy(new_png->p_png_buffer + buf_pointer, TYPE_IDAT, CHUNK_TYPE_SIZE); /* Chunk type */
    buf_pointer += 4;
    memcpy(new_png->p_png_buffer + buf_pointer, compressed_buf, compressed_size); /* Chunk data */
    buf_pointer += compressed_size;
    write_big_endian_u32(new_png->p_png_buffer + buf_pointer, crc(new_png->p_png_buffer + crc_pointer, compressed_size + CHUNK_TYPE_SIZE));
    buf_pointer += 4;
    free(compressed_buf);

    //IEND
    write_big_endian_u32(new_png->p_png_buffer + buf_pointer, 0);
    buf_pointer += 4;
    crc_pointer = buf_pointer;
    memcpy(new_png->p_png_buffer + buf_pointer, TYPE_IEND, CHUNK_TYPE_SIZE); /* Chunk type */
    buf_pointer += 4;
    write_big_endian_u32(new_png->p_png_buffer + buf_pointer, crc(new_png->p_png_buffer + crc_pointer, CHUNK_TYPE_SIZE));
    buf_pointer += 4;

    U8* p_chunk_start = new_png->p_png_buffer + PNG_SIG_SIZE;
    U64 buf_size_remained = new_png->buf_length - PNG_SIG_SIZE;

    // Read the raw data into a full simple_PNG structure.
    new_png->p_IHDR = create_chunk();
    if (!read_chunk(p_chunk_start, buf_size_remained, new_png->p_IHDR, &p_chunk_start, &buf_size_remained)) {
        printf("Error when reading the IHDR chunk.\n");
        destroy_png(new_png);
        return NULL;
    }
    new_png->p_IDAT = create_chunk();
    if (!read_chunk(p_chunk_start, buf_size_remained, new_png->p_IDAT, &p_chunk_start, &buf_size_remained)) {
        printf("Error when reading the IDAT chunk.\n");
        destroy_png(new_png);
        return NULL;
    }
    new_png->p_IEND = create_chunk();
    if (!read_chunk(p_chunk_start, buf_size_remained, new_png->p_IEND, &p_chunk_start, &buf_size_remained)) {
        printf("Error when reading the IEND chunk.\n");
        destroy_png(new_png);
        return NULL;
    }

    return new_png;
}

// int main(int argc, char* argv[]) {
//     if (argc > 1) {
//         simple_PNG_p* pngs = malloc(sizeof(simple_PNG_p) * (argc - 1));
//         int valid_pngs_count = 0;
//         for (int i=0; i<argc-1; i++)
//             pngs[i] = NULL; /* Initialize the array */
//         for (int i=1; i<argc; i++) {
//             if (is_png(argv[i])) {
//                 pngs[valid_pngs_count] = create_png(argv[i]);
//                 valid_pngs_count += 1;
//                 // printf("Adding %s.\n", argv[i]);
//             } else {
//                 printf("%s is not a valid PNG file. Existing...\n", argv[i]);
//                 for (int j=0; j<argc-1; j++) {
//                     if (pngs[j] != NULL) {
//                         destroy_png(pngs[j]);
//                     }
//                     free(pngs);
//                     return 1;
//                 }
//             }
//         }
//         /* Copy all valid pngs to a new array */
//         simple_PNG_p* valid_pngs = malloc(sizeof(simple_PNG_p) * valid_pngs_count);
//         memcpy(valid_pngs, pngs, sizeof(simple_PNG_p) * valid_pngs_count);
//         free(pngs); /* Free the buffer */
        
//         /* Concatenate pngs */
//         simple_PNG_p concatenated_png = concatenate_pngs(valid_pngs, valid_pngs_count);
//         write_png(concatenated_png, "./all.png"); /* Write the new png to all.png */

//         // Free all pngs
//         for (int i=0; i<valid_pngs_count; i++) {
//             destroy_png(valid_pngs[i]);
//             valid_pngs[i] = NULL;
//         }
//         free(valid_pngs);

//         // Free the new png
//         destroy_png(concatenated_png);
//         concatenated_png = NULL;
//     } else {
//         printf("Usage: %s <directory name 1> <directory name 2> ...\n", argv[0]);
//         return 1;
//     }
//     return 0;
// }