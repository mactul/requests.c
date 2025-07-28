#include <stdbool.h>

#ifdef WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>

#else // Linux / MacOS
    #include <netdb.h>
    #include <arpa/inet.h>
    #include <netinet/in.h>
    #include <sys/socket.h>
    #include <sys/time.h>

#endif

#include <openssl/ssl.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include "requests_helper/network/easy_tcp_tls.h"
#include "requests_helper/strings/strings.h"


#ifdef WIN32
    typedef SOCKET sock_fd;
    #define RH_INVALID_SOCKET INVALID_SOCKET
#else
    typedef int sock_fd;
    #define RH_INVALID_SOCKET -1
#endif

struct _rh_socket_handler {
    sock_fd fd;
    SSL* ssl;
    SSL_CTX* ctx;
};


/*
Internal function used to init the sockets for windows
*/
void _rh_socket_start(void)
{
    #ifdef WIN32
        static char is_started = 0;
        if(!is_started)
        {
            WSADATA WrhData;
            WSAStartup(MAKEWORD(2,0), &WrhData);
            is_started = 1;
        }
    #endif
    return;
}

/*
Internal function that destroy all sockets on windows
*/
void _rh_socket_cleanup(void)
{
    #ifdef WIN32
        WSACleanup();
    #endif
    return;
}

/*
Internal function to set a file descriptor in blocking/non-blocking mode
*/
static bool set_blocking_mode(sock_fd fd, bool blocking)
{
    if (fd == RH_INVALID_SOCKET) return false;

#ifdef WIN32
    unsigned long mode = (unsigned long)!blocking;
    return ioctlsocket(fd, FIONBIO, &mode) == 0;
#else
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
        return false;
    flags = blocking ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK);
    return fcntl(fd, F_SETFL, flags) == 0;
#endif
}


/*
Internal function to connect or build a socket according to the AI_FAMILY specified
*/
static sock_fd build_connected_socket(const char* server_hostname, char* str_server_port, rh_milliseconds max_connect_time)
{
    int r;
    size_t i = 0;
    size_t number_of_addr = 0;
    fd_set fdset;
    struct timeval tv;
    sock_fd* fds = NULL;
    sock_fd fd = RH_INVALID_SOCKET;
    struct addrinfo* result = NULL;
    struct addrinfo* next_result = NULL;
    rh_nanoseconds timer;

    struct addrinfo hints = {
        .ai_family = AF_UNSPEC,
        .ai_socktype = SOCK_STREAM,
        .ai_flags = 0,
        .ai_protocol = IPPROTO_TCP
    };

    if(getaddrinfo(server_hostname, str_server_port, &hints, &result))
    {
        goto FREE;
    }

    next_result = result;
    while(next_result != NULL)
    {
        number_of_addr++;
        next_result = next_result->ai_next;
    }

    if(number_of_addr == 0)
    {
        goto FREE;
    }

    fds = malloc(number_of_addr * sizeof(sock_fd));
    if(fds == NULL)
    {
        goto FREE;
    }

    for(i = 0; i < number_of_addr; i++)
    {
        fds[i] = RH_INVALID_SOCKET;
    }

    i = 0;
    next_result = result;
    while(next_result != NULL)
    {
        fds[i] = socket(next_result->ai_family, next_result->ai_socktype, next_result->ai_protocol);
        if(fds[i] == RH_INVALID_SOCKET)
        {
            next_result = next_result->ai_next;
            continue;
        }

        set_blocking_mode(fds[i], false);
        if(connect(fds[i], next_result->ai_addr, (socklen_t)next_result->ai_addrlen) == -1 && errno != EINPROGRESS)
        {
            #ifdef WIN32
                closesocket(fds[i]);
            #else
                close(fds[i]);
            #endif
            fds[i] = RH_INVALID_SOCKET;
            next_result = next_result->ai_next;
            continue;
        }
        set_blocking_mode(fds[i], true);

        next_result = next_result->ai_next;
        i++;
    }

    timer = rh_timer_now();

    while(rh_timer_elapsed_ms(timer) < max_connect_time)
    {
        int max_fd = 0;
        rh_microseconds delay_us = (rh_microseconds)rh_duration((rh_microseconds)(max_connect_time * 1000), rh_timer_elapsed_us(timer));

        tv.tv_sec = (long)(delay_us / (rh_microseconds)(1000 * 1000));
        tv.tv_usec = (long)(delay_us % (rh_microseconds)(1000 * 1000));

        FD_ZERO(&fdset);
        for(i = 0; i < number_of_addr; i++)
        {
            if(fds[i] != RH_INVALID_SOCKET)
            {
                FD_SET(fds[i], &fdset);
                if((int)fds[i] > max_fd)
                {
                    max_fd = (int)fds[i];
                }
            }
        }
        r = select(max_fd + 1, NULL, &fdset, NULL, &tv);
        if(r < 1)
        {
            goto FREE;
        }

        for(i = 0; i < number_of_addr; i++)
        {
            if(FD_ISSET(fds[i], &fdset))
            {
                int so_error;
                socklen_t len = sizeof(so_error);
                if(getsockopt(fds[i], SOL_SOCKET, SO_ERROR, (void*)&so_error, &len) == 0 && so_error == 0)
                {
                    fd = fds[i];
                    goto SUCCESS;
                }
                else
                {
                    #ifdef WIN32
                        closesocket(fds[i]);
                    #else
                        close(fds[i]);
                    #endif
                    fds[i] = RH_INVALID_SOCKET;
                }
            }
        }
    }

SUCCESS:
FREE:
    if(fds != NULL)
    {
        for(i = 0; i < number_of_addr; i++)
        {
            if(fds[i] != fd && fds[i] != RH_INVALID_SOCKET)
            {
                #ifdef WIN32
                    closesocket(fds[i]);
                #else
                    close(fds[i]);
                #endif
            }
        }
    }
    free(fds);
    freeaddrinfo(result);
    return fd;
}

/*
This function is used to convert a 64 bits integer from your binary representation to a Big Endian representation and vice-versa
If your system is already a Big Endian, it will do nothing.
*/
uint64_t rh_socket_ntoh64(uint64_t input)
{
    int n = 1;
    if((&n)[0] == 1)  // LITTLE_ENDIAN
    {
        uint64_t rval;
        uint8_t *data = (uint8_t *)&rval;

        for(size_t i = 0; i < sizeof(uint64_t); i++)
        {
            data[i] = (uint8_t)(input >> (8 * i));
        }

        return rval;
    }
    return input;
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
    char str_server_port[8];  // 2**16 = 65536 (5 chars)
    rh_SocketHandler* client;

    client = (rh_SocketHandler*) malloc(sizeof(rh_SocketHandler));

    if(client == NULL)
    {
        return NULL;
    }

    rh_uint64_to_str(str_server_port, server_port);

    client->fd = build_connected_socket(server_hostname, str_server_port, max_connect_time);
    if(client->fd == RH_INVALID_SOCKET)
    {
        free(client);
        return NULL;
    }

    client->ssl = NULL;
    client->ctx = NULL;


    return client;
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
    rh_SocketHandler* client;
    SSL_library_init();
    OpenSSL_add_all_algorithms();  /* Load cryptos, et.al. */
    SSL_load_error_strings();   /* Bring in and register error messages */

    client = rh_socket_client_init(server_hostname, server_port, max_connect_time);
    if(client == NULL)
        return NULL;

    client->ctx = SSL_CTX_new(SSLv23_client_method());
    if(client->ctx == NULL)
    {
        rh_socket_close(&client);
        return NULL;
    }
    client->ssl = SSL_new(client->ctx);
    if(client->ssl == NULL)
    {
        rh_socket_close(&client);
        return NULL;
    }

    SSL_set_tlsext_host_name(client->ssl, server_hostname);
    SSL_set_fd(client->ssl, (int)client->fd);

    if (SSL_connect(client->ssl) == -1)
    {
        rh_socket_close(&client);
        return NULL;
    }

    return client;
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
    if(s->ssl == NULL)
    {
        return send(s->fd, buffer, n, 0);
    }
    else
    {
        return SSL_write(s->ssl, buffer, (int)n);
    }
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
    if(s->ssl == NULL)
    {
        return recv(s->fd, buffer, n, 0);
    }
    else
    {
        return SSL_read(s->ssl, buffer, (int)n);
    }
}

/*
This function take the address of the pointer on the handler, release all the stuff, close the socket and put the SocketHandler pointer to NULL.

PPS: the address of the pointer on the socket
*/
void rh_socket_close(rh_SocketHandler** pps)
{
    if(*pps == NULL)
    {
        return;
    }
    if((*pps)->ssl != NULL)
    {
        SSL_shutdown((*pps)->ssl);  // a first time to send the close_notify alert
        SSL_shutdown((*pps)->ssl);  // a second time to wait for the peer response
        SSL_free((*pps)->ssl);
    }
    if((*pps)->ctx != NULL)
    {
        SSL_CTX_free((*pps)->ctx);
    }
    #ifdef WIN32
        closesocket((*pps)->fd);
    #else
        close((*pps)->fd);
    #endif

    free(*pps);
    *pps = NULL;
}