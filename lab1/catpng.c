#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <zlib.h>

// Structure to store IHDR chunk data
struct IHDRChunk {
    uint32_t width;
    uint32_t height;
    uint8_t bit_depth;
    uint8_t color_type;
    uint8_t compression_method;
    uint8_t filter_method;
    uint8_t interlace_method;
};

// Function to read IHDR chunk from a PNG file
int read_ihdr_chunk(FILE *file, struct IHDRChunk *ihdr) {
    char chunk_type[4];
    if (fread(chunk_type, 1, 4, file) != 4 || memcmp(chunk_type, "IHDR", 4) != 0) {
        return 0; // Not an IHDR chunk
    }

    // Read IHDR data
    fread(&ihdr->width, sizeof(uint32_t), 1, file);
    fread(&ihdr->height, sizeof(uint32_t), 1, file);
    fread(&ihdr->bit_depth, sizeof(uint8_t), 1, file);
    fread(&ihdr->color_type, sizeof(uint8_t), 1, file);
    fread(&ihdr->compression_method, sizeof(uint8_t), 1, file);
    fread(&ihdr->filter_method, sizeof(uint8_t), 1, file);
    fread(&ihdr->interlace_method, sizeof(uint8_t), 1, file);

    return 1;
}

// Function to concatenate PNG files vertically
void concatenate_pngs(const char *output_file, const char **input_files, int num_input_files) {
    if (num_input_files == 0) {
        printf("No input PNG files specified.\n");
        return;
    }

    FILE *output = fopen(output_file, "wb");
    if (output == NULL) {
        perror("Error opening output file");
        return;
    }

    // Read the IHDR chunk from the first input PNG file
    struct IHDRChunk ihdr;
    FILE *first_input = fopen(input_files[0], "rb");
    if (first_input == NULL) {
        perror("Error opening input file");
        fclose(output);
        return;
    }

    if (!read_ihdr_chunk(first_input, &ihdr)) {
        fprintf(stderr, "Error: Invalid PNG file format in %s\n", input_files[0]);
        fclose(first_input);
        fclose(output);
        return;
    }

    // Initialize zlib for decompression
    z_stream stream;
    memset(&stream, 0, sizeof(stream));
    if (inflateInit(&stream) != Z_OK) {
        perror("Error initializing zlib");
        fclose(first_input);
        fclose(output);
        return;
    }

    // Concatenate pixel data
    uint8_t filter_byte;
    uint8_t buffer[1024]; // Adjust buffer size as needed
    size_t buffer_size = 0;

    for (int i = 0; i < num_input_files; i++) {
        FILE *input = fopen(input_files[i], "rb");
        if (input == NULL) {
            perror("Error opening input file");
            inflateEnd(&stream);
            fclose(output);
            return;
        }

        // Skip header and IDAT chunk markers
        fseek(input, 33, SEEK_SET);

        while (1) {
            size_t bytes_read = fread(&filter_byte, 1, 1, input);
            if (bytes_read == 0) {
                break; // End of file
            }

            // Read data into buffer
            size_t bytes_to_read = fread(buffer + buffer_size, 1, sizeof(buffer) - buffer_size, input);
            buffer_size += bytes_to_read;

            // Process data in buffer
            while (buffer_size > 0) {
                stream.next_in = buffer;
                stream.avail_in = buffer_size;
                stream.next_out = NULL;
                stream.avail_out = 0;

                if (inflate(&stream, Z_NO_FLUSH) != Z_OK) {
                    fprintf(stderr, "Error inflating data in %s\n", input_files[i]);
                    inflateEnd(&stream);
                    fclose(input);
                    fclose(output);
                    return;
                }

                // Write decompressed data to output
                fwrite(stream.next_out, 1, stream.avail_out, output);

                // Move remaining data to the beginning of the buffer
                memmove(buffer, buffer + (buffer_size - stream.avail_in), stream.avail_in);
                buffer_size = stream.avail_in;
            }
        }

        fclose(input);
    }

    // End zlib decompression
    inflateEnd(&stream);

    // Update IHDR chunk height
    uint32_t new_height = ihdr.height * num_input_files;
    ihdr.height = new_height;

    // Write the new PNG file
    fseek(output, 0, SEEK_SET);
    fwrite("\x89PNG\r\n\x1a\n", 1, 8, output);
    fwrite("IHDR", 1, 4, output);
    fwrite(&ihdr, sizeof(struct IHDRChunk), 1, output);

    // Close output file
    fclose(output);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s [PNG FILE]... [OUTPUT FILE]\n", argv[0]);
        return 1;
    }

    const char **input_files = (const char **)malloc((argc - 2) * sizeof(char *));
    for (int i = 1; i < argc - 1; i++) {
        input_files[i - 1] = argv[i];
    }

    const char *output_file = argv[argc - 1];
    concatenate_pngs(output_file, input_files, argc - 2);

    free(input_files);
    return 0;
}