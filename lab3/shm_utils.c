#include "shm_utils.h"
// #define DEBUG

_shared_mem_t _shared_mem = {0};

void _init_shm(_shm_holder_p shm_holder, size_t size) {
    shm_holder->_size = size;
    shm_holder->_shmid = shmget(IPC_PRIVATE, size, IPC_CREAT | 0666);
    if (shm_holder->_shmid < 0) {
        perror("shmget");
        abort();
    }
    shm_holder->_shmaddr = NULL;
}

void _cleanup_shm(_shm_holder_p shm_holder) {
    if (shm_holder->_shmid > 0) {
        if (shmctl(shm_holder->_shmid, IPC_RMID, NULL) == -1) {
            perror("shmctl");
            abort();
        }
    }
}

void _attach_shm(_shm_holder_p shm_holder) {
    shm_holder->_shmaddr = shmat(shm_holder->_shmid, NULL, 0);
    if (shm_holder->_shmaddr == (void *) -1) {
        perror("shmat");
        abort();
    }
}

void _detach_shm(_shm_holder_p shm_holder) {
    if (shm_holder->_shmaddr != NULL) {
        if (shmdt(shm_holder->_shmaddr) != 0) {
            perror("shmdt");
            abort();
        }
        shm_holder->_shmaddr = NULL;
    }
}

void _init_shm_sem(_shm_holder_p shm_holder, int value) {
    _attach_shm(shm_holder);
    if (sem_init(shm_holder->_shmaddr, 1, value) != 0) {
        perror("sem_init");
        abort();
    }
    _detach_shm(shm_holder);
}

int _is_all_png_downloaded() {
    _attach_shm(&_shared_mem._shmid_sem_png_array_mutex);
    if (sem_wait(_shared_mem._shmid_sem_png_array_mutex._shmaddr) != 0) {
        perror("sem_wait");
        abort();
    }

    int res;
    int* count;
    int* num_consumers;

    _attach_shm(&_shared_mem._downloaded_count);
    _attach_shm(&_shared_mem._num_consumers);
    count = (int*) _shared_mem._downloaded_count._shmaddr;

    num_consumers = (int*) _shared_mem._num_consumers._shmaddr;
    
    // printf("Requiring to download: %d\n", *count + 1);

    if (*num_consumers <= 0) {
        res = 1; 
        // If the number of consumers are more
        // than the number of pngs left to download
        // We shall terminate one consumer
    } else if (*count >= NUM_SEGMENTS) {
        *num_consumers -= 1;
        res = 1;
    } else {
        *count += 1;
        res = 0;
    }

    #ifdef DEBUG
    printf("Res: %d\n", res);
    #endif

    _detach_shm(&_shared_mem._num_consumers);
    _detach_shm(&_shared_mem._downloaded_count);

    // Signal that the png array is not being accessed
    if (sem_post(_shared_mem._shmid_sem_png_array_mutex._shmaddr) != 0) {
        perror("sem_post");
        abort();
    }
    _detach_shm(&_shared_mem._shmid_sem_png_array_mutex);

    return res;
}

void init_shared_mem(int max_stack_size, int num_consumers) {
    // Init the semaphores
    _init_shm(&_shared_mem._shmid_sem_spaces_counting, sizeof(sem_t));
    _init_shm(&_shared_mem._shmid_sem_items_counting, sizeof(sem_t));
    _init_shm(&_shared_mem._shmid_sem_stack_mutex, sizeof(sem_t));
    _init_shm(&_shared_mem._shmid_sem_png_array_mutex, sizeof(sem_t));
    _init_shm(&_shared_mem._shmid_sem_download_mutex, sizeof(sem_t));

    _init_shm_sem(&_shared_mem._shmid_sem_spaces_counting, max_stack_size);
    _init_shm_sem(&_shared_mem._shmid_sem_items_counting, 0);
    _init_shm_sem(&_shared_mem._shmid_sem_stack_mutex, 1);
    _init_shm_sem(&_shared_mem._shmid_sem_png_array_mutex, 1);
    _init_shm_sem(&_shared_mem._shmid_sem_download_mutex, 1);

    // Init the png array to be empty
    _init_shm(&_shared_mem._shmid_png_array, NUM_SEGMENTS * sizeof(_shm_holder_t));
    _attach_shm(&_shared_mem._shmid_png_array);
    _shm_holder_p shm_holder = (_shm_holder_p) _shared_mem._shmid_png_array._shmaddr;
    for (int i = 0; i < NUM_SEGMENTS; i++) {
        shm_holder[i]._shmaddr = NULL;
        shm_holder[i]._shmid = -1;
        shm_holder[i]._size = 0;
    }
    _detach_shm(&_shared_mem._shmid_png_array);

    _init_shm(&_shared_mem._shmid_recv_stack, sizeof_shm_stack(max_stack_size));
    _attach_shm(&_shared_mem._shmid_recv_stack);
    init_shm_stack((RecvStack*) _shared_mem._shmid_recv_stack._shmaddr, max_stack_size);
    _detach_shm(&_shared_mem._shmid_recv_stack);

    _init_shm(&_shared_mem._shmid_png_count, sizeof(int));
    _attach_shm(&_shared_mem._shmid_png_count);
    *((int*) _shared_mem._shmid_png_count._shmaddr) = 0;
    _detach_shm(&_shared_mem._shmid_png_count);

    _init_shm(&_shared_mem._num_consumers, sizeof(int));
    _attach_shm(&_shared_mem._num_consumers);
    *((int*) _shared_mem._num_consumers._shmaddr) = num_consumers;
    _detach_shm(&_shared_mem._num_consumers);

    _init_shm(&_shared_mem._downloaded_count, sizeof(int));
    _attach_shm(&_shared_mem._downloaded_count);
    *((int*) _shared_mem._downloaded_count._shmaddr) = 0;
    _detach_shm(&_shared_mem._downloaded_count);
}

void cleanup_shared_mem() {
    // Cleanup the semaphores
    
    _cleanup_shm(&_shared_mem._shmid_sem_spaces_counting);
    _cleanup_shm(&_shared_mem._shmid_sem_items_counting);
    _cleanup_shm(&_shared_mem._shmid_sem_stack_mutex);
    _cleanup_shm(&_shared_mem._shmid_sem_png_array_mutex);
    _cleanup_shm(&_shared_mem._shmid_sem_download_mutex);

    // Cleanup the png array
    
    _attach_shm(&_shared_mem._shmid_png_array);
    _shm_holder_p shm_holder = (_shm_holder_p) _shared_mem._shmid_png_array._shmaddr;
    for (int i = 0; i < NUM_SEGMENTS; i++) {
        _cleanup_shm(&shm_holder[i]); // Cleanup the dynamic memory in the shared memory
    }
    _detach_shm(&_shared_mem._shmid_png_array);
    _cleanup_shm(&_shared_mem._shmid_png_array);

    _cleanup_shm(&_shared_mem._shmid_recv_stack);

    _cleanup_shm(&_shared_mem._shmid_png_count);

    _cleanup_shm(&_shared_mem._num_consumers);
    _cleanup_shm(&_shared_mem._downloaded_count);
}

// This is dumb, but I don't know how to do it better
void add_png_to_shm_array(int index, simple_PNG_p png) {
    // Get the array in the shared memory    
    _attach_shm(&_shared_mem._shmid_png_array);
    _shm_holder_p shm_holder = (_shm_holder_p) _shared_mem._shmid_png_array._shmaddr;

    _init_shm(&shm_holder[index], png->buf_length);
    _attach_shm(&shm_holder[index]);
    #ifdef DEBUG
    printf("index: %d, %d\n", index, shm_holder[index]._shmid);
    #endif
    memcpy(shm_holder[index]._shmaddr, png->p_png_buffer, png->buf_length);
    _detach_shm(&shm_holder[index]);

    _detach_shm(&_shared_mem._shmid_png_array);
}

void push_recv_buf_to_shm_stack(Recv_buf_p recv_buf) {
    // Wait if the stack is full
    _attach_shm(&_shared_mem._shmid_sem_spaces_counting);
    if (sem_wait(_shared_mem._shmid_sem_spaces_counting._shmaddr) != 0) {
        perror("sem_wait");
        abort();
    }
    _detach_shm(&_shared_mem._shmid_sem_spaces_counting);

    // Wait if the stack is being accessed
    _attach_shm(&_shared_mem._shmid_sem_stack_mutex);
    if (sem_wait(_shared_mem._shmid_sem_stack_mutex._shmaddr) != 0) {
        perror("sem_wait");
        abort();
    }
    // Push the recv buffer to the stack
    _attach_shm(&_shared_mem._shmid_recv_stack);
    push((RecvStack*) _shared_mem._shmid_recv_stack._shmaddr, recv_buf);
    _detach_shm(&_shared_mem._shmid_recv_stack);

    // Signal that the stack is not being accessed
    if (sem_post(_shared_mem._shmid_sem_stack_mutex._shmaddr) != 0) {
        perror("sem_post");
        abort();
    }
    _detach_shm(&_shared_mem._shmid_sem_stack_mutex);

    // Signal that the stack get pushed
    _attach_shm(&_shared_mem._shmid_sem_items_counting);
    if (sem_post(_shared_mem._shmid_sem_items_counting._shmaddr) != 0) {
        perror("sem_post");
        abort();
    }
    _detach_shm(&_shared_mem._shmid_sem_items_counting);
}

simple_PNG_p* get_all_png_from_shm_array() {
    // An array in process heap to hold the pngs
    simple_PNG_p* pngs = (simple_PNG_p*) malloc(NUM_SEGMENTS * sizeof(simple_PNG_p));
    
    _attach_shm(&_shared_mem._shmid_png_array);
    // Get the pngs in the shared memory
    _shm_holder_p shm_holder = (_shm_holder_p) _shared_mem._shmid_png_array._shmaddr;
    
    for (int i = 0; i < NUM_SEGMENTS; i++) {
        #ifdef DEBUG
        printf("attach png %d\n", i);
        #endif
        _attach_shm(&shm_holder[i]);

        void* heap_buf = malloc(shm_holder[i]._size);
        memcpy(heap_buf, shm_holder[i]._shmaddr, shm_holder[i]._size);
        pngs[i] = (simple_PNG_p) create_png_from_buf(heap_buf, shm_holder[i]._size);
        
        _detach_shm(&shm_holder[i]);
    }
    _detach_shm(&_shared_mem._shmid_png_array);

    return pngs;
}

Recv_buf_p pop_recv_buf_from_shm_stack() {
    if (_is_all_png_downloaded() == 1) {
        return NULL;
    }
    // Wait if the stack is empty
    _attach_shm(&_shared_mem._shmid_sem_items_counting);
    if (sem_wait(_shared_mem._shmid_sem_items_counting._shmaddr) != 0) {
        perror("sem_wait");
        abort();
    }
    _detach_shm(&_shared_mem._shmid_sem_items_counting);

    // Wait if the stack is being accessed
    _attach_shm(&_shared_mem._shmid_sem_stack_mutex);
    if (sem_wait(_shared_mem._shmid_sem_stack_mutex._shmaddr) != 0) {
        perror("sem_wait");
        abort();
    }

    // Pop the recv buffer from the stack
    _attach_shm(&_shared_mem._shmid_recv_stack);
    Recv_buf_p recv_buf = NULL;
    pop((RecvStack*) _shared_mem._shmid_recv_stack._shmaddr, &recv_buf);
    _detach_shm(&_shared_mem._shmid_recv_stack);

    // Signal that the stack is not being accessed
    if (sem_post(_shared_mem._shmid_sem_stack_mutex._shmaddr) != 0) {
        perror("sem_post");
        abort();
    }
    _detach_shm(&_shared_mem._shmid_sem_stack_mutex);

    // Signal that the stack get popped
    _attach_shm(&_shared_mem._shmid_sem_spaces_counting);
    if (sem_post(_shared_mem._shmid_sem_spaces_counting._shmaddr) != 0) {
        perror("sem_post");
        abort();
    }
    _detach_shm(&_shared_mem._shmid_sem_spaces_counting);

    return recv_buf;
}

int get_download_segment_index() {
    int* count;
    int res;
    _attach_shm(&_shared_mem._shmid_sem_download_mutex);
    if (sem_wait(_shared_mem._shmid_sem_download_mutex._shmaddr) != 0) {
        perror("sem_wait");
        abort();
    }
    _attach_shm(&_shared_mem._shmid_png_count);
    
    count = (int*) _shared_mem._shmid_png_count._shmaddr;
    if (*count >= NUM_SEGMENTS) {
        res = -1;
    } else {
        res = *count;
        *count += 1;
    }
    
    _detach_shm(&_shared_mem._shmid_png_count);

    if (sem_post(_shared_mem._shmid_sem_download_mutex._shmaddr) != 0) {
        perror("sem_post");
        abort();
    }
    // printf("Assigned to download %d\n", res);
    _detach_shm(&_shared_mem._shmid_sem_download_mutex);

    return res;
}