#include "urlutils.h"
#include "shm_utils.h"
#include <sys/wait.h>
#include <sys/stat.h>

#define NUM_ITEMS 10

int main(int argc, char* argv[]) {
    init_shared_mem(NUM_ITEMS, 0);

    for (int i=0; i<NUM_ITEMS; i++) {
        pid_t pid = fork();
        if (pid > 0) {
            // parent
            printf("Parent: forked child %d.\n", pid);
        } else if (pid == 0) {
            // child
            Recv_buf_p p_recv_buf = create_recv_buf();
            p_recv_buf->size = 512;
            p_recv_buf->seq = 0;
            p_recv_buf->buf = malloc(p_recv_buf->max_size);

            sprintf((char *)p_recv_buf->buf, "child text: %d", getpid());

            push_recv_buf_to_shm_stack(p_recv_buf);
            destroy_recv_buf(p_recv_buf);

            exit(0);
        } else {
            perror("fork");
            abort();
        }
    }

    // Wait for all children to finish
    int status;
    pid_t wpid;
    for (int i=0; i<NUM_ITEMS; i++) {
        wpid = wait(&status);
        printf("Parent: child exited: %d\n", wpid);

    }
    for (int i=0; i<NUM_ITEMS; i++) {
        Recv_buf_p p_recv_buf = pop_recv_buf_from_shm_stack();
        printf("Parent: popped %s.\n", p_recv_buf->buf);
        destroy_recv_buf(p_recv_buf);
    }

    cleanup_shared_mem();

    return 0;
}