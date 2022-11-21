#include <stdint.h>
#include <unistd.h>
#include <openssl/ssl.h>
#ifdef __unix__
    #include <netdb.h>
    #include <arpa/inet.h>
    #include <netinet/in.h>
    #include <sys/socket.h>

    #define socket_cleanup() (void)0
    #define socket_start() (void)0

#elif defined(_WIN32) || defined(WIN32)
    #include <winsock2.h>
    #include <ws2tcpip.h>
    //#pragma comment(lib, "ws2_32.lib")

    #define IS_WINDOWS 1

    #define socket_cleanup() WSACleanup()
    #define socket_start() WSADATA WSAData; WSAStartup(MAKEWORD(2,0), &WSAData);

#endif


enum ERROR_CODES {
    SOCKET_ATTRIBUTION_ERROR = -1,
    CONNECTION_REFUSED = -2,
    UNABLE_TO_BIND = -3,
    UNABLE_TO_LISTEN = -4,
    WRONG_PUBLIC_KEY_FP = -5,
    WRONG_PRIVATE_KEY_FP = -6,
    SSL_CONNECTION_REFUSED = -7
};

typedef struct client_data {
    char ip[INET_ADDRSTRLEN];
    uint16_t port;
} ClientData;

typedef struct socket_handler {
    int fd;
    SSL* ssl;
    SSL_CTX* ctx;
} SocketHandler;


char socket_ssl_server_init(SocketHandler* server, const char* server_ip, uint16_t server_port, int max_connections, const char* public_key_fp, const char* private_key_fp);
char socket_ssl_client_init(SocketHandler* client, const char* server_ip, uint16_t server_port, const char* sni_hostname);
char socket_client_init(SocketHandler* client, const char* server_ip, uint16_t server_port);
char socket_server_init(SocketHandler* server, const char* server_ip, uint16_t server_port, int max_connections);
char socket_accept(SocketHandler* client, SocketHandler* server, ClientData* pclient_data);
int socket_send(SocketHandler* s, const char* buffer, int n, int flags);
int socket_recv(SocketHandler* s, char* buffer, int n, int flags);
void socket_close(SocketHandler* s);