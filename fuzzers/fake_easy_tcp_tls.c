#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "requests_helper/network/easy_tcp_tls.h"

static const uint8_t* _data = NULL;
static size_t _data_size = 0;

struct _rh_socket_handler {
    int unused;
};

void _rh_socket_start(void)
{
    return;
}

void _rh_socket_cleanup(void)
{
    return;
}

/*
This function will create the socket and returns a socket handler.

SERVER_HOSTNAME: the targeted server ip, formatted like "127.0.0.1", like "2001:0db8:85a3:0000:0000:8a2e:0370:7334" or like "example.com"
SERVER_PORT: the opened server port that listen the connection

- when it succeeds, it returns a pointer to a structure handler.
- when it fails, it returns NULL and rh_print_last_error() can tell what happened
*/
rh_SocketHandler* rh_socket_client_init(const char* server_hostname, uint16_t server_port, rh_milliseconds max_connect_time)
{
    return (rh_SocketHandler*) malloc(sizeof(rh_SocketHandler));
}

/*
This function works like rh_socket_client_init, but it will create an ssl secured socket connection.

SERVER_HOSTNAME: the targeted server ip, formatted like "127.0.0.1", like "2001:0db8:85a3:0000:0000:8a2e:0370:7334" or like "example.com"
SERVER_PORT: the opened server port that listen the connection
SNI_HOSTNAME: this is especially for web applications, when a single ip address can have multiple services. If it's set to NULL, it will take the value of server_hostname, otherwise, you must set the domain name of the server.

- when it succeeds, it returns a pointer to a structure handler.
- when it fails, it returns NULL and rh_print_last_error() can tell what happened
*/
rh_SocketHandler* rh_socket_ssl_client_init(const char* server_hostname, uint16_t server_port, rh_milliseconds max_connect_time)
{
    return rh_socket_client_init(server_hostname, server_port, max_connect_time);
}



/*
This function will send the data contained in the buffer array through the socket

S: a pointer to a rh_SocketHandler. If you are in a client application, it's the handler returned by rh_socket_client_init or rh_socket_ssl_client_init. If you are in a server application, it's the handler returned by rh_socket_accept
BUFFER: a buffer containing all the data you want to send
N: the size of the data, this can be different from the sizeof(buffer) if your buffer isn't full.
FLAGS: I recommend you to let this parameter to 0, but you can see the man page of send posix function to know more about that. This parameter do nothing if the connection is TLS

- when it succeeds, it returns the number of bytes sended
- when it fails, it returns -1 and errno contains more information.

*/
ssize_t rh_socket_send(rh_SocketHandler* s, const char* buffer, size_t n)
{
    return (ssize_t)n;
}

/*
This function will wait for data to arrive in the socket and fill a buffer with them.

S: a pointer to a rh_SocketHandler. If you are in a client application, it's the handler returned by rh_socket_client_init or rh_socket_ssl_client_init. If you are in a server application, it's the handler returned by rh_socket_accept.
BUFFER: an empty buffer that will be filled with data from the socket
N: the size of your buffer, you can simply provide sizeof(buffer).
FLAGS: I recommend you to let this parameter to 0, but you can see the man page of recv posix function to know more about that. This parameter do nothing if the connection is TLS


- when it succeeds, it returns the number of bytes read
- when it fails, it returns -1 and errno contains more information.
*/
ssize_t rh_socket_recv(rh_SocketHandler* s, char* buffer, size_t n)
{
    if(_data_size == 0)
    {
        return -1;
    }
    if(_data_size < n)
    {
        n = _data_size;
    }
    memcpy(buffer, _data, n);
    _data += n;
    _data_size -= n;
    return (ssize_t)n;
}

/*
This function take the address of the pointer on the handler, release all the stuff, close the socket and put the SocketHandler pointer to NULL.

PPS: the address of the pointer on the socket
*/
void rh_socket_close(rh_SocketHandler** pps)
{
    free(*pps);
    *pps = NULL;
}

void _rh_fuzzer_set_data(const uint8_t* data, size_t size)
{
    _data = data;
    _data_size = size;
}