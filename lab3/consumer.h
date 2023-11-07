#ifndef CONSUMER_H
#define CONSUMER_H

// Define constants and data structures related to the consumer module

#define MAX_IMAGE_SEGMENTS 50 // Maximum number of image segments
#define IMAGE_SEGMENT_SIZE 10000 // Size of an image segment in bytes

// Data structure for an image segment
typedef struct {
    int sequence_number;
    int image_number;
    char data[IMAGE_SEGMENT_SIZE];
} ImageSegment;

// Function to process and validate an image segment
// Arguments:
//   segment: A pointer to the image segment to be processed
// Returns:
//   0 on success, -1 on failure
int process_image_segment(ImageSegment* segment);

#endif /* CONSUMER_H */