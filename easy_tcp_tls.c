#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
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

#endif

struct socket_handler {
    int fd;
    SSL* ssl;
    SSL_CTX* ctx;
};

static int _error_code = 0;

void socket_start(void)
{
    #ifdef IS_WINDOWS
        WSADATA WSAData;
        WSAStartup(MAKEWORD(2,0), &WSAData);
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


SocketHandler* socket_ssl_client_init(const char* server_ip, uint16_t server_port, const char* sni_hostname)
{
    SocketHandler* client;
    SSL_library_init();
    OpenSSL_add_all_algorithms();  /* Load cryptos, et.al. */
    SSL_load_error_strings();   /* Bring in and register error messages */

    client = socket_client_init(server_ip, server_port);
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
        SSL_set_tlsext_host_name(client->ssl, server_ip);
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

SocketHandler* socket_client_init(const char* server_ip, uint16_t server_port)
{
    struct sockaddr_in my_addr;
    SocketHandler* client;

    client = (SocketHandler*) malloc(sizeof(SocketHandler));

    if(client == NULL)
    {
        // out of memory, stop everything
        exit(1);
    }

    client->fd = socket(AF_INET, SOCK_STREAM, 0);
    
    client->ssl = NULL;
    client->ctx = NULL;

    if (client->fd < 0)
    {
        _error_code = SOCKET_ATTRIBUTION_ERROR;
        free(client);
        return NULL;
    }
    
    my_addr.sin_family = AF_INET;
    my_addr.sin_addr.s_addr = INADDR_ANY;
    my_addr.sin_port = htons(server_port);
    

    my_addr.sin_addr.s_addr = inet_addr(server_ip);
    if (connect(client->fd, (struct sockaddr*) &my_addr, sizeof my_addr) != 0)
    {
        _error_code = CONNECTION_REFUSED;
        free(client);
        return NULL;
    }
    
    _error_code = 0;

    return client;
}

SocketHandler* socket_ssl_server_init(const char* server_ip, uint16_t server_port, int max_connections, const char* public_key_fp, const char* private_key_fp)
{
    SocketHandler* server;
    SSL_library_init();
    OpenSSL_add_all_algorithms();  /* Load cryptos, et.al. */
    SSL_load_error_strings();   /* Bring in and register error messages */

    server = socket_server_init(server_ip, server_port, max_connections);
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

SocketHandler* socket_server_init(const char* server_ip, uint16_t server_port, int max_connections)
{
    SocketHandler* server;

    server = (SocketHandler*) malloc(sizeof(SocketHandler));

    if(server == NULL)
    {
        // out of memory, stop everything
        exit(1);
    }

    server->fd = socket(AF_INET, SOCK_STREAM, 0);
    server->ssl = NULL;
    server->ctx = NULL;

    if (server->fd < 0)
    {
        _error_code = SOCKET_ATTRIBUTION_ERROR;
        free(server);
        return NULL;
    }

    struct timeval tv;

    struct sockaddr_in my_addr;
    my_addr.sin_family = AF_INET;
    my_addr.sin_addr.s_addr = INADDR_ANY;
    
    my_addr.sin_addr.s_addr = inet_addr(server_ip);
    my_addr.sin_port = htons(server_port);

    tv.tv_sec = 60;
    tv.tv_usec = 0;
    setsockopt(server->fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
 
    if (bind(server->fd, (struct sockaddr*) &my_addr, sizeof(my_addr)) != 0)
    {
        _error_code = UNABLE_TO_BIND;
        free(server);
        return NULL;
    }
         
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
    int addr_size = sizeof(struct sockaddr_in);

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
        return recv(s->fd, buffer, n, flags);
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
    char buffer[2];
    socket_recv(*pps, buffer, 1, 0);  // Yes, this is ugly, but it's the only way I found to wait for the end of precedent operations

    if((*pps)->ssl != NULL)
    {
        SSL_shutdown((*pps)->ssl);
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