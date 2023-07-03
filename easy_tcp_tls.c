#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <openssl/ssl.h>
#include "easy_tcp_tls.h"


#if defined(_WIN32) || defined(WIN32)
    #include <winsock2.h>
    #include <ws2tcpip.h>

    #define IS_WINDOWS 1

#else // Linux / MacOS
    #include <netdb.h>
    #include <arpa/inet.h>
    #include <netinet/in.h>
    #include <sys/socket.h>
    #include <sys/time.h>

#endif

struct socket_handler {
    int fd;
    SSL* ssl;
    SSL_CTX* ctx;
};

static int _error_code = 0;

static char* uint16_to_str(char* result, uint16_t n)
{
    int i = 0;
    int j = 0;
    do
    {
        result[i] = n % 10 + '0';
        i++;
    } while ((n /= 10) > 0);
    result[i] = '\0';
    i--;

    while(j < i)
    {
        char c = result[j];
        result[j] = result[i];
        result[i] = c;
        j++;
        i--;
    }
    return result;
}


void socket_start(void)
{
    #ifdef IS_WINDOWS
        static char is_started = 0;
        if(!is_started)
        {
            WSADATA WSAData;
            WSAStartup(MAKEWORD(2,0), &WSAData);
            is_started = 1;
        }
    #endif
    return;
}

void socket_cleanup(void)
{
    #ifdef IS_WINDOWS
        WSACleanup();
    #endif
    return;
}

uint64_t socket_ntoh64(uint64_t input)
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

int socket_get_last_error(void)
{
    return _error_code;
}

void socket_print_last_error(void)
{
    switch(_error_code)
    {
        case SOCKET_ATTRIBUTION_ERROR:
            printf("Socket attribution error\nUnix socket function returns -1, check errno\n");
            break;
        case CONNECTION_REFUSED:
            printf("Connection refused\nCheck your connection internet, check if the server is started\n");
            break;
        case UNABLE_TO_BIND:
            printf("Unable to bind\nMaybe the port is already taken. Use port=0 for an automatic attribution\n");
            break;
        case UNABLE_TO_LISTEN:
            printf("Unable to listen\n");
            break;
        case WRONG_PUBLIC_KEY_FP:
            printf("Wrong public key file path\nPlease provide a valid file path for your cert.pem file\n");
            break;
        case WRONG_PRIVATE_KEY_FP:
            printf("Wrong private key file path\nPlease provide a valid file path for your key.pem file\n");
            break;
        case SSL_CONNECTION_REFUSED:
            printf("SSL connection refused\n");
            break;
        case ACCEPT_FAILED:
            printf("Accept failed\n");
            break;
        case SSL_ACCEPT_FAILED:
            printf("SSL accept failed\n");
            break;
        case SSL_CTX_CREATION_FAILED:
            printf("SSL_CTX_new returns NULL, check the openssl documentation\n");
            break;
        case SSL_CREATION_FAILED:
            printf("SSL_new returns NULL, check the openssl documentation\n");
            break;
        default:
            printf("no relevant error to print\n");
            break;
    }
}


SocketHandler* socket_ssl_client_init(const char* server_hostname, uint16_t server_port, const char* sni_hostname)
{
    SocketHandler* client;
    SSL_library_init();
    OpenSSL_add_all_algorithms();  /* Load cryptos, et.al. */
    SSL_load_error_strings();   /* Bring in and register error messages */

    client = socket_client_init(server_hostname, server_port);
    if(client == NULL)
        return NULL;

    client->ctx = SSL_CTX_new(SSLv23_client_method());
    if(client->ctx == NULL)
    {
        _error_code = SSL_CTX_CREATION_FAILED;
        socket_close(&client);
        return NULL;
    }
    client->ssl = SSL_new(client->ctx);
    if(client->ssl == NULL)
    {
        _error_code = SSL_CREATION_FAILED;
        socket_close(&client);
        return NULL;
    }

    if(sni_hostname == NULL)
    {
        SSL_set_tlsext_host_name(client->ssl, server_hostname);
    }
    else
    {
        SSL_set_tlsext_host_name(client->ssl, sni_hostname);
    }
    SSL_set_fd(client->ssl, client->fd);

    if (SSL_connect(client->ssl) == -1)
    {
        _error_code = SSL_CONNECTION_REFUSED;
        socket_close(&client);
        return NULL;
    }

    _error_code = 0;

    return client;
}

char set_blocking_mode(int fd, char blocking)
{
   if (fd < 0) return 0;

#ifdef _WIN32
   unsigned long mode = blocking ? 0 : 1;
   return (ioctlsocket(fd, FIONBIO, &mode) == 0) ? 1 : 0;
#else
   int flags = fcntl(fd, F_GETFL, 0);
   if (flags == -1) return 0;
   flags = blocking ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK);
   return (fcntl(fd, F_SETFL, flags) == 0) ? 1 : 0;
#endif
}

char timeout_connect(int fd, const struct sockaddr* name, int namelen, int timeout_sec)
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
    #ifndef IS_WINDOWS
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

SocketHandler* socket_client_init(const char* server_hostname, uint16_t server_port)
{
    struct addrinfo hints;
    struct addrinfo* result;
    struct addrinfo* rp;
    char str_server_port[8];  // 2**16 = 65536 (5 chars)
    SocketHandler* client;

    client = (SocketHandler*) malloc(sizeof(SocketHandler));

    if(client == NULL)
    {
        // out of memory, stop everything
        exit(1);
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;       /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM;   /* TCP socket */
    hints.ai_flags = 0;
    hints.ai_protocol = IPPROTO_TCP;   /* Any protocol */

    if(getaddrinfo(server_hostname, uint16_to_str(str_server_port, server_port), &hints, &result))
    {
        _error_code = UNABLE_TO_FIND_ADDRESS;
        free(client);
        return NULL;
    }

    for(rp = result; rp != NULL; rp = rp->ai_next)
    {   
        client->fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (client->fd == -1)
            continue;

        if (timeout_connect(client->fd, rp->ai_addr, rp->ai_addrlen, 10))
            break;                  /* Success */

        close(client->fd);
    }

    freeaddrinfo(result);

    if(client->fd == -1)
    {
        _error_code = SOCKET_ATTRIBUTION_ERROR;
        free(client);
        return NULL;
    }

    if (rp == NULL)
    {
        _error_code = CONNECTION_REFUSED;
        free(client);
        return NULL;
    }
    
    client->ssl = NULL;
    client->ctx = NULL;
    
    _error_code = 0;

    return client;
}

SocketHandler* socket_ssl_server_init(const char* server_hostname, uint16_t server_port, int max_connections, const char* public_key_fp, const char* private_key_fp)
{
    SocketHandler* server;
    SSL_library_init();
    OpenSSL_add_all_algorithms();  /* Load cryptos, et.al. */
    SSL_load_error_strings();   /* Bring in and register error messages */

    server = socket_server_init(server_hostname, server_port, max_connections);
    if(server == NULL)
        return NULL;

    server->ctx = SSL_CTX_new(SSLv23_server_method());
    if(server->ctx == NULL)
    {
        _error_code = SSL_CTX_CREATION_FAILED;
        socket_close(&server);
        return NULL;
    }

    if(SSL_CTX_use_certificate_file(server->ctx, public_key_fp, SSL_FILETYPE_PEM) <= 0)
    {
        _error_code = WRONG_PUBLIC_KEY_FP;
        socket_close(&server);
        return NULL;
    }
    if(SSL_CTX_use_PrivateKey_file(server->ctx, private_key_fp, SSL_FILETYPE_PEM) <= 0)
    {
        _error_code = WRONG_PRIVATE_KEY_FP;
        socket_close(&server);
        return NULL;
    }

    _error_code = 0;

    return server;
}

SocketHandler* socket_server_init(const char* server_hostname, uint16_t server_port, int max_connections)
{
    struct addrinfo hints;
    struct addrinfo* result;
    struct addrinfo* rp;
    char str_server_port[8];  // 2**16 = 65536 (5 chars)
    SocketHandler* server;

    server = (SocketHandler*) malloc(sizeof(SocketHandler));

    if(server == NULL)
    {
        // out of memory, stop everything
        exit(1);
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;      /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM;  /* TCP socket */
    hints.ai_flags = AI_PASSIVE;      /* For wildcard IP address */
    hints.ai_protocol = 0;            /* Any protocol */
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

    if(getaddrinfo(server_hostname, uint16_to_str(str_server_port, server_port), &hints, &result))
    {
        _error_code = UNABLE_TO_FIND_ADDRESS;
        free(server);
        return NULL;
    }

    for(rp = result; rp != NULL; rp = rp->ai_next)
    {
        server->fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (server->fd == -1)
            continue;

        if (bind(server->fd, rp->ai_addr, rp->ai_addrlen) != -1)
            break;                  /* Success */

        close(server->fd);
    }

    freeaddrinfo(result);

    if(server->fd == -1)
    {
        _error_code = SOCKET_ATTRIBUTION_ERROR;
        free(server);
        return NULL;
    }

    if (rp == NULL)
    {
        _error_code = CONNECTION_REFUSED;
        free(server);
        return NULL;
    }

    server->ssl = NULL;
    server->ctx = NULL;

    struct timeval tv;
    tv.tv_sec = 60;
    tv.tv_usec = 0;
    setsockopt(server->fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
 
         
    if (listen(server->fd, max_connections) != 0)
    {
        _error_code = UNABLE_TO_LISTEN;
        free(server);
        return NULL;
    }
    
    _error_code = 0;
    
    return server;
}

SocketHandler* socket_accept(SocketHandler* server, ClientData* pclient_data)
{
    SocketHandler* client;
    struct sockaddr_in peer_addr;
    #ifdef IS_WINDOWS
        int addr_size = sizeof(struct sockaddr_in);
    #else
        unsigned int addr_size = sizeof(struct sockaddr_in);
    #endif

    client = (SocketHandler*) malloc(sizeof(SocketHandler));

    if(client == NULL)
    {
        // out of memory, stop everything
        exit(1);
    }

    client->fd = accept(server->fd, (struct sockaddr*) &peer_addr, &addr_size);
    client->ssl = NULL;
    client->ctx = NULL;

    if(client->fd <= 0)
    {
        printf("%d\n", server->fd);
        _error_code = ACCEPT_FAILED;
        socket_close(&client);
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
            _error_code = SSL_ACCEPT_FAILED;
            socket_close(&client);
            return NULL;
        }
    }

    _error_code = 0;

    return client;
}

int socket_send(SocketHandler* s, const char* buffer, int n, int flags)
{
    if(s->ssl == NULL)
    {
        return send(s->fd, buffer, n, flags);
    }
    else
    {
        return SSL_write(s->ssl, buffer, n);
    }
}

int socket_recv(SocketHandler* s, char* buffer, int n, int flags)
{
    if(s->ssl == NULL)
    {
        return recv(s->fd, buffer, n, flags);
    }
    else
    {
        return SSL_read(s->ssl, buffer, n);
    }
}

void socket_close(SocketHandler** pps)
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
    #ifdef IS_WINDOWS
        closesocket((*pps)->fd);
    #else
        close((*pps)->fd);
    #endif

    free(*pps);
    *pps = NULL;
}