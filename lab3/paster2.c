#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/time.h>

#include "urlutils.h"
#include "shm_utils.h"
#include "lab_png.h"
#include "catpng.h"
#include "consumer_producer.h"

// Comment this out
#define DEBUG

int main(int argc, char* argv[]) {
    if (argc != 6) {
        fprintf(stderr, "Usage: %s <B> <P> <C> <X> <N>\n", argv[0]);
        exit(1);
    }

    // Measure execution time
    struct timeval start, end;
    gettimeofday(&start, NULL);

    int B = atoi(argv[1]); // The max size of the recv stack
    int P = atoi(argv[2]); // The number of producers
    int C = atoi(argv[3]); // The number of consumers
    int X = atoi(argv[4]); // The time in milliseconds for the consumer to sleep
    int N = atoi(argv[5]); // The image id

    #ifdef DEBUG
    printf("B: %d\n", B);
    printf("P: %d\n", P);
    printf("C: %d\n", C);
    printf("X: %d\n", X);
    printf("N: %d\n", N);
    #endif

    init_shared_mem(B, C);

    // Fork the producers
    for (int i=0; i<P; i++) {
        pid_t pid = fork();
        if (pid > 0) {
            // parent
            #ifdef DEBUG
            printf("Parent: forked child %d.\n", pid);
            #endif
        } else if (pid == 0) {
            // child
            produce(URL_LIST[i % NUM_URLS], N);
        } else {
            perror("fork");
            abort();
        }
    }

    // Fork the consumers
    for (int i=0; i<C; i++) {
        pid_t pid = fork();
        if (pid > 0) {
            #ifdef DEBUG
            printf("Parent: forked child %d.\n", pid);
            #endif
            // parent
        } else if (pid == 0) {
            // child
            consume(X);
        } else {
            perror("fork");
            abort();
        }
    }

    // Wait for all children to finish
    int status;
    pid_t wpid;
    for (int i=0; i<P+C; i++) {
        wpid = wait(&status);
        #ifdef DEBUG
        printf("Parent: child exited: %d\n", wpid);
        #endif
    }
    simple_PNG_p* all_pngs = get_all_png_from_shm_array();
    simple_PNG_p p_png = concatenate_pngs(all_pngs, NUM_SEGMENTS);
    if (p_png == NULL) {
        perror("concatenate_pngs");
        abort();
    }

    write_png(p_png, "all.png");
    destroy_png(p_png);

    cleanup_shared_mem();

    // Measure end time
    gettimeofday(&end, NULL);
    double execution_time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1e6;
    printf("paster2 execution time: %.2f seconds\n", execution_time);

    return 0;
}
