#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <pthread.h>
#include "consumer.h"

// Define constants and data structures related to shared memory and synchronization
#define SHM_KEY 1234
#define SEM_KEY 5678

// Shared memory segment key and ID
key_t shm_key = SHM_KEY;
int shm_id;

// Define the shared buffer
ImageSegment *buffer = NULL;

// Define synchronization objects
pthread_mutex_t buffer_mutex;
pthread_cond_t buffer_not_empty;

// Simulated function to validate and process an image segment
// In a real implementation, you would implement image validation and processing logic.
int process_image_segment(ImageSegment *segment)
{
    // Simulated validation and processing (replace with actual logic)
    if (segment == NULL)
    {
        return -1; // Error: Invalid segment
    }

    // Simulated processing (printing the image data)
    printf("Processing image segment %d for image %d: %s\n", segment->sequence_number, segment->image_number, segment->data);

    return 0; // Simulated success
}

// Initialize shared memory and synchronization objects
void initialize_objects()
{
    // Attach to the shared buffer
    shm_id = shmget(shm_key, MAX_IMAGE_SEGMENTS * sizeof(ImageSegment), 0);
    if (shm_id == -1)
    {
        perror("Failed to access shared memory");
        exit(1);
    }

    buffer = shmat(shm_id, NULL, 0);
    if (buffer == (ImageSegment *)-1)
    {
        perror("Failed to attach to shared memory");
        exit(1);
    }

    // Initialize mutex and conditional variable
    if (pthread_mutex_init(&buffer_mutex, NULL) != 0)
    {
        perror("Failed to initialize mutex");
        exit(1);
    }

    if (pthread_cond_init(&buffer_not_empty, NULL) != 0)
    {
        perror("Failed to initialize conditional variable");
        exit(1);
    }
}

// Finalize and clean up objects
void finalize_objects()
{
    // Detach from shared memory
    shmdt(buffer);

    // Destroy mutex and conditional variable
    pthread_mutex_destroy(&buffer_mutex);
    pthread_cond_destroy(&buffer_not_empty);
}

// Consumer thread function
void *consumer_thread(void *arg)
{
    int sequence_number = *(int *)arg;

    while (1)
    {
        // Access the shared buffer and perform processing
        ImageSegment *segment;

        pthread_mutex_lock(&buffer_mutex);
        while (buffer[sequence_number].image_number == -1)
        {
            pthread_cond_wait(&buffer_not_empty, &buffer_mutex);
        }

        segment = &buffer[sequence_number];

        if (process_image_segment(segment) == 0)
        {
            printf("Consumer: Processed and validated image segment %d for image %d\n", sequence_number, segment->image_number);
            // Mark the segment as processed
            segment->image_number = -1;
        }
        else
        {
            fprintf(stderr, "Consumer: Failed to process and validate image segment %d for image %d\n", sequence_number, segment->image_number);
            // Handle the error gracefully, e.g., by retrying or exiting
        }

        pthread_mutex_unlock(&buffer_mutex);
    }
}

// int main(int argc, char* argv[]) {
//     if (argc != 1) {
//         fprintf(stderr, "Usage: %s\n", argv[0]);
//         exit(1);
//     }

//     // Initialize shared memory and synchronization objects
//     initialize_objects();

//     // Create and start consumer threads
//     pthread_t threads[MAX_IMAGE_SEGMENTS];
//     int sequence_numbers[MAX_IMAGE_SEGMENTS];

//     for (int i = 0; i < MAX_IMAGE_SEGMENTS; i++) {
//         sequence_numbers[i] = i;
//         if (pthread_create(&threads[i], NULL, consumer_thread, &sequence_numbers[i]) != 0) {
//             perror("Failed to create consumer thread");
//             exit(1);
//         }
//     }

//     // Wait for consumer threads to finish (this is a simplified way to wait indefinitely)
//     for (int i = 0; i < MAX_IMAGE_SEGMENTS; i++) {
//         if (pthread_join(threads[i], NULL) != 0) {
//             perror("Failed to join consumer thread");
//             exit(1);
//         }
//     }

//     // Finalize and clean up objects
//     finalize_objects();

//     return 0;
// }