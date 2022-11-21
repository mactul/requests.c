#include "requests.h"
#include <string.h>
#include <stdlib.h>
#define MAX_CHAR_ON_HOST 253  /* this is exact, don't change */
#define HEADERS_LENGTH   200  /* this is exact, don't change */

void int_to_string(int n, char s[]);
void reverse_string(char s[]);

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

char get(RequestsHandler* handler, char* url, char* additional_headers)
{
    return request(handler, "GET ", url, "", additional_headers);
}

char post(RequestsHandler* handler, char* url, char* data, char* additional_headers)
{
    return request(handler, "POST ", url, data, additional_headers);
}

char delete(RequestsHandler* handler, char* url, char* additional_headers)
{
    return request(handler, "DELETE ", url, "", additional_headers);
}

char patch(RequestsHandler* handler, char* url, char* data, char* additional_headers)
{
    return request(handler, "PATCH ", url, data, additional_headers);
}

char put(RequestsHandler* handler, char* url, char* data, char* additional_headers)
{
    return request(handler, "PUT ", url, data, additional_headers);
}


char request(RequestsHandler* handler, char* method, char* url, char* data, char* additional_headers)
{
    /* make a request with any method. Use the functions above instead. */
    int i;

    uint16_t port;
    char host[MAX_CHAR_ON_HOST + 1];
    char uri[MAX_URI_LENGTH];
    char content_length[30];

    char* headers = NULL;

    if(url[0] == 'h' && url[1] == 't' && url[2] == 't' && url[3] == 'p' && url[4] == 's' && url[5] == ':')
    {
        port = 443;
        url += 8;
    }
    else if(url[0] == 'h' && url[1] == 't' && url[2] == 't' && url[3] == 'p' && url[4] == ':')
    {
        port = 80;
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
    if(i == 0)
    {
        // There is no relative url
        uri[i] = '/';
        i++;
    }
    uri[i] = '\0';

    int_to_string(strlen(data), content_length);
    

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
    strcat(headers, "\r\n");
    if(stristr(additional_headers, "content-type:") == -1)  // we don't want to have the same header two times
    {
        strcat(headers, "Content-Type: application/x-www-form-urlencoded\r\n");
    }
    strcat(headers, "Content-Length: ");
    strcat(headers, content_length);
    strcat(headers, "\r\n");
    if(stristr(additional_headers, "connection:") == -1)  // we don't want to have the same header two times
    {
        strcat(headers, "Connection: close\r\n");
    }
    if(stristr(additional_headers, "accept") == -1)  // we don't want to have the same header two times
    {
        strcat(headers, "Accept: */*\r\n");
    }
    if(stristr(additional_headers, "user-agent") == -1)  // we don't want to have the same header two times
    {
        strcat(headers, "User-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/51.0.2704.103 Safari/537.36\r\n");
    }
    strcat(headers, additional_headers);
    strcat(headers, "\r\n");
    strcat(headers, data);

    struct hostent *server;

    server = gethostbyname(host);
    if(server == NULL)
    {
        return ERROR_HOST_CONNECTION;
    }

    handler->headers_readed = 0;

    if(port == 80)
    {
        if(socket_client_init(&(handler->handler), inet_ntoa(*((struct in_addr*)server->h_addr_list[0])), 80) < 0)
            return UNABLE_TO_BUILD_SOCKET;
    }
    else // Only 2 protocols are supported
    {
        if(socket_ssl_client_init(&(handler->handler), inet_ntoa(*((struct in_addr*)server->h_addr_list[0])), 443, host))
            return UNABLE_TO_BUILD_SOCKET;
    }

    int total = strlen(headers);
    int sent = 0;
    do {
        int bytes = socket_send(&(handler->handler), headers+sent, total-sent, 0);
        if (bytes < 0)
            return ERROR_WRITE;
        if (bytes == 0)
            break;
        sent+=bytes;
    } while (sent < total);

    return 0;
}

/*
    Skip the response header and fill the buffer with the server response
    Returns the number of bytes readed
    Use this in a loop
*/
int read_output_body(RequestsHandler* handler, char* buffer, int buffer_size)
{
    if(handler->headers_readed)
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
        handler->headers_readed = 1;
        return size - body_pos + read_output(handler, &(buffer[size-body_pos]), buffer_size-size+body_pos);
    }
}

/*
    Fill the buffer with the http response
    Returns the numbers of bytes readed
    Use this in a loop
*/
int read_output(RequestsHandler* s, char* buffer, int n)
{
    return socket_recv(&(s->handler), buffer, n, 0);
}

/*
    Close the connection and free the ssl ctx
*/
void close_connection(RequestsHandler* s)
{
    socket_close(&(s->handler));
}