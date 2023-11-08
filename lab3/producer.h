#ifndef PRODUCER_H
#define PRODUCER_H

// Define constants and data structures related to the producer module

#define MAX_IMAGE_SEGMENTS 50 // Maximum number of image segments
#define IMAGE_SEGMENT_SIZE 10000 // Size of an image segment in bytes

// Data structure for an image segment
typedef struct {
    int sequence_number;
    int image_number;
    char data[IMAGE_SEGMENT_SIZE];
} ImageSegment;

// Function to download an image segment from the server
// Arguments:
//   sequence_number: The sequence number of the image segment to download
//   image_number: The image number to request from the server
//   buffer: A pointer to the buffer where the downloaded image segment will be stored
// Returns:
//   0 on success, -1 on failure
int download_image_segment(int sequence_number, int image_number, char* buffer);

#endif /* PRODUCER_H */