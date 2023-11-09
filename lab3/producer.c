#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include "producer.h"

// Define the buffer to store downloaded image segments
ImageSegment buffer[MAX_IMAGE_SEGMENTS];

// Define semaphore IDs
int sem_id;

// Simulated function to download an image segment from the server
// In a real implementation, you would use network requests to fetch image segments.
int download_image_segment(int sequence_number, int image_number, char *data)
{
    // Simulated delay to simulate download time
    usleep(10000); // Sleep for 10 milliseconds
    // Simulated data (replace with actual download logic)
    snprintf(data, IMAGE_SEGMENT_SIZE, "Image data for image %d, segment %d", image_number, sequence_number);
    return 0; // Simulated success
}

// Initialize and manage semaphores
void init_semaphores()
{
    // Create or access the semaphore set (key should be unique)
    sem_id = semget(IPC_PRIVATE, 1, IPC_CREAT | 0600);
    if (sem_id == -1)
    {
        perror("Failed to create semaphore");
        exit(1);
    }
    // Initialize the semaphore to 1 (unlocked)
    if (semctl(sem_id, 0, SETVAL, 1) == -1)
    {
        perror("Failed to initialize semaphore");
        exit(1);
    }
}

// Wait (lock) the semaphore
void wait_semaphore()
{
    struct sembuf sops = {0, -1, 0};
    if (semop(sem_id, &sops, 1) == -1)
    {
        perror("Failed to wait for semaphore");
        exit(1);
    }
}

// Signal (unlock) the semaphore
void signal_semaphore()
{
    struct sembuf sops = {0, 1, 0};
    if (semop(sem_id, &sops, 1) == -1)
    {
        perror("Failed to signal semaphore");
        exit(1);
    }
}

// int main(int argc, char *argv[]) {
//     if (argc != 2) {
//         fprintf(stderr, "Usage: %s <image_number>\n", argv[0]);
//         exit(1);
//     }

//     int image_number = atoi(argv[1]);

//     // Initialize and manage semaphores
//     init_semaphores();

//     // Implement the logic to download image segments
//     for (int sequence_number = 0; sequence_number < MAX_IMAGE_SEGMENTS; sequence_number++) {
//         ImageSegment segment;

//         wait_semaphore();  // Lock the semaphore before accessing the buffer

//         if (download_image_segment(sequence_number, image_number, segment.data) == 0) {
//             segment.sequence_number = sequence_number;
//             segment.image_number = image_number;

//             buffer[sequence_number] = segment;

//             printf("Producer: Downloaded image segment %d for image %d\n", sequence_number, image_number);
//         } else {
//             fprintf(stderr, "Producer: Failed to download image segment %d for image %d\n", sequence_number, image_number);
//             // Handle the error gracefully, e.g., by retrying or exiting
//         }

//         signal_semaphore();  // Unlock the semaphore after accessing the buffer
//     }

//     // Additional logic may be needed to signal that the producer has finished downloading

//     // Cleanup semaphores
//     if (semctl(sem_id, 0, IPC_RMID) == -1) {
//         perror("Failed to remove semaphore");
//         exit(1);
//     }

//     return 0;
// }