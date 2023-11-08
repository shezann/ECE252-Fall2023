#include "shm_utils.h"

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

void init_shared_mem(int max_stack_size) {
    // Init the semaphores
    _init_shm(&_shared_mem._shmid_sem_spaces_counting, sizeof(sem_t));
    _init_shm(&_shared_mem._shmid_sem_items_counting, sizeof(sem_t));
    _init_shm(&_shared_mem._shmid_sem_stack_mutex, sizeof(sem_t));
    _init_shm(&_shared_mem._shmid_sem_png_array_mutex, sizeof(sem_t));
    
    _init_shm_sem(&_shared_mem._shmid_sem_spaces_counting, 0);
    _init_shm_sem(&_shared_mem._shmid_sem_items_counting, max_stack_size);
    _init_shm_sem(&_shared_mem._shmid_sem_stack_mutex, 1);
    _init_shm_sem(&_shared_mem._shmid_sem_png_array_mutex, 1);

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
}

void cleanup_shared_mem() {
    // Cleanup the semaphores
    _cleanup_shm(&_shared_mem._shmid_sem_spaces_counting);
    _cleanup_shm(&_shared_mem._shmid_sem_items_counting);
    _cleanup_shm(&_shared_mem._shmid_sem_stack_mutex);
    _cleanup_shm(&_shared_mem._shmid_sem_png_array_mutex);

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
}

// This is dumb, but I don't know how to do it better
void add_png_to_shm_array(int index, simple_PNG_p png) {
    // Get the array in the shared memory
    _attach_shm(&_shared_mem._shmid_png_array);
    // not protected by sems, since shm_holder is only an intance of this process
    _shm_holder_p shm_holder = (_shm_holder_p) _shared_mem._shmid_png_array._shmaddr;

    // Init the shared memory for the png at index
    // This is a dynamic memory
    _init_shm(&shm_holder[index], png->buf_length);
    _attach_shm(&shm_holder[index]);

    memcpy(shm_holder[index]._shmaddr, png, png->buf_length);

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
    int res = -1;
    _attach_shm(&_shared_mem._shmid_sem_png_array_mutex);
    if (sem_wait(_shared_mem._shmid_sem_png_array_mutex._shmaddr) != 0) {
        perror("sem_wait");
        abort();
    }
    _attach_shm(&_shared_mem._shmid_png_count);
    res = (*((int*) _shared_mem._shmid_png_count._shmaddr))++;
    _detach_shm(&_shared_mem._shmid_png_count);

    if (sem_post(_shared_mem._shmid_sem_png_array_mutex._shmaddr) != 0) {
        perror("sem_post");
        abort();
    }

    _detach_shm(&_shared_mem._shmid_sem_png_array_mutex);

    // Check if the index is valid
    if (res >= NUM_SEGMENTS) {
        return -1;
    }

    return res;
}