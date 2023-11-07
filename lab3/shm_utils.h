#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>

#include "lab_png.h"
#include "shm_stack.h"
#include "urlutils.h"
#include "paster.h"

typedef struct _shm_holder {
    int _shmid;         // The shared memory id
    void* _shmaddr;     // The shared memory address
    size_t _size;       // Size of the shared memory
} _shm_holder_t, *_shm_holder_p;

/**
 * @brief The shared memory table for all processes
 * @note The shmid will not be changed after init and containes no dynamic memory
*/
typedef struct _shared_mem {
    _shm_holder_t _shmid_sem_spaces_counting;    // Track the size of the recv stack
                                               // Init to 0, increased by producer, decreased by consumer
    _shm_holder_t _shmid_sem_items_counting;   // Track the B - size of the recv stack
                                               // Init to B, increased by consumer, decreased by producer
    _shm_holder_t _shmid_sem_stack_mutex;      // Binary semaphore for accessing the recv stack
    _shm_holder_t _shmid_sem_png_array_mutex;  // Binary semaphore for accessing the png array and count
    _shm_holder_t _shmid_recv_stack;           // The recv stack to save the received pngs buffers
    _shm_holder_t _shmid_png_array;            // The png array to save the pngs
                                               // The actual array is in the shared memory.
                                               // The array is of size NUM_SEGMENTS * _shm_holder_t.
                                               // The actual buffer of the pngs are dynamically allocated in shared memory
    _shm_holder_t _shmid_png_count;            // The number of pngs received and the next index to download
} _shared_mem_t, *_shared_mem_p;

extern _shared_mem_t _shared_mem;

void _attach_shm(_shm_holder_p shm_holder);

void _detach_shm(_shm_holder_p shm_holder);

void _init_shm(_shm_holder_p shm_holder, size_t size);

void _cleanup_shm(_shm_holder_p shm_holder);

void _init_shm_sem(_shm_holder_p shm_holder, int value);

void _cleanup_shm_sem(_shm_holder_p shm_holder);

/**
 * @brief Initialize the shared memory table for all processes
 * @param[in] max_stack_size The max size of the recv stack
 * @note  This should be initilized in the main process before fork
*/
void init_shared_mem(int max_stack_size);

/**
 * @brief Cleanup the shared memory table for all processes
 * @note  This should be called in the main process after all child processes exit
*/
void cleanup_shared_mem();

/**
 * @brief Add a copy of png to the png array in shared memory
 * @param[in] index The index of the png in the png 
 *                  array from 0 to NUM_SEGMENTS - 1
 * @param[in] png The png to add
 * @note THIS FUNCTION IS NOT THREAD SAFE. The caller should 
 *       not be wrting to the same index more than one time.
 *       OTHERWISE THERE WILL BE MEMORY LEAK.
 * @note The png array will NOT hold a reference to the png.
*/
void add_png_to_shm_array(int index, simple_PNG_p png);

/**
 * @brief Push a copy of recv buffer to the recv stack in shared memory
 * @param[in] recv_buf The recv buffer to push
 * @note This function is thread safe. The process will block
 *      if the recv stack is full and wait for the consumer to
 *      pop a recv buffer.
 * @note The stack will NOT hold a reference to the recv buffer.
*/
void push_recv_buf_to_shm_stack(Recv_buf_p recv_buf);

/**
 * @brief Get all pngs from the png array in shared memory
 * @return The DYNAMICALLY ALOCATED COPIES IN HEAP of all pngs
 * @note THIS IS NOT THREAD SAFE. The caller should not be
 *       calling this function after all child processes exit.
 * @note The caller is responsible to call the destroy_png method 
 *       to prevent memory leak.
*/
simple_PNG_p* get_all_png_from_shm_array();

/**
 * @brief Pop a recv buffer from the recv stack in shared memory
 * @return The DYNAMICALLY ALLOCATED COPY IN HEAP of the recv buffer
 * @note This function is thread safe. The process will block 
 *       if the recv stack is empty and wait for the producer to
 *       push a recv buffer.
 * @note The caller is responsible to call the destroy_recv_buf method
 *       to prevent memory leak.
*/
Recv_buf_p pop_recv_buf_from_shm_stack();

/**
 * @brief Get the next image segment index to download in the shared memory
 * @return The next image segment index to download
 *         >= 0 if there is still image segment to download;
 *         -1 if there is no more image segment to download and the producer should exit
 * @note This function is thread safe and is protected by a 
 *       binary semaphore. Every time this function is called,
 *       the index will be increased by 1.
*/
int get_download_segment_index();