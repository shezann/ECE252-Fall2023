#include "web_crawler.h"

const uint8_t PNG_SIGNITURE[] = {137, 80, 78, 71, 13, 10, 26, 10};

char *_logfile = NULL;
pthread_mutex_t _logfile_mutex = PTHREAD_MUTEX_INITIALIZER;

void set_logfile(char *filename)
{
    _logfile = filename;
}

void _log(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    pthread_mutex_lock(&_logfile_mutex);
    if (_logfile != NULL)
    {
        FILE *fp = fopen(_logfile, "a");
        if (fp != NULL)
        {
            vfprintf(fp, fmt, args);
            fclose(fp);
        }
    }
    pthread_mutex_unlock(&_logfile_mutex);
    va_end(args);
}

int is_png(const uint8_t *buf, int size)
{
    if (size < PNG_SIG_SIZE)
    {
        return 0;
    }
    return memcmp(buf, PNG_SIGNITURE, PNG_SIG_SIZE) == 0;
}

int recv_buf_init(RECV_BUF *ptr, size_t max_size)
{
    void *p = NULL;

    if (ptr == NULL)
    {
        return 1;
    }

    p = malloc(max_size);
    if (p == NULL)
    {
        return 2;
    }

    ptr->buf = p;
    ptr->size = 0;
    ptr->max_size = max_size;
    ptr->seq = -1; /* valid seq should be positive */
    return 0;
}

int recv_buf_cleanup(RECV_BUF *ptr)
{
    if (ptr == NULL)
    {
        return 1;
    }

    free(ptr->buf);
    ptr->size = 0;
    ptr->max_size = 0;
    return 0;
}

void cleanup(CURL *curl, RECV_BUF *ptr)
{
    curl_easy_cleanup(curl);
    curl_global_cleanup();
    recv_buf_cleanup(ptr);
}

/**
 * @brief create a curl easy handle and set the options.
 * @param RECV_BUF *ptr points to user data needed by the curl write call back function
 * @param const char *url is the target url to fetch resoruce
 * @return a valid CURL * handle upon sucess; NULL otherwise
 * Note: the caller is responsbile for cleaning the returned curl handle
 */
CURL *easy_handle_init(RECV_BUF *ptr, const char *url)
{
    CURL *curl_handle = NULL;

    if (ptr == NULL || url == NULL)
    {
        return NULL;
    }

    /* init user defined call back function buffer */
    if (recv_buf_init(ptr, BUF_SIZE) != 0)
    {
        return NULL;
    }
    /* init a curl session */
    curl_handle = curl_easy_init();

    if (curl_handle == NULL)
    {
        fprintf(stderr, "curl_easy_init: returned NULL\n");
        return NULL;
    }

    /* specify URL to get */
    curl_easy_setopt(curl_handle, CURLOPT_URL, url);

    /* register write call back function to process received data */
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_cb_curl3);
    /* user defined data structure passed to the call back function */
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)ptr);

    /* register header call back function to process received header data */
    curl_easy_setopt(curl_handle, CURLOPT_HEADERFUNCTION, header_cb_curl);
    /* user defined data structure passed to the call back function */
    curl_easy_setopt(curl_handle, CURLOPT_HEADERDATA, (void *)ptr);

    /* some servers requires a user-agent field */
    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "ece252 lab4 crawler");

    /* follow HTTP 3XX redirects */
    curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L);
    /* continue to send authentication credentials when following locations */
    curl_easy_setopt(curl_handle, CURLOPT_UNRESTRICTED_AUTH, 1L);
    /* max numbre of redirects to follow sets to 5 */
    curl_easy_setopt(curl_handle, CURLOPT_MAXREDIRS, 5L);
    /* supports all built-in encodings */
    curl_easy_setopt(curl_handle, CURLOPT_ACCEPT_ENCODING, "");

    /* Max time in seconds that the connection phase to the server to take */
    // curl_easy_setopt(curl_handle, CURLOPT_CONNECTTIMEOUT, 5L);
    /* Max time in seconds that libcurl transfer operation is allowed to take */
    // curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, 10L);
    /* Time out for Expect: 100-continue response in milliseconds */
    // curl_easy_setopt(curl_handle, CURLOPT_EXPECT_100_TIMEOUT_MS, 0L);

    /* Enable the cookie engine without reading any initial cookies */
    curl_easy_setopt(curl_handle, CURLOPT_COOKIEFILE, "");
    /* allow whatever auth the proxy speaks */
    curl_easy_setopt(curl_handle, CURLOPT_PROXYAUTH, CURLAUTH_ANY);
    /* allow whatever auth the server speaks */
    curl_easy_setopt(curl_handle, CURLOPT_HTTPAUTH, CURLAUTH_ANY);

    return curl_handle;
}

/**
 * @brief  cURL header call back function to extract image sequence number from
 *         http header data. An example header for image part n (assume n = 2) is:
 *         X-Ece252-Fragment: 2
 * @param  char *p_recv: header data delivered by cURL
 * @param  size_t size size of each memb
 * @param  size_t nmemb number of memb
 * @param  void *userdata user defined data structurea
 * @return size of header data received.
 * @details this routine will be invoked multiple times by the libcurl until the full
 * header data are received.  we are only interested in the ECE252_HEADER line
 * received so that we can extract the image sequence number from it. This
 * explains the if block in the code.
 */
size_t header_cb_curl(char *p_recv, size_t size, size_t nmemb, void *userdata)
{
    int realsize = size * nmemb;
    RECV_BUF *p = userdata;

#ifdef DEBUG
    printf("%s", p_recv);
#endif /* DEBUG */
    if (realsize > strlen(ECE252_HEADER) &&
        strncmp(p_recv, ECE252_HEADER, strlen(ECE252_HEADER)) == 0)
    {

        /* extract img sequence number */
        p->seq = atoi(p_recv + strlen(ECE252_HEADER));
    }
    return realsize;
}

/**
 * @brief write callback function to save a copy of received data in RAM.
 *        The received libcurl data are pointed by p_recv,
 *        which is provided by libcurl and is not user allocated memory.
 *        The user allocated memory is at p_userdata. One needs to
 *        cast it to the proper struct to make good use of it.
 *        This function maybe invoked more than once by one invokation of
 *        curl_easy_perform().
 */
size_t write_cb_curl3(char *p_recv, size_t size, size_t nmemb, void *p_userdata)
{
    size_t realsize = size * nmemb;
    RECV_BUF *p = (RECV_BUF *)p_userdata;

    if (p->size + realsize + 1 > p->max_size)
    { /* hope this rarely happens */
        /* received data is not 0 terminated, add one byte for terminating 0 */
        size_t new_size = p->max_size + max(BUF_INC, realsize + 1);
        char *q = realloc(p->buf, new_size);
        if (q == NULL)
        {
#ifdef DEBUG
            perror("realloc"); /* out of memory */
#endif
            return -1;
        }
        p->buf = q;
        p->max_size = new_size;
    }

    memcpy(p->buf + p->size, p_recv, realsize); /*copy data from libcurl*/
    p->size += realsize;
    p->buf[p->size] = 0;

    return realsize;
}

htmlDocPtr mem_getdoc(char *buf, int size, const char *url)
{
    int opts = HTML_PARSE_NOBLANKS | HTML_PARSE_NOERROR |
               HTML_PARSE_NOWARNING | HTML_PARSE_NONET;
    htmlDocPtr doc = htmlReadMemory(buf, size, url, NULL, opts);

    if (doc == NULL)
    {
#ifdef DEBUG
        fprintf(stderr, "Document not parsed successfully.\n");
#endif /* DEBUG */
        return NULL;
    }
    return doc;
}

xmlXPathObjectPtr getnodeset(xmlDocPtr doc, xmlChar *xpath)
{

    xmlXPathContextPtr context;
    xmlXPathObjectPtr result;

    context = xmlXPathNewContext(doc);
    if (context == NULL)
    {
#ifdef DEBUG
        printf("Error in xmlXPathNewContext\n");
#endif
        return NULL;
    }
    result = xmlXPathEvalExpression(xpath, context);
    xmlXPathFreeContext(context);
    if (result == NULL)
    {
#ifdef DEBUG
        printf("Error in xmlXPathEvalExpression\n");
#endif
        return NULL;
    }
    if (xmlXPathNodeSetIsEmpty(result->nodesetval))
    {
        xmlXPathFreeObject(result);
#ifdef DEBUG
        printf("No result\n");
#endif
        return NULL;
    }
    return result;
}

int find_http(char *buf, int size, int follow_relative_links, const char *base_url, queue_t *ret)
{
    int i;
    htmlDocPtr doc;
    xmlChar *xpath = (xmlChar *)"//a/@href";
    xmlNodeSetPtr nodeset;
    xmlXPathObjectPtr result;
    xmlChar *href;

    if (buf == NULL)
    {
#ifdef DEBUG
        fprintf(stderr, "buf is NULL\n");
#endif /* DEBUG */
        return 1;
    }

    doc = mem_getdoc(buf, size, base_url);
    if (doc == NULL)
    {
#ifdef DEBUG
        fprintf(stderr, "doc is NULL\n");
#endif /* DEBUG */
        xmlCleanupParser();
        return 1;
    }

    result = getnodeset(doc, xpath);
    if (result == NULL)
    {
#ifdef DEBUG
        fprintf(stderr, "result is NULL\n");
#endif /* DEBUG */
        xmlFreeDoc(doc);
        xmlCleanupParser();
        return 1;
    }

    nodeset = result->nodesetval;
    for (i = 0; i < nodeset->nodeNr; i++)
    {
        href = xmlNodeListGetString(doc, nodeset->nodeTab[i]->xmlChildrenNode, 1);
        if (href == NULL)
        {
#ifdef DEBUG
            fprintf(stderr, "href is NULL\n");
#endif /* DEBUG */
            continue;
        }

        if (follow_relative_links == 1)
        {
            xmlChar *old = href;
            href = xmlBuildURI(href, (xmlChar *)base_url);
            xmlFree(old);
        }
        if (href != NULL && !strncmp((const char *)href, "http", 4))
        {
            queue_enqueue(ret, (const char *)href);
#ifdef DEBUG
            printf("href: %s\n", href);
#endif /* DEBUG */
        }
        xmlFree(href);
    }
    xmlXPathFreeObject(result);

    xmlFreeDoc(doc);
    xmlCleanupParser();
    return 0;
}

/**
 * @brief process the download data by curl and save the potiential urls to the queue
 *
 * @param CURL *curl_handle is the curl handler
 * @param RECV_BUF p_recv_buf contains the received data.
 * @param queue_t *ret is the queue to save the urls
 *
 * @return
 *  @retval effective url if the received data is a png
 *  @retval NULL if the received data is not a png
 * @note the caller is responsible for freeing the returned string
 */
char *process_data(CURL *curl_handle, RECV_BUF *p_recv_buf, queue_t *ret)
{
    CURLcode res;
    long response_code;

    res = curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &response_code);
    if (res == CURLE_OK)
    {
#ifdef DEBUG
        printf("Response code: %ld\n", response_code);
#endif
    }

    if (response_code >= 400)
    {
#ifdef DEBUG
        fprintf(stderr, "Error.\n");
#endif
        return NULL;
    }
    char *ct = NULL;
    res = curl_easy_getinfo(curl_handle, CURLINFO_CONTENT_TYPE, &ct);
    if (res == CURLE_OK && ct != NULL)
    {
#ifdef DEBUG
        printf("Content-Type: %s, len=%ld\n", ct, strlen(ct));
#endif
    }
    else
    {
#ifdef DEBUG
        fprintf(stderr, "Failed obtain Content-Type\n");
#endif
        return NULL;
    }

    char *eurl_tmp = NULL;
    curl_easy_getinfo(curl_handle, CURLINFO_EFFECTIVE_URL, &eurl_tmp);
    char *eurl = malloc(strlen(eurl_tmp) + 1);
    strcpy(eurl, eurl_tmp);

    // printf("effective url: %s\n", eurl);
    if (strstr(ct, CT_PNG))
    {
        if (is_png((uint8_t *)p_recv_buf->buf, p_recv_buf->size) == 1 &&
            eurl != NULL)
        {
            return eurl;
        }
#ifdef DEBUG
        fprintf(stderr, "Fake png file: %s\n", eurl);
#endif
    }
    else if (strstr(ct, CT_HTML))
    {
        find_http(p_recv_buf->buf, p_recv_buf->size, 1, eurl, ret);
    }

    return NULL;
}

search_return_t *web_crawler(void *arg)
{
    const char *url = (const char *)arg;

    _log(url);

    // Initialize curl
    CURL *curl_handle;
    CURLcode res;
    RECV_BUF recv_buf;

    curl_handle = easy_handle_init(&recv_buf, url);

    if (curl_handle == NULL)
    {
#ifdef DEBUG
        fprintf(stderr, "Curl initialization failed. Exiting...\n");
#endif
        abort();
    }

    // Get it
    res = curl_easy_perform(curl_handle);

    if (res != CURLE_OK)
    {
#ifdef DEBUG
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
#endif
        cleanup(curl_handle, &recv_buf);
        return NULL;
    }

    // Initialize queue
    queue_t queue;
    queue_init(&queue);

    search_return_t *ret = malloc(sizeof(search_return_t));

    ret->effective_url = process_data(curl_handle, &recv_buf, &queue);
    ret->n_children = queue_size(&queue);
    ret->children = (void **)queue_to_array(&queue);

    cleanup(curl_handle, &recv_buf);
    queue_cleanup(&queue);

    return ret;
}