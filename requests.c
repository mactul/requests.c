#include "requests.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>  /* read, write, close */
#include <sys/socket.h>  /* socket, connect */
#include <netinet/in.h>  /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>  /* struct hostent, gethostbyname */
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/bio.h>
#define MAX_CHAR_ON_HOST 253  /* this is exact, don't change */
#define HEADERS_LENGTH   115  /* this is exact, don't change */


BIO* _bio[MAX_CONNECTIONS];
int _sockfd[MAX_CONNECTIONS];
char _nonfree_handlers[MAX_CONNECTIONS] = {0};
char _http_handlers[MAX_CONNECTIONS] = {0};
char _headers_readed[MAX_CONNECTIONS] = {0};

void int_to_string(int n, char s[])
{
    int i, sign;

    if ((sign = n) < 0)  /* record sign */
        n = -n;          /* make n positive */
    i = 0;
    do
    {
        s[i++] = n % 10 + '0';   /* get next digit */
    } while ((n /= 10) > 0);     /* delete it */
    if (sign < 0)
        s[i++] = '-';
    s[i] = '\0';
    reverse_string(s);
}

void reverse_string(char s[])
{
    int i, j;
    char c;

    for (i = 0, j = strlen(s)-1; i<j; i++, j--)
    {
        c = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}

void bytescpy(char* dest, const char* src, int n)
{
    int i = 0;
    while(i < n)
    {
        dest[i] = src[i];
        i++;
    }
}

int stristr(const char* string, const char* exp)
{
    /* return the position of the first occurence's end
       this function is non case-sensitive */
    int string_counter = 0;
    int exp_counter = 0;
    while(string[string_counter] != '\0')
    {
        if(tolower(string[string_counter]) == tolower(exp[0]))
        {
            while(exp[exp_counter] != '\0' && string[string_counter] != '\0' && tolower(string[string_counter]) == tolower(exp[exp_counter]))
            {
                exp_counter++;
                string_counter++;
            }
            if(exp[exp_counter] == '\0')
            {
                return string_counter;
            }
            exp_counter = 0;
        }
        string_counter++;
    }
    return -1;
}


/* This part is all http methods implementation. */

Handler get(char* url, char* additional_headers)
{
    return request("GET ", url, "", additional_headers);
}

Handler post(char* url, char* data, char* additional_headers)
{
    return request("POST ", url, data, additional_headers);
}

Handler delete(char* url, char* additional_headers)
{
    return request("DELETE ", url, "", additional_headers);
}

Handler patch(char* url, char* data, char* additional_headers)
{
    return request("PATCH ", url, data, additional_headers);
}

Handler put(char* url, char* data, char* additional_headers)
{
    return request("PUT ", url, data, additional_headers);
}


Handler request(char* method, char* url, char* data, char* additional_headers)
{
    /* make a request with any method. Use the functions above instead. */
    int i;

    char port[30];
    char host[MAX_CHAR_ON_HOST + 1];
    char hostname[MAX_CHAR_ON_HOST + 30];
    char uri[MAX_URI_LENGTH];
    char content_length[30];

    char* headers = NULL;

    int size;
    char buf[1024];

    if(url[0] == 'h' && url[1] == 't' && url[2] == 't' && url[3] == 'p' && url[4] == 's' && url[5] == ':')
    {
        port[0] = '4';
        port[1] = '4';
        port[2] = '3';
        port[3] = '\0';
        url += 8;
    }
    else if(url[0] == 'h' && url[1] == 't' && url[2] == 't' && url[3] == 'p' && url[4] == ':')
    {
        port[0] = '8';
        port[1] = '0';
        port[2] = '\0';
        url += 7;
    }
    else
    {
        return ERROR_PROTOCOL;
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
    uri[i] = '\0';

    int_to_string(strlen(data), content_length);
    
    // build the connection string
    strcpy(hostname, host);
    strcat(hostname, ":");
    strcat(hostname, port);

    // reserves the exact memory space for the request
    headers = (char*) malloc(HEADERS_LENGTH + strlen(method) + strlen(uri) + strlen(host) + strlen(content_length) + strlen(data) + strlen(additional_headers) + 5);
    
    if(headers == NULL)
    {
        return ERROR_MALLOC;
    }

    // build the request with all the datas
    strcpy(headers, method);
    strcat(headers, uri);
    strcat(headers, " HTTP/1.1\r\nHost: ");
    strcat(headers, host);
    if(stristr(additional_headers, "content-type:") == -1)  // we don't wont to have the same header two times
    {
        strcat(headers, "\r\nContent-Type: application/x-www-form-urlencoded");
    }
    strcat(headers, "\r\nContent-Length: ");
    strcat(headers, content_length);
    if(stristr(additional_headers, "connection:") == -1)  // we don't wont to have the same header two times
    {
        strcat(headers, "\r\nConnection: close\r\n");
    }
    strcat(headers, additional_headers);
    strcat(headers, "\r\n");
    strcat(headers, data);

    if(port[0] == '4') // we don't need to test the whole string, the first character is enough
    {
        return https_request(hostname, headers);
    }
    else
    {
        return http_request(host, headers);
    }
}

Handler https_request(char* hostname, char* headers)
{
    SSL* ssl;
    SSL_CTX* ctx;
    Handler handler = 0;

    // found a free handler
    while(handler < MAX_CONNECTIONS && _nonfree_handlers[handler])
    {
        handler++;
    }
    if(_nonfree_handlers[handler])
    {
        return ERROR_MAX_CONNECTIONS;
    }

    _nonfree_handlers[handler] = 1;

    SSL_library_init();

    ctx = SSL_CTX_new(SSLv23_client_method());

    if (ctx == NULL)
    {
        return ERROR_SSL;
    }

    _bio[handler] = BIO_new_ssl_connect(ctx);

    SSL_CTX_free(ctx);

    BIO_set_conn_hostname(_bio[handler], hostname);

    if(BIO_do_connect(_bio[handler]) <= 0)
    {
        return ERROR_HOST_CONNECTION;
    }

    if(BIO_write(_bio[handler], headers, strlen(headers)) <= 0)
    {
        return ERROR_WRITE;
    }

    return handler;
}


Handler http_request(char* hostname, char* headers)
{
    Handler handler = 0;
    struct hostent *server;
    struct sockaddr_in serv_addr;
    int bytes, sent, received, total;

    //found a free handler
    while(handler < MAX_CONNECTIONS && _nonfree_handlers[handler])
    {
        handler++;
    }
    if(_nonfree_handlers[handler])
    {
        return ERROR_MAX_CONNECTIONS;
    }

    _nonfree_handlers[handler] = 1;


    _sockfd[handler] = socket(AF_INET, SOCK_STREAM, 0);
    if(_sockfd[handler] < 0)
    {
        return ERROR_HOST_CONNECTION;
    }

    server = gethostbyname(hostname);
    if(server == NULL)
    {
        return ERROR_HOST_CONNECTION;
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(80);
    memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);

    if (connect(_sockfd[handler], (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        return ERROR_HOST_CONNECTION;
    }

    total = strlen(headers);
    sent = 0;
    do {
        bytes = write(_sockfd[handler], headers+sent, total-sent);
        if (bytes < 0)
            return ERROR_WRITE;
        if (bytes == 0)
            break;
        sent+=bytes;
    } while (sent < total);

    _http_handlers[handler] = 1;

    return handler;
}

/*
    Skip the response header and fill the buffer with the server response
    Returns the number of bytes readed
    Use this in a loop
*/
int read_output_body(Handler handler, char* buffer, int buffer_size)
{
    if(_headers_readed[handler])
    {
        return read_output(handler, buffer, buffer_size);
    }
    else
    {
        int size;
        char c_return = 0;
        int body_pos = -1;
        while(body_pos == -1 && (size = read_output(handler, buffer, buffer_size)) > 0)
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
        _headers_readed[handler] = 1;
        return size - body_pos + read_output(handler, &(buffer[size-body_pos]), buffer_size-size+body_pos);
    }
}

/*
    Fill the buffer with the http response
    Returns the numbers of bytes readed
    Use this in a loop
*/
int read_output(Handler handler, char* buffer, int buffer_size)
{
    if(!_http_handlers[handler])
    {
        return  BIO_read(_bio[handler], buffer, buffer_size);
    }
    else
    {
        return read(_sockfd[handler], buffer, buffer_size);
    }
}


/*
    Close the connection and free the handler
*/
void close_connection(Handler handler)
{
    if(!_http_handlers[handler])
    {
        BIO_free_all(_bio[handler]);
    }
    else
    {
        close(_sockfd[handler]);
    }
    _nonfree_handlers[handler] = 0;
    _http_handlers[handler] = 0;
    _headers_readed[handler] = 0;
}