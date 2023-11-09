#include "consumer_producer.h"

void consume(size_t time_to_sleep) {
    size_t micro = time_to_sleep * 1000;
    while (1) {
        usleep(micro);
        #ifdef DEBUG
        printf("Consumer: woke up: %d\n", getpid());
        #endif
        // Get the data
        Recv_buf_p p_recv_buf = pop_recv_buf_from_shm_stack();
        #ifdef DEBUG
        printf("Consumer: Recv buf %p\n", p_recv_buf);
        #endif
        if (p_recv_buf == NULL) {
            return;
        }

        simple_PNG_p p_png = create_png_from_buf(p_recv_buf->buf, p_recv_buf->size);
        #ifdef DEBUG
        printf("Consumer: png %d, size: %ld\n", p_recv_buf->seq, p_recv_buf->size);
        #endif
        // Save to the shm
        add_png_to_shm_array(p_recv_buf->seq, p_png);

        destroy_png(p_png);
        
        free(p_recv_buf);

        #ifdef DEBUG
        printf("Consumer: sleeped: %d\n", getpid());
        #endif
    }

}

void produce(const char* baseurl, int image_id) {
    curl_init();
    while (1) {
        #ifdef DEBUG
        printf("Producer: woke up: %d\n", getpid());
        #endif
        int image_part = get_download_segment_index();
        #ifdef DEBUG
        printf("Producer: part: %d\n", image_part);
        #endif
        if (image_part == -1) {
            curl_cleanup();
            return;
        }
        Recv_buf_p p_recv_buf;
        CURL* curl = get_image_url_handle(baseurl, image_id, image_part);
        while (1) {
            p_recv_buf = fetch_url(curl);

            if (p_recv_buf == NULL) {
                #ifdef DEBUG
                perror("fetch_url");
                #endif
                continue;
            }
            
            break;
        }
        #ifdef DEBUG
        printf("Producer: seq %d, size: %ld\n", p_recv_buf->seq, p_recv_buf->size);
        #endif
        push_recv_buf_to_shm_stack(p_recv_buf);

        curl_easy_cleanup(curl);
        destroy_recv_buf(p_recv_buf);
    }
}