#include "lab_png.h"

/* The magic number of the PNG header */
const U8 PNG_SIGNITURE[PNG_SIG_SIZE] = { 0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A };
/* The file type names */
const U8 TYPE_IHDR[CHUNK_TYPE_SIZE + 1] = "IHDR";
const U8 TYPE_IDAT[CHUNK_TYPE_SIZE + 1] = "IDAT";
const U8 TYPE_IEND[CHUNK_TYPE_SIZE + 1] = "IEND";


int is_png(const char* path) {
    FILE* file = fopen(path, "rb");

    if (!file) { // The file doesn't exists
        return 0;
    }

    // Stack buffer to save the image header
    U8 header_buf[8];

    // Read the first 8 bytes(U8) of the header file
    U64 buf_len = fread(header_buf, sizeof(U8), PNG_SIG_SIZE, file);

    fclose(file); // Close the file and unsign the pointer
    file = NULL;

    // The file size is too small
    if (buf_len != 8) {
        return 0;
    }
    
    // Check the header signitures
    for (int i = 0; i < PNG_SIG_SIZE; i++) {
        if (header_buf[i] != PNG_SIGNITURE[i]) {
            return 0; 
        }
    }

    return 1;
}

void write_png(simple_PNG_p p_png, const char* filename) {
    FILE *file = fopen(filename, "wb"); /* Open a file in binary mode */
    if (file == NULL) {
        perror("Error when writing the file.\n");
        return;
    }
    U64 written = fwrite(p_png->p_png_buffer, 1, p_png->buf_length, file);
    fclose(file); 

    if (written != p_png->buf_length) {
        perror("Error when writing the file.\n");
    }
}

simple_PNG_p create_png(char* path) {
    if (!is_png(path)) {
        printf("%s is not a png file.\n", path);
        return NULL;
    }
    // Create an empty png file and initialize all its data
    simple_PNG_p p_png = malloc(sizeof(struct simple_PNG));

    p_png->buf_length = 0;
    p_png->p_png_buffer = NULL;
    p_png->p_png_buffer = read_buf(path, &(p_png->buf_length));
    if (!p_png->p_png_buffer) {
        printf("Error when reading the PNG.\n");
        destroy_png(p_png);
        return NULL;
    }

    U8* p_chunk_start = p_png->p_png_buffer + PNG_SIG_SIZE;
    U64 buf_size = p_png->buf_length - PNG_SIG_SIZE;

    // Read the raw chunks
    p_png->p_IHDR = create_chunk();
    if (!read_chunk(p_chunk_start, buf_size, p_png->p_IHDR, &p_chunk_start, &buf_size)) {
        printf("Error when reading the IHDR chunk.\n");
        destroy_png(p_png);
        return NULL;
    }
    p_png->p_IDAT = create_chunk();
    if (!read_chunk(p_chunk_start, buf_size, p_png->p_IDAT, &p_chunk_start, &buf_size)) {
        printf("Error when reading the IDAT chunk.\n");
        destroy_png(p_png);
        return NULL;
    }
    p_png->p_IEND = create_chunk();
    if (!read_chunk(p_chunk_start, buf_size, p_png->p_IEND, &p_chunk_start, &buf_size)) {
        printf("Error when reading the IEND chunk.\n");
        destroy_png(p_png);
        return NULL;
    }

    return p_png;

}

void destroy_png(simple_PNG_p p_png) {
    if (p_png) {
        if (p_png->p_png_buffer) {
            free(p_png->p_png_buffer);
        }

        if (p_png->p_IHDR) {
            destroy_chunk(p_png->p_IHDR);
        }
        if (p_png->p_IDAT) {
            destroy_chunk(p_png->p_IDAT);
        }
        if (p_png->p_IEND) {
            destroy_chunk(p_png->p_IEND);
        }

        free(p_png);
    }
}


chunk_p create_chunk() {
    chunk_p p_chunk = malloc(sizeof(struct chunk));
    p_chunk->length = 0;
    for (int i=0; i<CHUNK_TYPE_SIZE; i++) {
        p_chunk->type[i] = 0;
    }
    p_chunk->p_data = NULL;
    p_chunk->crc = 0;
    p_chunk->p_data_IHDR = NULL;
    p_chunk->is_corrupted = 0;

    return p_chunk;
}

void destroy_chunk(chunk_p p_chunk) {
    if (p_chunk) {
        if (p_chunk->p_data_IHDR) {
            free(p_chunk->p_data_IHDR);
        }
        free(p_chunk);
    } else {
        perror("p_chunk is a null pointer!\n");
    }
}

U8* read_buf(const char* path, U64* p_buf_length) {
    FILE *file = fopen(path, "rb");

    if (file == NULL) { // The file doesn't exist
        perror("Failed to open the file\n"); 
        return NULL;
    }

    // Get the size of the file
    fseek(file, 0, SEEK_END);
    U64 f_length = ftell(file);
    fseek(file, 0, SEEK_SET);

    /**
     *  Allocate the memory for the buffer.
     *  The buffer will be destoried in the destory_png method
     */
    U8* p_buf = (U8*) malloc(f_length);
    
    if (fread(p_buf, sizeof(U8), f_length, file) != f_length) {
        // Handle read error here, e.g., close file, free buffer, return NULL.
        perror("Failed to read the entire file.\n");
        free(p_buf);
        fclose(file);
        return NULL;
    }

    fclose(file);

    // Return the buffer length
    if (p_buf_length) {
        *p_buf_length = f_length;
    }

    return p_buf;
}

int read_chunk( U8* p_chunk_start, 
                U64 buf_length, 
                chunk_p p_chunk, 
                U8** p_next_chunk_start, 
                U64* p_remained_buffer_size) {
    if (buf_length < CHUNK_LEN_SIZE + CHUNK_TYPE_SIZE) {
        perror("The chunk size is too small.\n");
        return 0;
    }

    if (!p_chunk) {
        perror("p_chunk should be allocated in advanced.\n");
        return 0;
    }
    
    // Read the chunk length and set the pointer to the start of the chunk.
    p_chunk->length = read_big_endian_u32(p_chunk_start);
    U64 chunk_size = CHUNK_LEN_SIZE + CHUNK_TYPE_SIZE + p_chunk->length + CHUNK_CRC_SIZE;

    if (buf_length < chunk_size) {
        perror("The chunk buffer doesn't content the expected len of the data.\n");
        return 0;
    }
    p_chunk->p_data = p_chunk_start + CHUNK_LEN_SIZE + CHUNK_TYPE_SIZE;

    // Copy the chunk type.
    memcpy(p_chunk->type, p_chunk_start + CHUNK_LEN_SIZE, CHUNK_TYPE_SIZE);

    // If the chunk is an IHDR chunk.
    if (memcmp(p_chunk->type, TYPE_IHDR, CHUNK_TYPE_SIZE) == 0) {
        if (p_chunk->length == DATA_IHDR_SIZE) {
            p_chunk->p_data_IHDR = read_IHDR(p_chunk->p_data);
        } else {
            perror("The chunk has type IHDR but the length is not 13 bytes.\n");
            return 0;
        }
    }

    // Finalized by reading the p_chunk crc.
    p_chunk->crc = read_big_endian_u32(p_chunk->p_data + p_chunk->length);
    p_chunk->is_corrupted = p_chunk->crc != verify_crc(p_chunk);
 
    if (p_next_chunk_start) {
        *p_next_chunk_start = p_chunk_start + chunk_size;
    }
    if (p_remained_buffer_size) {
        *p_remained_buffer_size = buf_length - chunk_size;
    }

    return 1;
} 

data_IHDR_p read_IHDR(U8* p_data) {
    /* This pointer will be freed in destroy_chunk */
    data_IHDR_p p_IHDR = malloc(sizeof(struct data_IHDR));

    p_IHDR->width = read_big_endian_u32(p_data);
    p_IHDR->height = read_big_endian_u32(p_data + 4);
    p_IHDR->bit_depth = p_data[8];
    p_IHDR->color_type = p_data[9];
    p_IHDR->compression = p_data[10];
    p_IHDR->filter = p_data[11];
    p_IHDR->interlace = p_data[12];
    
    return p_IHDR;
}

U32 read_big_endian_u32(U8* p_u8) {
    U32 len = 0;

    /** 
     * XXX: Zhaoch23
     *      The png file is encoded in big-endianness 
     *      whereas some os are working on little-endianness.
     *      Therefore byte shifting cast is more properiate than 
     *      pointer cast.
     */
    for (int i=0; i<CHUNK_LEN_SIZE; i++) {
        len = len << 8; // Shift a byte for a new byte
        len = len | p_u8[i]; // Add the new byte
    }
    return len;
}

U32 verify_crc(chunk_p p_chunk) {
    if (!crc_table_computed) {
        make_crc_table();
    }

    // Allocate a buffer and copy the type and chunk data for computing the crc.
    int buf_size = CHUNK_TYPE_SIZE + p_chunk->length;
    U8* buf = (U8*) malloc(buf_size);
    memcpy(buf, p_chunk->type, CHUNK_TYPE_SIZE);
    memcpy(buf + CHUNK_TYPE_SIZE, p_chunk->p_data, p_chunk->length);

    // Compute the crc.
    U32 crc_ = crc(buf, buf_size);

    // Free the temp buffer.
    free(buf);

    return crc_;
}

void write_big_endian_u32(U8* p_chunk_start, U32 value) {
    for (int i=0; i<4; i++) {
        p_chunk_start[i] = (value >> ((3 - i) * 8)) & 0xFF;
    }
}
