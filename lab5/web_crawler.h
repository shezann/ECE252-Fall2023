#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <curl/curl.h>
#include <libxml/HTMLparser.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/uri.h>

#include "queue.h"
#include "bfs.h"

#define SEED_URL "http://ece252-1.uwaterloo.ca/lab4/"
#define ECE252_HEADER "X-Ece252-Fragment: "
#define BUF_SIZE 1048576 /* 1024*1024 = 1M */
#define BUF_INC 524288   /* 1024*512  = 0.5M */

#define CT_PNG "image/png"
#define CT_HTML "text/html"
#define CT_PNG_LEN 9
#define CT_HTML_LEN 9

#define PNG_SIG_SIZE 8

#define max(a, b) \
  ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })

extern const uint8_t PNG_SIGNITURE[];

extern char *_logfile;
extern pthread_mutex_t _logfile_mutex;

typedef struct recv_buf2
{
  char *buf;       /* memory to hold a copy of received data */
  size_t size;     /* size of valid data in buf in bytes*/
  size_t max_size; /* max capacity of buf in bytes*/
  int seq;         /* >=0 sequence number extracted from http header */
                   /* <0 indicates an invalid seq number */
} RECV_BUF;

void set_logfile(char *filename);

void _log(const char *fmt, ...);

CURL *easy_handle_init(RECV_BUF *ptr, const char *url);
void cleanup(CURL *curl, RECV_BUF *ptr);

int recv_buf_init(RECV_BUF *ptr, size_t max_size);
int recv_buf_cleanup(RECV_BUF *ptr);

size_t header_cb_curl(char *p_recv, size_t size, size_t nmemb, void *userdata);
size_t write_cb_curl3(char *p_recv, size_t size, size_t nmemb, void *p_userdata);

htmlDocPtr mem_getdoc(char *buf, int size, const char *url);
xmlXPathObjectPtr getnodeset(xmlDocPtr doc, xmlChar *xpath);

int is_png(const uint8_t *buf, int size);

int find_http(char *buf,
              int size,
              int follow_relative_links,
              const char *base_url,
              queue_t *ret);

char *process_data(CURL *curl_handle, RECV_BUF *p_recv_buf, queue_t *ret);
