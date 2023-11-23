/*
 * The code is derived from 
 * Copyright(c) 2018-2019 Yiqing Huang, <yqhuang@uwaterloo.ca>.
 *
 * This software may be freely redistributed under the terms of the X11 License.
 */

/**
 * @brief  stack to push/pop integers.   
 */

#include <stdio.h>
#include <stdlib.h>
#include "shm_stack.h"

/* a stack that can hold integers */
/* Note this structure can be used by shared memory,
   since the items field points to the memory right after it.
   Hence the structure and the data items it holds are in one
   continuous chunk of memory.

   The memory layout:
   +===============+
   | size          | 4 bytes
   +---------------+
   | pos           | 4 bytes
   +---------------+
   | items         | 8 bytes
   +---------------+
   | items[0]      | SHM_BUF_SIZE
   +---------------+
   | items[1]      | SHM_BUF_SIZE
   +---------------+
   | ...           | SHM_BUF_SIZE
   +---------------+
   | items[size-1] | SHM_BUF_SIZE
   +===============+
*/


/**
 * @brief calculate the total memory that the struct int_stack needs and
 *        the items[size] needs.
 * @param int size maximum number of integers the stack can hold
 * @return return the sum of RecvStack size and the size of the data that
 *         items points to.
 */

size_t sizeof_shm_stack(int size)
{
    return (sizeof(RecvStack) + SHM_BUF_SIZE * (size_t)size);
}

/**
 * @brief initialize the RecvStack member fields.
 * @param RecvStack *p points to the starting addr. of an RecvStack struct
 * @param int stack_size max. number of items the stack can hold
 * @return 0 on success; non-zero on failure
 * NOTE:
 * The caller first calls sizeof_shm_stack() to allocate enough memory;
 * then calls the init_shm_stack to initialize the struct
 */
int init_shm_stack(RecvStack *p, int stack_size)
{
    if ( p == NULL || stack_size == 0 ) {
        return 1;
    }

    p->size = stack_size;
    p->pos  = -1;
    p->items = (unsigned char *) ((char *)p + sizeof(RecvStack));
    return 0;
}


/**
 * @brief check if the stack is full
 * @param RecvStack *p the address of the RecvStack data structure
 * @return non-zero if the stack is full; zero otherwise
 */

int is_full(RecvStack *p)
{
    if ( p == NULL ) {
        return 0;
    }
    return ( p->pos == (p->size -1) );
}

/**
 * @brief check if the stack is empty 
 * @param RecvStack *p the address of the RecvStack data structure
 * @return non-zero if the stack is empty; zero otherwise
 */

int is_empty(RecvStack *p)
{
    if ( p == NULL ) {
        return 0;
    }
    return ( p->pos == -1 );
}

/**
 * @brief push one integer onto the stack 
 * @param RecvStack *p the address of the RecvStack data structure
 * @param Recv_buf_p item the integer to be pushed onto the stack 
 * @return 0 on success; non-zero otherwise
 */

int push(RecvStack *p, Recv_buf_p item)
{
    if ( p == NULL ) {
        return -1;
    }

    if ( !is_full(p) ) {
        ++(p->pos);
        //printf("push %d, %d, %ld, %p\n", p->pos, item->seq, item->size, p->items + p->pos * SHM_BUF_SIZE);
        unsigned char* dest_buf = p->items + p->pos * SHM_BUF_SIZE;
        memcpy(dest_buf, item, sizeof(Recv_buf_t));
        memcpy(dest_buf + sizeof(Recv_buf_t), item->buf, item->max_size);
        ((Recv_buf_p) dest_buf)->buf = dest_buf + sizeof(Recv_buf_t);
        // unsigned char* ptr = dest_buf;
        // for (int i=0; i<512; i++) {
        //      printf("%02x ", ptr[i]);
        // }
        return 0;
    } else {
        return -1;
    }
}

/**
 * @brief push one integer onto the stack 
 * @param RecvStack *p the address of the RecvStack data structure
 * @param int *item output parameter to save the integer value 
 *        that pops off the stack 
 * @return 0 on success; non-zero otherwise
 */
int pop(RecvStack *p, Recv_buf_p *p_item)
{
    if ( p == NULL ) {
        return -1;
    }

    if ( !is_empty(p) ) {
        // Now only gods know what I am doing here
        Recv_buf_p p_recv_buf = ((Recv_buf_p) (p->items + p->pos * SHM_BUF_SIZE));

        // unsigned char* ptr = (unsigned char*)p_recv_buf;
        // for (int i=0; i<512; i++) {
        //      printf("%02x ", ptr[i]);
        // }
        // printf("\n");
        
        *p_item = create_recv_buf();
        (*p_item)->buf = (unsigned char *) malloc(p_recv_buf->max_size);
        memcpy((*p_item)->buf,
            p->items + p->pos * SHM_BUF_SIZE + sizeof(Recv_buf_t), 
            BUF_SIZE
            );
        (*p_item)->size = p_recv_buf->size;
        (*p_item)->max_size = p_recv_buf->max_size;
        (*p_item)->seq = p_recv_buf->seq;
        
        (p->pos)--;
        return 0;
    } else {
        return 1;
    }
}