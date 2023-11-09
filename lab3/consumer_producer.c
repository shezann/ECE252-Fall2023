#include "consumer_producer.h"

void consume(size_t time_to_sleep) {
    while (1) {
        sleep(time_to_sleep);
        #ifdef DEBUG
        printf("Consumer: woke up: %d\n", getpid());
        #endif
        // Get the data
        Recv_buf_p p_recv_buf = pop_recv_buf_from_shm_stack();

        if (p_recv_buf == NULL) {
            exit(0);
        }

        simple_PNG_p p_png = create_png_from_buf(p_recv_buf->buf, p_recv_buf->size);

        // Save to the shm
        add_png_to_shm_array(p_recv_buf->seq, p_png);

        destroy_png(p_png);
        destroy_recv_buf(p_recv_buf);

        #ifdef DEBUG
        printf("Consumer: sleeped: %d\n", getpid());
        #endif
    }

}

void produce(char* baseurl, int image_id) {
    while (1) {
        #ifdef DEBUG
        printf("Producer: woke up: %d\n", getpid());
        #endif
        int image_part = get_download_segment_index();
        if (image_part == -1) {
            exit(0);
        }
        CURL* curl = get_image_url_handle(baseurl, image_id, image_part);

        Recv_buf_p p_recv_buf = fetch_url(curl);
        if (p_recv_buf == NULL) {
            perror("fetch_url");
            abort();
        }

        push_recv_buf_to_shm_stack(p_recv_buf);

        curl_easy_cleanup(curl);
        destroy_recv_buf(p_recv_buf);
    }
}