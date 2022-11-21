#include "easy_tcp_tls.h"
#define MAX_URI_LENGTH  1024 /* this can be changed, it's the maximum length a url can have */

/* This is needed for windows */
#define requests_init() socket_start()
#define requests_cleanup() socket_cleanup()

enum errors {
    ERROR_MAX_CONNECTIONS = -1,
    ERROR_SSL = -2,
    ERROR_HOST_CONNECTION = -3,
    ERROR_WRITE = -4,
    ERROR_PROTOCOL = -5,
    ERROR_MALLOC = -6,
    UNABLE_TO_BUILD_SOCKET = -7
};

typedef struct requests_handler {
    SocketHandler handler;
    char headers_readed;
} RequestsHandler;

char request(RequestsHandler* handler, char* method, char* url, char* data, char* headers);
char post(RequestsHandler* handler, char* url, char* data, char* headers);
char get(RequestsHandler* handler, char* url, char* headers);
char delete(RequestsHandler* handler, char* url, char* headers);
char patch(RequestsHandler* handler, char* url, char* data, char* additional_headers);
char put(RequestsHandler* handler, char* url, char* data, char* additional_headers);
int read_output(RequestsHandler* handler, char* buffer, int buffer_size);
int read_output_body(RequestsHandler* handler, char* buffer, int buffer_size);
void close_connection(RequestsHandler* handler);