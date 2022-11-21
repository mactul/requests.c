#include "easy_tcp_tls.h"
#include <errno.h>

//https://wiki.openssl.org/index.php/Simple_TLS_Server

char socket_ssl_client_init(SocketHandler* client, const char* server_ip, uint16_t server_port, const char* sni_hostname)
{
    char error_code;
    SSL_library_init();
    OpenSSL_add_all_algorithms();  /* Load cryptos, et.al. */
    SSL_load_error_strings();   /* Bring in and register error messages */

    error_code = socket_client_init(client, server_ip, server_port);
    if(error_code < 0)
        return error_code;

    client->ctx = SSL_CTX_new(SSLv23_client_method());
    client->ssl = SSL_new(client->ctx);
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
        return CONNECTION_REFUSED;
    }

    return 0;
}

char socket_client_init(SocketHandler* client, const char* server_ip, uint16_t server_port)
{
    struct sockaddr_in my_addr;
    client->fd = socket(AF_INET, SOCK_STREAM, 0);
    
    client->ssl = NULL;
    client->ctx = NULL;

    if (client->fd < 0)
        return SOCKET_ATTRIBUTION_ERROR;
    
    my_addr.sin_family = AF_INET;
    my_addr.sin_addr.s_addr = INADDR_ANY;
    my_addr.sin_port = htons(server_port);
    
    // This ip address is the server ip address
    my_addr.sin_addr.s_addr = inet_addr(server_ip);
    errno = 0;
    if (connect(client->fd, (struct sockaddr*) &my_addr, sizeof my_addr) != 0)
    {
        return CONNECTION_REFUSED;
    }
    
    return 0;
}

char socket_ssl_server_init(SocketHandler* server, const char* server_ip, uint16_t server_port, int max_connections, const char* public_key_fp, const char* private_key_fp)
{
    char error_code;
    SSL_library_init();
    OpenSSL_add_all_algorithms();  /* Load cryptos, et.al. */
    SSL_load_error_strings();   /* Bring in and register error messages */

    error_code = socket_server_init(server, server_ip, server_port, max_connections);
    if(error_code < 0)
        return error_code;

    server->ctx = SSL_CTX_new(SSLv23_server_method());

    if(SSL_CTX_use_certificate_file(server->ctx, public_key_fp, SSL_FILETYPE_PEM) <= 0)
        return WRONG_PUBLIC_KEY_FP;
    if(SSL_CTX_use_PrivateKey_file(server->ctx, private_key_fp, SSL_FILETYPE_PEM) <= 0)
        return WRONG_PRIVATE_KEY_FP;

    return 0;
}

char socket_server_init(SocketHandler* server, const char* server_ip, uint16_t server_port, int max_connections)
{
    #ifdef IS_WINDOWS
        WSADATA WSAData;
        WSAStartup(MAKEWORD(2,0), &WSAData);
    #endif

    server->fd = socket(AF_INET, SOCK_STREAM, 0);
    server->ssl = NULL;
    server->ctx = NULL;
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
        return UNABLE_TO_BIND;
         
    if (listen(server->fd, max_connections) != 0)
        return UNABLE_TO_LISTEN;
    
    return 0;
}

char socket_accept(SocketHandler* client, SocketHandler* server, ClientData* pclient_data)
{
    struct sockaddr_in peer_addr;
    int addr_size;
    addr_size = sizeof(struct sockaddr_in);

    client->fd = accept(server->fd, (struct sockaddr*) &peer_addr, &addr_size);
    client->ssl = NULL;
    client->ctx = NULL;

    if(client->fd <= 0)
    {
        return CONNECTION_REFUSED;
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
            return SSL_CONNECTION_REFUSED;
        }
    }
    return 0;
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

void socket_close(SocketHandler* s)
{
    if(s->ssl != NULL)
    {
        SSL_shutdown(s->ssl);
        SSL_free(s->ssl);
    }
    if(s->ctx != NULL)
    {
        SSL_CTX_free(s->ctx);
    }
    #ifdef IS_WINDOWS
        closesocket(s->fd);
    #else
        close(s->fd);
    #endif
}