#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#if defined(_WIN32) || defined(WIN32)
    #include <winsock2.h>
    #include <ws2tcpip.h>

    #define IS_WINDOWS 1

#else // Linux / MacOS
    #include <netdb.h>
    #include <arpa/inet.h>
    #include <netinet/in.h>
    #include <sys/socket.h>

#endif

#include "easy_tcp_tls.h"
#include "requests.h"
#include "utils.h"
#include "parser_tree.h"

#define MAX_CHAR_ON_HOST 253  /* this is exact, don't change */
#define HEADERS_LENGTH   256  /* this is exact, don't change */

#define CONTENT_LENGTH_STR "content-length"

#define PARSER_BUFFER_SIZE 1024

#ifndef min
#define min(a, b) (((a) < (b)) ? (a): (b))
#endif

struct requests_handler {
    SocketHandler* handler;
    int64_t total_bytes;
    uint64_t bytes_readed;
    char chunked;
    ParserTree* headers_tree;
    char* reading_residue;
    int residue_size;
    int residue_offset;
    char read_finished;
};

static int _error_code = 0;

int req_get_last_error(void)
{
    return _error_code;
}

void req_init(void)
{
    socket_start();
}

void req_cleanup(void)
{
    socket_cleanup();
}

char req_parse_headers(RequestsHandler* handler);
int req_read_output(RequestsHandler* handler, char* buffer, int n);


/* This part is all http methods implementation. */

RequestsHandler* req_get(char* url, char* additional_headers)
{
    return req_request("GET ", url, "", additional_headers);
}

RequestsHandler* req_post(char* url, char* data, char* additional_headers)
{
    return req_request("POST ", url, data, additional_headers);
}

RequestsHandler* req_delete(RequestsHandler* handler, char* url, char* additional_headers)
{
    return req_request("DELETE ", url, "", additional_headers);
}

RequestsHandler* req_patch(RequestsHandler* handler, char* url, char* data, char* additional_headers)
{
    return req_request("PATCH ", url, data, additional_headers);
}

RequestsHandler* req_put(RequestsHandler* handler, char* url, char* data, char* additional_headers)
{
    return req_request("PUT ", url, data, additional_headers);
}

RequestsHandler* req_head(char* url, char* additional_headers)
{
    return req_request("HEAD ", url, "", additional_headers);
}

RequestsHandler* req_request(char* method, char* url, char* data, char* additional_headers)
{
    /* make a request with any method. Use the functions above instead. */
    int i;

    uint16_t port;
    char host[MAX_CHAR_ON_HOST + 1];
    char uri[MAX_URI_LENGTH];
    char content_length[30];
    const char* reference_url = url;
    RequestsHandler* handler;

    if(starts_with(url, "https:"))
    {
        port = 443;
        url += 8;
    }
    else if(starts_with(url, "http:"))
    {
        port = 80;
        url += 7;
    }
    else
    {
        _error_code = ERROR_PROTOCOL;
        return NULL;
    }

    // get the host from url
    i = 0;
    while(*url != '\0' && *url != '/' && *url != '?' && *url != '#')
    {
        host[i] = *url;
        i++;
        url++;
    }
    host[i] = '\0';


    // get the relative url from url
    i = 0;
    while(*url != '\0' && *url != '#')
    {
        uri[i] = *url;
        i++;
        url++;
    }
    if(i == 0)
    {
        // There is no relative url
        uri[i] = '/';
        i++;
    }
    uri[i] = '\0';

    int_to_string(strlen(data), content_length);
    

    // reserves the exact memory space for the request
    char headers[HEADERS_LENGTH + strlen(method) + strlen(uri) + strlen(host) + strlen(content_length) + strlen(data) + strlen(additional_headers) + 5];

    // build the request with all the datas
    strcpy(headers, method);
    strcat(headers, uri);
    strcat(headers, " HTTP/1.1\r\nHost: ");
    strcat(headers, host);
    strcat(headers, "\r\nContent-Length: ");
    strcat(headers, content_length);
    strcat(headers, "\r\n");

    if(stristr(additional_headers, "content-type:") == -1)  // we don't want to have the same header two times
    {
        strcat(headers, "Content-Type: application/x-www-form-urlencoded\r\n");
    }
    if(stristr(additional_headers, "accept") == -1)  // we don't want to have the same header two times
    {
        strcat(headers, "Accept: */*\r\n");
    }
    if(stristr(additional_headers, "user-agent") == -1)  // we don't want to have the same header two times
    {
        strcat(headers, "User-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/51.0.2704.103 Safari/537.36\r\n");
    }
    if(stristr(additional_headers, "connection") == -1)  // we don't want to have the same header two times
    {
        strcat(headers, "Connection: close\r\n");
    }
    if(stristr(additional_headers, "accept-encoding") == -1)  // we don't want to have the same header two times
    {
        strcat(headers, "Accept-Encoding: identity\r\n");
    }
    
    strcat(headers, additional_headers);
    strcat(headers, "\r\n");
    strcat(headers, data);

    struct hostent *server;

    server = gethostbyname(host);
    if(server == NULL)
    {
        _error_code = ERROR_HOST_CONNECTION;
        return NULL;
    }

    handler = (RequestsHandler*) malloc(sizeof(RequestsHandler));

    handler->headers_tree = NULL;
    handler->reading_residue = NULL;
    handler->residue_size = 0;
    handler->bytes_readed = 0;
    handler->residue_offset = 0;
    handler->read_finished = 0;

    if(port == 80)
    {
        handler->handler = socket_client_init(inet_ntoa(*((struct in_addr*)server->h_addr_list[0])), 80);
        if(handler->handler == NULL)
        {
            _error_code = UNABLE_TO_BUILD_SOCKET;
            free(handler);
            return NULL;
        }
    }
    else // Only 2 protocols are supported
    {
        handler->handler = socket_ssl_client_init(inet_ntoa(*((struct in_addr*)server->h_addr_list[0])), 443, host);
        if(handler->handler == NULL)
        {
            _error_code = UNABLE_TO_BUILD_SOCKET;
            free(handler);
            return NULL;
        }
    }

    int total = strlen(headers);
    int sent = 0;
    do {
        int bytes = socket_send(handler->handler, headers+sent, total-sent, 0);
        if (bytes < 0)
        {
            _error_code = ERROR_WRITE;
            req_close_connection(&handler);
            return NULL;
        }
        if (bytes == 0)
            break;
        sent+=bytes;
    } while (sent < total);

    req_parse_headers(handler);

    if(strcmp(method, "HEAD ") == 0)
    {
        handler->read_finished = 1;
    }
    else
    {
        const char* response_content_length = req_get_header_value(handler, "content-length");
        const char* transfer_encoding = req_get_header_value(handler, "transfer-encoding");
        if(response_content_length == NULL || (transfer_encoding != NULL && stristr(transfer_encoding, "chunked") != -1))
        {
            handler->total_bytes = 0;
            handler->chunked = 1;
        }
        else
        {
            handler->total_bytes = atoll(response_content_length);
            handler->chunked = 0;
        }
    }

    const char* location = ptree_get_value(handler->headers_tree, "location");
    if(location != NULL)
    {
        char location_url[MAX_URI_LENGTH];
        strcpy(location_url, location);
        retrieve_absolute_url(location_url, reference_url);
        req_close_connection(&handler);
        return req_request(method, location_url, data, additional_headers);
    }

    return handler;
}


char req_parse_headers(RequestsHandler* handler)
{
    if(handler->headers_tree != NULL)
        return 1;
    
    char buffer[PARSER_BUFFER_SIZE];
    char key_value[PARSER_BUFFER_SIZE];

    char c_return = 0;
    int offset = -1;
    int size;
    char in_value = 0;
    int j = 0;

    handler->headers_tree = ptree_init();

    while(offset == -1 && (size = req_read_output(handler, buffer, PARSER_BUFFER_SIZE)) > 0)
    {
        int i = 0;
        while(i < size && (buffer[i] != '\n' || !c_return))
        {
            key_value[j] = '\0';

            if(j == PARSER_BUFFER_SIZE-1)
            {
                if(in_value)
                {
                    if(ptree_update_value(handler->headers_tree, strtrim(key_value)) == 0)
                        return 0;
                }
                else
                {
                    if(ptree_update_key(handler->headers_tree, strtrim(key_value)) == 0)
                        return 0;
                }
                j = 0;
                key_value[0] = '\0';
            }
            if(!in_value && buffer[i] == ':')
            {
                in_value = 1;
                if(ptree_update_key(handler->headers_tree, strtrim(key_value)) == 0)
                    return 0;
                j = 0;
            }
            else if(buffer[i] == '\n')
            {
                c_return = 1;
                if(in_value)
                {
                    in_value = 0;
                    if(ptree_update_value(handler->headers_tree, strtrim(key_value)) == 0)
                        return 0;
                    if(ptree_push(handler->headers_tree) == 0)
                        return 0;
                }
                else
                {
                    ptree_abort(handler->headers_tree);
                }
                j = 0;
            }
            else
            {
                if(buffer[i] >= '0')
                    c_return = 0;
                
                key_value[j] = buffer[i];
                j++;
            }

            i++;
        }
        if(buffer[i] == '\n' && c_return)
        {
            offset = i+1;
        }
    }
    
    handler->reading_residue = (char*) malloc((size - offset) * sizeof(char));
    if(handler->reading_residue == NULL)
        return 0;
    handler->residue_size = size - offset;
    bytescpy(handler->reading_residue, &(buffer[offset]), size-offset);

    return 1;
}

const char* req_get_header_value(RequestsHandler* handler, char* header_name)
{
    return ptree_get_value(handler->headers_tree, header_name);
}

void req_display_headers(RequestsHandler* handler)
{
    ptree_display(handler->headers_tree);
}

int64_t get_chunk_size(char c)
{
    static int i = 0;
    static char length[255];

    if(('0' <= c && c <= '9') || ('a' <= c && c <= 'f') || ('A' <= c && c <= 'F'))
    {
        length[i] = c;
        i++;
    }
    else if(c == '\r')
    {
        if(i == 0)
        {
            return -1;
        }
        length[i] = '\0';
        i++;
    }
    else if(c == '\n' && i != 0)
    {
        i = 0;
        return strtol(length, NULL, 16);
    }
    return -1;
}

int start_chunk_read(RequestsHandler* handler, char* buffer, int bytes_in_buffer, int buffer_size)
{
    int offset = 0;
    handler->total_bytes = -1;
    while(handler->total_bytes == -1)
    {
        while(handler->total_bytes == -1 && offset < bytes_in_buffer)
        {
            handler->total_bytes = get_chunk_size(buffer[offset]);
            (offset)++;
        }
        if(handler->total_bytes == -1)
        {
            bytes_in_buffer = req_read_output(handler, buffer, buffer_size);
            offset = 0;
        }
    }
    
    if(handler->total_bytes == 0)
    {
        // That was the last one
        handler->read_finished = 1;
        return 0;
    }
    if(bytes_in_buffer <= 0)
    {
        return bytes_in_buffer;
    }
    bytes_in_buffer = relu(bytes_in_buffer - offset);
    bytescpy(buffer, &(buffer[offset]), bytes_in_buffer);
    handler->bytes_readed = bytes_in_buffer;

    
    if(bytes_in_buffer > handler->total_bytes)
    {
        int64_t chunk_size = handler->total_bytes;
        bytes_in_buffer = chunk_size + start_chunk_read(handler, &(buffer[handler->total_bytes]), bytes_in_buffer-chunk_size, buffer_size);
    }

    return bytes_in_buffer;
}


/*
    Skip the response header and fill the buffer with the server response
    Returns the number of bytes readed
    Use this in a loop
*/
int req_read_output_body(RequestsHandler* handler, char* buffer, int buffer_size)
{
    int size = 0;
    char new_chunk = 0;

    assert(handler != 0);

    if(handler->read_finished)
    {
        return 0;
    }
    if(handler->total_bytes > handler->bytes_readed)
    {
        int n = min(buffer_size, handler->total_bytes - handler->bytes_readed);
        size = req_read_output(handler, buffer, n);
        if(size <= 0)
        {
            handler->read_finished = 1;
            return 0;
        }
        handler->bytes_readed += size;
    }
    else if(handler->chunked)
    {
        new_chunk = 1;
        size = req_read_output(handler, buffer, buffer_size);
        if(size <= 0)
        {
            handler->read_finished = 1;
            return 0;
        }
    }

    if(size <= 0)
    {
        return size;
    }

    if(handler->chunked)
    {
        if(new_chunk)
        {
            size = start_chunk_read(handler, buffer, size, buffer_size);
        }
    }
        if(size < buffer_size)
        {
            return size + req_read_output_body(handler, &(buffer[size]), buffer_size-size);
        }
    return size;
}

/*
    Fill the buffer with the http response
    Returns the numbers of bytes readed
    Use this in a loop
*/
int req_read_output(RequestsHandler* handler, char* buffer, int n)
{
    int readed;
    if(handler->residue_size > 0)
    {
        readed = min(n, handler->residue_size);
        bytescpy(buffer, &(handler->reading_residue[handler->residue_offset]), readed);
        handler->residue_size -= readed;
        handler->residue_offset += readed;
        if(handler->residue_size <= 0)
        {
            free(handler->reading_residue);
            handler->reading_residue = NULL;
        }
    }
    else
    {
        readed = socket_recv(handler->handler, buffer, n, 0);
    }

    return readed;
}

/*
    Close the connection and free the ssl ctx
*/
void req_close_connection(RequestsHandler** ppr)
{
    if(*ppr == NULL)
    {
        return;
    }
    socket_close(&((*ppr)->handler));
    ptree_free(&((*ppr)->headers_tree));
    if((*ppr)->reading_residue != NULL)
        free((*ppr)->reading_residue);
    free(*ppr);
    *ppr = NULL;
}