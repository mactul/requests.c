#define MAX_URI_LENGTH  1024 /* this can be changed, it's the maximum length a url can have */


enum errors {
    ERROR_MAX_CONNECTIONS = -1,
    ERROR_SSL = -2,
    ERROR_HOST_CONNECTION = -3,
    ERROR_WRITE = -4,
    ERROR_PROTOCOL = -5,
    ERROR_MALLOC = -6,
    UNABLE_TO_BUILD_SOCKET = -7
};

typedef struct requests_handler RequestsHandler;

void req_init(void);
void req_cleanup(void);
int req_get_last_error(void);
RequestsHandler* req_request(char* method, char* url, char* data, char* additional_headers);
RequestsHandler* req_get(char* url, char* additional_headers);
RequestsHandler* req_post(char* url, char* data, char* additional_headers);
RequestsHandler* req_delete(RequestsHandler* handler, char* url, char* additional_headers);
RequestsHandler* req_patch(RequestsHandler* handler, char* url, char* data, char* additional_headers);
RequestsHandler* req_put(RequestsHandler* handler, char* url, char* data, char* additional_headers);
int req_read_output(RequestsHandler* handler, char* buffer, int buffer_size);
int req_read_output_body(RequestsHandler* handler, char* buffer, int buffer_size);
void req_close_connection(RequestsHandler** ppr);