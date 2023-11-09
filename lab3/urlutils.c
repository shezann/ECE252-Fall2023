#include "urlutils.h"

const char* URL_LIST[NUM_URLS] = {
    "http://ece252-1.uwaterloo.ca:2530/image?",
    "http://ece252-2.uwaterloo.ca:2530/image?",
    "http://ece252-3.uwaterloo.ca:2530/image?"
};

Recv_buf_p create_recv_buf(void) {
    Recv_buf_p p_recv_buf = malloc(sizeof(struct recv_buf));
    if (p_recv_buf == NULL) {
        perror("Not enough memory.");
        return NULL;
    }
    p_recv_buf->buf = malloc(BUF_SIZE);
    if (p_recv_buf->buf == NULL) {
        perror("Not enough memory.");
        return NULL;
    }
    p_recv_buf->size = 0;
    p_recv_buf->max_size = BUF_SIZE;
    p_recv_buf->seq = -1;       /* valid seq should be non-negative */
    
    return p_recv_buf;
}

void destroy_recv_buf(Recv_buf_p p_recv_buf) {
    if (p_recv_buf) {
        if (p_recv_buf->buf) {
            free(p_recv_buf->buf);
        }
        free(p_recv_buf);
    }
}

void curl_init(void) {
    curl_global_init(CURL_GLOBAL_ALL);
}

void curl_cleanup(void) {
    curl_global_cleanup();
}

CURL* get_handle(const char* url) {
    CURL *curl_handle = curl_easy_init();

    if (curl_handle == NULL) {
        perror("curl_easy_init: returned NULL");
        return NULL;
    }

    /* specify URL to get */
    curl_easy_setopt(curl_handle, CURLOPT_URL, url);

    /* register write call back function to process received data */
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_cb_curl); 

    /* register header call back function to process received header data */
    curl_easy_setopt(curl_handle, CURLOPT_HEADERFUNCTION, header_cb_curl); 

    /* some servers requires a user-agent field */
    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");
    
    return curl_handle;
}

CURL *get_image_url_handle(const char* base_url, int image_id, int image_part) {
    char url[128];
    sprintf(url, "%simg=%d&part=%d", base_url, image_id, image_part);
    return get_handle(url);
}

Recv_buf_p fetch_url(CURL* curl_handle) {
    Recv_buf_p p_recv_buf = create_recv_buf();
    if (p_recv_buf == NULL) {
        return NULL;
    }

    /* user defined data structure passed to the call back function */
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)p_recv_buf);
    /* user defined data structure passed to the call back function */
    curl_easy_setopt(curl_handle, CURLOPT_HEADERDATA, (void *)p_recv_buf);

    /* get it! */
    CURLcode res = curl_easy_perform(curl_handle);

    if (res != CURLE_OK) {
        // fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        destroy_recv_buf(p_recv_buf);
        return NULL;
    }

    return p_recv_buf;
}

size_t write_cb_curl(void *p_recv, size_t size, size_t nmemb, void *p_userdata) {
    size_t realsize = size * nmemb;
    Recv_buf_p p = (Recv_buf_p) p_userdata;
 
    if (p->size + realsize + 1 > p->max_size) {/* hope this rarely happens */ 
        /* received data is not 0 terminated, add one byte for terminating 0 */
        // size_t new_size = p->max_size + max(BUF_INC, realsize + 1);   
        // unsigned char *q = realloc(p->buf, new_size);
        // if (q == NULL) {
        //     perror("realloc"); /* out of memory */
        //     return -1;
        // }
        // p->buf = q;
        // p->max_size = new_size;

        fprintf(stderr, "write_cb_curl: recieved %lu bytes, \
                but buffer size is %lu.\n", realsize, p->max_size);
        return -1;
    }

    memcpy(p->buf + p->size, p_recv, realsize); /*copy data from libcurl*/
    p->size += realsize;
    p->buf[p->size] = 0;

    return realsize;
}

size_t header_cb_curl(char *p_recv, size_t size, size_t nmemb, void *userdata)
{
    int realsize = size * nmemb;
    Recv_buf_p p = (Recv_buf_p) userdata;
    
    if (realsize > strlen(ECE252_HEADER) &&
	strncmp(p_recv, ECE252_HEADER, strlen(ECE252_HEADER)) == 0) {

        /* extract img sequence number */
	p->seq = atoi(p_recv + strlen(ECE252_HEADER));

    }
    return realsize;
}

void deep_copy_recv_buf(Recv_buf_p recv_buf, unsigned char* dest_buf, size_t dest_size) {
    if (recv_buf->max_size + sizeof(Recv_buf_t) > dest_size) {
        perror("deep_copy_recv_buf: dest_size is too small.");
        return;
    }

    memcpy(dest_buf, recv_buf, sizeof(Recv_buf_t));
    memcpy(dest_buf + sizeof(Recv_buf_t), recv_buf->buf, recv_buf->max_size);
    ((Recv_buf_p) dest_buf)->buf = dest_buf + sizeof(Recv_buf_t);
}