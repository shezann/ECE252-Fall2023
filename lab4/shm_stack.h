
/* The code is 
 * Copyright(c) 2018-2019 Yiqing Huang, <yqhuang@uwaterloo.ca>.
 *
 * This software may be freely redistributed under the terms of the X11 License.
 */
/**
 * @brief  stack to push/pop integer(s), API header  
 * @author yqhuang@uwaterloo.ca
 */
#include "stdlib.h"
#include "urlutils.h"

typedef struct recv_stack
{
    int size;               /* the max capacity of the stack */
    int pos;                /* position of last item pushed onto the stack */
    unsigned char* items;   /* stack of stored recv buf */
} RecvStack;

size_t sizeof_shm_stack(int size);
int init_shm_stack(RecvStack *p, int stack_size);
int is_full(RecvStack *p);
int is_empty(RecvStack *p);
int push(RecvStack *p, Recv_buf_p item);
int pop(RecvStack *p, Recv_buf_p *p_item);