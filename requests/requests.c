#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include "requests_helper/strings/strings.h"
#include "requests_helper/network/easy_tcp_tls.h"
#include "requests_helper/parsing/parsing.h"
#include "requests_helper/path/path.h"
#include "requests.h"

#define HEADERS_LENGTH   300  /* this is exact, don't change */

#define PARSER_BUFFER_SIZE 1024

struct _requests_handler {
    rh_SocketHandler* handler;
    rh_ParserTree* headers_tree;
    int64_t total_bytes;
    uint64_t bytes_read;
    char host[rh_MAX_CHAR_ON_HOST + 1];
    uint16_t port;
    char* reading_residue;
    int residue_size;
    int residue_offset;
    unsigned short int status_code;
    bool read_finished;
    bool chunked;
    bool secured;
    char keep_alive_read;
};

static int64_t min64(int64_t a, int64_t b)
{
    return a < b ? a: b;
}

static bool req_parse_headers(RequestsHandler* handler);
static int req_read_output(RequestsHandler* handler, char* buffer, int n);
static bool send_headers(RequestsHandler* handler, char* headers);
static bool connect_socket(RequestsHandler* handler);


void req_init()
{
    _rh_socket_start();
}

void req_destroy()
{
    _rh_socket_cleanup();
}

uint64_t req_nb_bytes_read(RequestsHandler* handler)
{
    return handler->bytes_read;
}

/*
This is not meant to be used directly, unless you have exotic HTTP methods.
*/
RequestsHandler* req_request(RequestsHandler* handler, const char* method, const char* url, const char* data, const char* additional_headers)
{
    rh_UrlSplitted url_splitted;
    char content_length[30];
    char* headers = NULL;


    if(!rh_parse_url(url, &url_splitted))
    {
        return NULL;
    }

    rh_uint64_to_str(content_length, strlen(data));


    // reserves the exact memory space for the request
    headers = (char*) malloc((HEADERS_LENGTH + strlen(method) + strlen(url_splitted.uri) + strlen(url_splitted.host) + strlen(content_length) + strlen(data) + strlen(additional_headers))*sizeof(char));
    if(headers == NULL)
    {
        free(handler);
        return NULL;
    }

    // build the request with all the data
    rh_strcpy(headers, method);
    rh_strcat(headers, url_splitted.uri);
    rh_strcat(headers, " HTTP/1.1\r\nHost: ");
    rh_strcat(headers, url_splitted.host);
    rh_strcat(headers, "\r\nContent-Length: ");
    rh_strcat(headers, content_length);
    rh_strcat(headers, "\r\n");

    if(rh_str_search_case_unsensitive(additional_headers, "content-type:") == -1)  // we don't want to have the same header two times
    {
        rh_strcat(headers, "Content-Type: application/x-www-form-urlencoded\r\n");
    }
    if(rh_str_search_case_unsensitive(additional_headers, "accept") == -1)  // we don't want to have the same header two times
    {
        rh_strcat(headers, "Accept: */*\r\n");
    }
    if(rh_str_search_case_unsensitive(additional_headers, "user-agent") == -1)  // we don't want to have the same header two times
    {
        rh_strcat(headers, "User-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/51.0.2704.103 Safari/537.36\r\n");
    }
    if(rh_str_search_case_unsensitive(additional_headers, "connection") == -1)  // we don't want to have the same header two times
    {
        rh_strcat(headers, "Connection: keep-alive\r\n");
    }
    if(rh_str_search_case_unsensitive(additional_headers, "accept-encoding") == -1)  // we don't want to have the same header two times
    {
        rh_strcat(headers, "Accept-Encoding: identity\r\n");
    }

    rh_strcat(headers, additional_headers);
    rh_strcat(headers, "\r\n");
    rh_strcat(headers, data);

    if(handler != NULL && rh_strcasecmp(handler->host, url_splitted.host) == 0 && handler->port == url_splitted.port && handler->secured == url_splitted.secured)
    {
        char trash_buffer[2048];
        //clean the socket
        while(req_read_output_body(handler, trash_buffer, 2048) > 0)
        {
            ;
        }
        rh_ptree_free(&(handler->headers_tree));
        free(handler->reading_residue);
        handler->reading_residue = NULL;

        if(!send_headers(handler, headers) || rh_socket_recv(handler->handler, &(handler->keep_alive_read), 1) <= 0)
        {
            req_close_connection(&handler);  // connection expired
        }
        // else: connection successfully reused
    }
    else if(handler != NULL)
    {
        req_close_connection(&handler);
    }

    if(handler == NULL)
    {
        handler = (RequestsHandler*) malloc(sizeof(RequestsHandler));
        if(handler == NULL)
        {
            free(headers);
            return NULL;
        }

        handler->keep_alive_read = '\0';

        rh_strncpy(handler->host, url_splitted.host, rh_MAX_CHAR_ON_HOST+1);
        handler->port = url_splitted.port;
        handler->secured = url_splitted.secured;

        if(connect_socket(handler) == 0)
        {
            free(headers);
            free(handler);
            return NULL;
        }

        if(!send_headers(handler, headers))
        {
            free(headers);
            req_close_connection(&handler);
            return NULL;
        }
    }

    free(headers);
    headers = NULL;

    handler->headers_tree = NULL;
    handler->reading_residue = NULL;
    handler->residue_size = 0;
    handler->bytes_read = 0;
    handler->residue_offset = 0;
    handler->read_finished = 0;
    handler->status_code = 0;

    if(!req_parse_headers(handler))
    {
        req_close_connection(&handler);
        return NULL;
    }

    if(strcmp(method, "HEAD ") == 0)
    {
        handler->read_finished = 1;
    }
    else
    {
        const char* response_content_length = req_get_header_value(handler, "content-length");
        const char* transfer_encoding = req_get_header_value(handler, "transfer-encoding");
        if(response_content_length == NULL || (transfer_encoding != NULL && rh_str_search_case_unsensitive(transfer_encoding, "chunked") != -1))
        {
            handler->total_bytes = 0;
            handler->chunked = 1;
        }
        else
        {
            handler->total_bytes = rh_str_to_uint64(response_content_length);
            if(handler->total_bytes == UINT64_MAX)
            {
                req_close_connection(&handler);
                return NULL;
            }
            handler->chunked = 0;
        }
    }

    const char* location = rh_ptree_get_value(handler->headers_tree, "location");
    if(location != NULL)
    {
        char temp_url[2*rh_MAX_URI_LENGTH];
        char location_url[2*rh_MAX_URI_LENGTH];
        char port_str[8] = "";
        char protocol[] = "https://";
        int n;
        if(rh_startswith(location, "http://") || rh_startswith(location, "https://"))
        {
            return req_request(handler, method, location, data, additional_headers);
        }

        n = strlen(url_splitted.uri);
        while(n > 0 && url_splitted.uri[n] != '/')
        {
            n--;
        }
        url_splitted.uri[n] = '\0';

        if((url_splitted.secured && url_splitted.port != 443) || (!url_splitted.secured && url_splitted.port != 80))
        {
            port_str[0] = ':';
            rh_uint64_to_str(port_str+1, url_splitted.port);
        }
        if(!url_splitted.secured)
        {
            rh_strcpy(protocol, "http://");
        }

        rh_strcpy(rh_strcpy(rh_strcpy(location_url, protocol), url_splitted.host), port_str);

        rh_path_join(temp_url, 2*rh_MAX_URI_LENGTH, 3, location_url, url_splitted.uri, location);
        rh_simplify_path(location_url, temp_url);
        return req_request(handler, method, location_url, data, additional_headers);
    }

    return handler;
}

static unsigned short parse_status(char* key_value, char keep_alive_read)
{   
    int k = 0;
    char status_code[4];
    int l = 0;

    if(!rh_startswith(key_value, "HTTP/") && (keep_alive_read != 'H' || !rh_startswith(key_value, "TTP/")))
    {
        return 0;
    }

    // This is meant to happen with the first header "HTTP/1.1 ERROR_CODE MSG"

    while(key_value[k] != '\0' && key_value[k] != ' ')
        k++;
    if(key_value[k] != '\0')
        k++;
    while(l < 3 && rh_CHAR_IS_DIGIT(key_value[k]))
    {
        status_code[l] = key_value[k];
        l++;
        k++;
    }
    status_code[l] = '\0';
    return rh_str_to_uint64(status_code);
}


static bool req_parse_headers(RequestsHandler* handler)
{
    if(handler->headers_tree != NULL)
        return 1;
    
    char buffer[PARSER_BUFFER_SIZE];
    char key_value[PARSER_BUFFER_SIZE];

    bool c_return = false;
    int offset = -1;
    int size;
    bool in_value = false;
    int j = 0;

    handler->headers_tree = rh_ptree_init();

    while(offset == -1 && (size = req_read_output(handler, buffer, PARSER_BUFFER_SIZE)) > 0)
    {
        int i = 0;
        while(i < size && (buffer[i] != '\n' || !c_return))
        {
            key_value[j] = '\0';

            if(j == PARSER_BUFFER_SIZE-1)
            {
                char* trimmed = rh_strtrim_inplace(key_value);
                if(in_value)
                {
                    if(rh_ptree_update_value(handler->headers_tree, trimmed, strlen(trimmed)+1) == 0)
                        return false;
                }
                else
                {
                    if(rh_ptree_update_key(handler->headers_tree, trimmed, strlen(trimmed)+1) == 0)
                        return false;
                }
                j = 0;
                key_value[0] = '\0';
            }
            if(!in_value && buffer[i] == ':')
            {
                in_value = true;
                char* trimmed = rh_strtrim_inplace(key_value);
                if(rh_ptree_update_key(handler->headers_tree, trimmed, strlen(trimmed)+1) == 0)
                    return false;
                j = 0;
            }
            else if(buffer[i] == '\n')
            {
                c_return = true;
                if(in_value)
                {
                    in_value = false;
                    char* trimmed = rh_strtrim_inplace(key_value);
                    if(rh_ptree_update_value(handler->headers_tree, trimmed, strlen(trimmed)+1) == 0)
                        return false;
                    if(rh_ptree_push(handler->headers_tree, NULL) == 0)
                        return false;
                }
                else
                {
                    unsigned short status_code = parse_status(key_value, handler->keep_alive_read);
                    if(status_code != 0)
                    {
                        handler->status_code = status_code;
                    }
                    rh_ptree_abort(handler->headers_tree);
                }
                j = 0;
            }
            else
            {
                if(buffer[i] >= '0')
                    c_return = false;
                
                key_value[j] = buffer[i];
                j++;
            }

            i++;
        }
        if(i < size)
        {
            offset = i+1;
        }
    }
    
    handler->reading_residue = (char*) malloc((size - offset) * sizeof(char));
    if(handler->reading_residue == NULL)
        return false;
    handler->residue_size = size - offset;
    memcpy(handler->reading_residue, &(buffer[offset]), size-offset);

    return true;
}

/*
Returns the HTTP status code of the page
*/
unsigned short int req_get_status_code(RequestsHandler* handler)
{
    return handler->status_code;
}

/*
Returns the value of a specific header.
If HEADER_NAME is not in the server response headers, it returns NULL.
If you have to modify this value, copy it before.
*/
const char* req_get_header_value(RequestsHandler* handler, const char* header_name)
{
    return rh_ptree_get_value(handler->headers_tree, header_name);
}

/*
This display all headers in server response.
Used for debug purpose.
*/
void req_display_headers(RequestsHandler* handler)
{
    printf("STATUS CODE: %hu\n\n", handler->status_code);
    rh_ptree_display(handler->headers_tree);
}

static int64_t get_chunk_size(char c)
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
        uint64_t len = rh_hex_to_uint64(length);
        if(len == UINT64_MAX)
        {
            return -1;
        }
        return len;
    }
    return -1;
}

static int start_chunk_read(RequestsHandler* handler, char* buffer, int bytes_in_buffer, int buffer_size)
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
    if(bytes_in_buffer > offset)
    {
        bytes_in_buffer -= offset;
    }
    else
    {
        bytes_in_buffer = 0;
    }
    memcpy(buffer, &(buffer[offset]), bytes_in_buffer);
    handler->bytes_read = bytes_in_buffer;

    
    if(bytes_in_buffer > handler->total_bytes)
    {
        int64_t chunk_size = handler->total_bytes;
        bytes_in_buffer = chunk_size + start_chunk_read(handler, &(buffer[handler->total_bytes]), bytes_in_buffer-chunk_size, buffer_size);
    }

    return bytes_in_buffer;
}


/*
    Skip the response header and fill the buffer with the server response
    Returns the number of bytes read
    Use this in a loop.
    Note: This function reads binary data.
    If you are getting text, use `sizeof(buffer)-1` for buffer_size and add an '\0' at the end of the read buffer.
*/
int req_read_output_body(RequestsHandler* handler, char* buffer, int buffer_size)
{
    int size = 0;
    bool new_chunk = false;

    assert(handler != NULL);

    if(handler->read_finished)
    {
        return 0;
    }
    if(handler->total_bytes > (int64_t)handler->bytes_read)
    {
        int n = min64(buffer_size, handler->total_bytes - handler->bytes_read);
        size = req_read_output(handler, buffer, n);
        if(size <= 0)
        {
            handler->read_finished = true;
            return 0;
        }
        handler->bytes_read += size;
    }
    else if(handler->chunked)
    {
        new_chunk = true;
        size = req_read_output(handler, buffer, buffer_size);
        if(size <= 0)
        {
            handler->read_finished = true;
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
    Returns the numbers of bytes read
    Can block if there is no data left.
*/
static int req_read_output(RequestsHandler* handler, char* buffer, int n)
{
    int read;
    if(handler->residue_size > 0)
    {
        read = min64(n, handler->residue_size);
        memcpy(buffer, &(handler->reading_residue[handler->residue_offset]), read);
        handler->residue_size -= read;
        handler->residue_offset += read;
        if(handler->residue_size <= 0)
        {
            free(handler->reading_residue);
            handler->reading_residue = NULL;
        }
    }
    else
    {
        read = rh_socket_recv(handler->handler, buffer, n);
    }

    return read;
}

static bool connect_socket(RequestsHandler* handler)
{
    if(!handler->secured)
    {
        handler->handler = rh_socket_client_init(handler->host, handler->port);
        if(handler->handler == NULL)
        {
            return false;
        }
    }
    else
    {
        handler->handler = rh_socket_ssl_client_init(handler->host, handler->port);
        if(handler->handler == NULL)
        {
            return false;
        }
    }
    return true;
}

static bool send_headers(RequestsHandler* handler, char* headers)
{
    int total = strlen(headers);
    int sent = 0;
    do {
        int bytes = rh_socket_send(handler->handler, headers+sent, total-sent);
        if (bytes < 0)
        {
            return false;
        }
        if (bytes == 0)
            break;
        sent+=bytes;
    } while (sent < total);

    return true;
}

/*
    Close the connection and free the ssl ctx.
    PPR must be the address of the socket handler.
*/
void req_close_connection(RequestsHandler** ppr)
{
    if(*ppr == NULL)
    {
        return;
    }
    rh_socket_close(&((*ppr)->handler));
    rh_ptree_free(&((*ppr)->headers_tree));
    free((*ppr)->reading_residue);
    free(*ppr);
    *ppr = NULL;
}