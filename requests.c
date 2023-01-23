#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

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

#define MAX_CHAR_ON_HOST 253  /* this is exact, don't change */
#define HEADERS_LENGTH   220  /* this is exact, don't change */


struct requests_handler {
    SocketHandler* handler;
    char headers_readed;
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


RequestsHandler* req_request(char* method, char* url, char* data, char* additional_headers)
{
    /* make a request with any method. Use the functions above instead. */
    int i;

    uint16_t port;
    char host[MAX_CHAR_ON_HOST + 1];
    char uri[MAX_URI_LENGTH];
    char content_length[30];
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
    if(stristr(additional_headers, "connection:") == -1)  // we don't want to have the same header two times
    {
        strcat(headers, "Connection: close\r\n");
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

    handler->headers_readed = 0;

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

    return handler;
}

/*
    Skip the response header and fill the buffer with the server response
    Returns the number of bytes readed
    Use this in a loop
*/
int req_read_output_body(RequestsHandler* handler, char* buffer, int buffer_size)
{
    if(handler->headers_readed)
    {
        return req_read_output(handler, buffer, buffer_size);
    }
    else
    {
        int size;
        char c_return = 0;
        int body_pos = -1;
        while(body_pos == -1 && (size = req_read_output(handler, buffer, buffer_size)) > 0)
        {
            int i = 0;
            while(i < size && (buffer[i] != '\n' || !c_return))
            {
                if(buffer[i] == '\n')
                {
                    c_return = 1;
                }
                else if(buffer[i] >= '0')
                {
                    c_return = 0;
                }
                i++;
            }
            if(buffer[i] == '\n' && c_return)
            {
                body_pos = i+1;
            }
        }
        bytescpy(buffer, &(buffer[body_pos]), size-body_pos);
        handler->headers_readed = 1;
        return size - body_pos + req_read_output(handler, &(buffer[size-body_pos]), buffer_size-size+body_pos);
    }
}

/*
    Fill the buffer with the http response
    Returns the numbers of bytes readed
    Use this in a loop
*/
int req_read_output(RequestsHandler* handler, char* buffer, int n)
{
    return socket_recv(handler->handler, buffer, n, 0);
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
    free(*ppr);
    *ppr = NULL;
}