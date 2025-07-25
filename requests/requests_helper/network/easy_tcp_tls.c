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

#include "requests_helper/network/easy_tcp_tls.h"
#include "requests_helper/strings/strings.h"


struct _rh_socket_handler {
    int fd;
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
static char set_blocking_mode(int fd, char blocking)
{
   if (fd < 0) return 0;

#ifdef WIN32
   unsigned long mode = blocking ? 0 : 1;
   return (ioctlsocket(fd, FIONBIO, &mode) == 0) ? 1 : 0;
#else
   int flags = fcntl(fd, F_GETFL, 0);
   if (flags == -1) return 0;
   flags = blocking ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK);
   return (fcntl(fd, F_SETFL, flags) == 0) ? 1 : 0;
#endif
}

/*
Internal function that operate a connect operation with a custom timeout instead of the classic 5 minutes timeout.
*/
static char timeout_connect(int fd, const struct sockaddr* name, int namelen, int timeout_sec)
{
    fd_set fdset;
    struct timeval tv;
    int r;

    set_blocking_mode(fd, 0);
    connect(fd, name, namelen);
    set_blocking_mode(fd, 1);

    FD_ZERO(&fdset);
    FD_SET(fd, &fdset);
    tv.tv_sec = timeout_sec;
    tv.tv_usec = 0;
    r = select(fd + 1, NULL, &fdset, NULL, &tv) == 1;
    #ifndef WIN32
        if(r)
        {
            int so_error;
            socklen_t len = sizeof so_error;
            getsockopt(fd, SOL_SOCKET, SO_ERROR, &so_error, &len);
            r = so_error == 0;
        }
    #endif
    return r;
}

/*
Internal function to connect or build a socket according to the AI_FAMILY specified
*/
static int build_connected_socket(const char* server_hostname, char* str_server_port, int ai_family, char is_server)
{
    int fd = -1;
    bool success = false;
    struct addrinfo* result = NULL;
    struct addrinfo* next_result = NULL;

    struct addrinfo hints = {
        .ai_family = ai_family,
        .ai_socktype = SOCK_STREAM,
        .ai_flags = 0,
        .ai_protocol = IPPROTO_TCP
    };


    fd = socket(hints.ai_family, hints.ai_socktype, hints.ai_protocol);
    if(fd == -1)
    {
        goto FREE;
    }

    if(getaddrinfo(server_hostname, str_server_port, &hints, &result))
    {
        goto FREE;
    }

    next_result = result;

    while(next_result != NULL)
    {
        if(is_server)
        {
            if (bind(fd, next_result->ai_addr, next_result->ai_addrlen) == 0)
                break;                  /* Success */
        }
        else
        {
            if (timeout_connect(fd, next_result->ai_addr, next_result->ai_addrlen, 10))
                break;                  /* Success */
        }

        next_result = next_result->ai_next;
    }
    if(next_result == NULL)
    {
        goto FREE;
    }

    success = true;
FREE:
    freeaddrinfo(result);
    if(success)
    {
        return fd;
    }
    close(fd);
    return -1;
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

        data[0] = input >> 56;
        data[1] = input >> 48;
        data[2] = input >> 40;
        data[3] = input >> 32;
        data[4] = input >> 24;
        data[5] = input >> 16;
        data[6] = input >> 8;
        data[7] = input >> 0;

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
rh_SocketHandler* rh_socket_client_init(const char* server_hostname, uint16_t server_port)
{
    char str_server_port[8];  // 2**16 = 65536 (5 chars)
    rh_SocketHandler* client;

    client = (rh_SocketHandler*) malloc(sizeof(rh_SocketHandler));

    if(client == NULL)
    {
        return NULL;
    }

    rh_uint64_to_str(str_server_port, server_port);

    client->fd = build_connected_socket(server_hostname, str_server_port, AF_INET, 0);
    if(client->fd == -1)
    {
        // there is no ipv4, we try ipv6
        client->fd = build_connected_socket(server_hostname, str_server_port, AF_INET6, 0);
    }
    if(client->fd == -1)
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
rh_SocketHandler* rh_socket_ssl_client_init(const char* server_hostname, uint16_t server_port)
{
    rh_SocketHandler* client;
    SSL_library_init();
    OpenSSL_add_all_algorithms();  /* Load cryptos, et.al. */
    SSL_load_error_strings();   /* Bring in and register error messages */

    client = rh_socket_client_init(server_hostname, server_port);
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
    SSL_set_fd(client->ssl, client->fd);

    if (SSL_connect(client->ssl) == -1)
    {
        rh_socket_close(&client);
        return NULL;
    }

    return client;
}

/*
This function will create a server instance.

SERVER_HOSTNAME: this is always the hostname of the machine as it is visible outside. For local applications it can be "127.0.0.1" or "localhost"
SERVER_PORT: the port you want to listen. make sure it's not already taken or you will have a rh_UNABLE_TO_BIND error. If your server isn't local, make sure your port is opened on your firewall.
BACKLOG: The length of the connections queue before accept.

- when it succeeds, it returns a pointer to a structure handler.
- when it fails, it returns NULL and rh_print_last_error() can tell what happened
*/
rh_SocketHandler* rh_socket_server_init(const char* server_hostname, uint16_t server_port, int backlog)
{
    char str_server_port[8];  // 2**16 = 65536 (5 chars)
    rh_SocketHandler* server;

    server = (rh_SocketHandler*) malloc(sizeof(rh_SocketHandler));

    if(server == NULL)
    {
        return NULL;
    }

    rh_uint64_to_str(str_server_port, server_port);

    server->fd = build_connected_socket(server_hostname, str_server_port, AF_INET, 1);
    if(server->fd == -1)
    {
        // there is no ipv4, we try ipv6
        server->fd = build_connected_socket(server_hostname, str_server_port, AF_INET6, 1);
    }
    if(server->fd == -1)
    {
        free(server);
        return NULL;
    }

    server->ssl = NULL;
    server->ctx = NULL;

    struct timeval tv;
    tv.tv_sec = 60;
    tv.tv_usec = 0;
    setsockopt(server->fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);

    if (listen(server->fd, backlog) != 0)
    {
        free(server);
        return NULL;
    }


    return server;
}

/*
This function will create an ssl secured server instance
You must generate a public and a private key with this command
```sh
openssl req -x509 -newkey rsa:4096 -nodes -out ./cert.pem -keyout ./key.pem -days 365
```

SERVER_HOSTNAME: this is always the hostname of the machine as it is visible outside. For local applications it can be "127.0.0.1" or "localhost"
SERVER_PORT: the port you want to listen. make sure it's not already taken or you will have a rh_UNABLE_TO_BIND error. If your server isn't local, make sure your port is opened on your firewall.
BACKLOG: The length of the connections queue before accept.
PUBLIC_KEY_FP: the path for your cert.pem file
PRIVATE_KEY_FP: the path for your key.pem file

- when it succeeds, it returns a pointer to a structure handler.
- when it fails, it returns NULL and rh_print_last_error() can tell what happened
*/
rh_SocketHandler* rh_socket_ssl_server_init(const char* server_hostname, uint16_t server_port, int backlog, const char* public_key_fp, const char* private_key_fp)
{
    rh_SocketHandler* server;
    SSL_library_init();
    OpenSSL_add_all_algorithms();  /* Load cryptos, et.al. */
    SSL_load_error_strings();   /* Bring in and register error messages */

    server = rh_socket_server_init(server_hostname, server_port, backlog);
    if(server == NULL)
        return NULL;

    server->ctx = SSL_CTX_new(SSLv23_server_method());
    if(server->ctx == NULL)
    {
        rh_socket_close(&server);
        return NULL;
    }

    if(SSL_CTX_use_certificate_file(server->ctx, public_key_fp, SSL_FILETYPE_PEM) <= 0)
    {
        rh_socket_close(&server);
        return NULL;
    }
    if(SSL_CTX_use_PrivateKey_file(server->ctx, private_key_fp, SSL_FILETYPE_PEM) <= 0)
    {
        rh_socket_close(&server);
        return NULL;
    }

    return server;
}


/*
This function is used in a server application to wait and accept client connections.

SERVER: a pointer to a rh_SocketHandler, returned by - rh_socket_server_init or rh_socket_ssl_server_init

PCLIENT_DATA: a pointer to a rh_ClientData structure. It can be NULL if you don't care about client ip infos.
rh_ClientData structure is defined like that:
```c
typedef struct _rh_client_data {
    char ip[ADDRSTRLEN];
    uint16_t port;
} rh_ClientData;
```
If you have defined rh_ClientData infos;, you can access infos.ip and infos.port.


- when it succeeds, it returns a pointer to a structure handler.
- when it fails, it returns NULL and socket_print_last_error() can tell what happened
*/
rh_SocketHandler* rh_socket_accept(rh_SocketHandler* server, rh_ClientData* pclient_data)
{
    rh_SocketHandler* client;
    struct sockaddr_in peer_addr;
    #ifdef WIN32
        int addr_size = sizeof(struct sockaddr_in);
    #else
        unsigned int addr_size = sizeof(struct sockaddr_in);
    #endif

    client = (rh_SocketHandler*) malloc(sizeof(rh_SocketHandler));

    if(client == NULL)
    {
        return NULL;
    }

    client->fd = accept(server->fd, (struct sockaddr*) &peer_addr, &addr_size);
    client->ssl = NULL;
    client->ctx = NULL;

    if(client->fd <= 0)
    {
        rh_socket_close(&client);
        return NULL;
    }

    if(pclient_data != NULL)
    {
        inet_ntop(AF_INET, &(peer_addr.sin_addr), pclient_data->ip, INET_ADDRSTRLEN);
        pclient_data->port = ntohs(peer_addr.sin_port);
    }
    if(server->ctx != NULL)
    {
        client->ssl = SSL_new(server->ctx);
        SSL_set_fd(client->ssl, client->fd);

        if(SSL_accept(client->ssl) <= 0)
        {
            rh_socket_close(&client);
            return NULL;
        }
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
int rh_socket_send(rh_SocketHandler* s, const char* buffer, int n)
{
    if(s->ssl == NULL)
    {
        return send(s->fd, buffer, n, 0);
    }
    else
    {
        return SSL_write(s->ssl, buffer, n);
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
int rh_socket_recv(rh_SocketHandler* s, char* buffer, int n)
{
    if(s->ssl == NULL)
    {
        return recv(s->fd, buffer, n, 0);
    }
    else
    {
        return SSL_read(s->ssl, buffer, n);
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