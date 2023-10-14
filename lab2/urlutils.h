#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <curl/curl.h>

#define NUM_URLS 3
#define ECE252_HEADER "X-Ece252-Fragment: "
#define BUF_SIZE 1048576  /* 1024*1024 = 1M */
#define BUF_INC  524288   /* 1024*512  = 0.5M */

#define max(a, b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })


extern const char* URL_LIST[NUM_URLS]; /* defined in urlutils.c */

/**
 * @brief the struct to store the PNG and the sequence number
*/
typedef struct recv_buf {
    char* buf;
    size_t size;
    size_t max_size;
    int seq;
}* Recv_buf_p;

/**
 * @brief create a Recv_buf_p type
 * @exception return NULL if malloc failed
 * @note The caller is responsible to call the destroy_recv_buf method to prevent memory leak.
 * @see destroy_recv_buf
*/
Recv_buf_p create_recv_buf(void);

/**
 * @brief destroy a Recv_buf_p type
 * @see create_recv_buf
*/
void destroy_recv_buf(Recv_buf_p p_recv_buf);

/**
 * @brief initialize the curl library
*/
void curl_init(void); 

/**
 * @brief clean up the curl library
*/
void curl_cleanup(void);

/**
 * @brief get a curl handle
 * @details The CURL handle returned can be reused for multiple requests.
 * 
 * @param url the url to fetch data
 * @return a curl handle
 * @exception return NULL if curl_easy_init failed
 * @note The caller is responsible to call the curl_easy_cleanup method to prevent memory leak.
*/
CURL *get_handle(char* url);

/**
 * @brief fetch url content and store in memory
 * 
 * @param[in] url the url to fetch
 * @return the received data
 * @exception return NULL if curl_easy_perform failed
 * @note The caller is responsible to call the destroy_recv_buf method to prevent memory leak.
*/
Recv_buf_p fetch_url(CURL* curl_handle);

/**
 * @brief Write callback function to save a the received data to a buffer.
 * 
 * @param[in] p_recv the received libcurl data, which is provided by libcurl.
 * @param[in] size the size of each memb.
 * @param[in] nmemb the number of memb.
 * @param[out] p_userdata the user data, which is going to be a Recv_buf_p type.
 * @return the size of the received data
*/
size_t write_cb_curl(void *p_recv, size_t size, size_t nmemb, void *p_userdata);

/**
 * @brief Header callback function to get the sequence number from the header and 
 *        store it in the user data.
 *        The sequence number is stored in the header as "X-Ece252-Fragment: <seq>".
 * 
 * @param[in] p_recv the received libcurl header data, which is provided by libcurl.
 * @param[in] size the size of each memb.
 * @param[in] nitems the number of memb.
 * @param[out] p_userdata the user data, which is going to be a Recv_PNG_p type.
 * @return the size of the received header data
*/
size_t header_cb_curl(char *p_recv, size_t size, size_t nitems, void *p_userdata);
