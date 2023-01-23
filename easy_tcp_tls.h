#define ADDRSTRLEN 22

enum ERROR_CODES {
    SOCKET_ATTRIBUTION_ERROR = -1,
    CONNECTION_REFUSED = -2,
    UNABLE_TO_BIND = -3,
    UNABLE_TO_LISTEN = -4,
    WRONG_PUBLIC_KEY_FP = -5,
    WRONG_PRIVATE_KEY_FP = -6,
    SSL_CONNECTION_REFUSED = -7,
    ACCEPT_FAILED = -8,
    SSL_ACCEPT_FAILED = -9,
    SSL_CTX_CREATION_FAILED = -10,
    SSL_CREATION_FAILED = -11
};

typedef struct client_data {
    char ip[ADDRSTRLEN];
    uint16_t port;
} ClientData;

typedef struct socket_handler SocketHandler;

void socket_start(void);
void socket_cleanup(void);
SocketHandler* socket_ssl_server_init(const char* server_ip, uint16_t server_port, int max_connections, const char* public_key_fp, const char* private_key_fp);
SocketHandler* socket_ssl_client_init(const char* server_ip, uint16_t server_port, const char* sni_hostname);
SocketHandler* socket_client_init(const char* server_ip, uint16_t server_port);
SocketHandler* socket_server_init(const char* server_ip, uint16_t server_port, int max_connections);
SocketHandler* socket_accept(SocketHandler* server, ClientData* pclient_data);
int socket_send(SocketHandler* s, const char* buffer, int n, int flags);
int socket_recv(SocketHandler* s, char* buffer, int n, int flags);
void socket_close(SocketHandler** pps);
int socket_get_last_error(void);
void socket_print_last_error(void);