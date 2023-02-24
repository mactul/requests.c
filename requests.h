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
RequestsHandler* req_request(const char* method, const char* url, const char* data, const char* additional_headers);
RequestsHandler* req_get(const char* url, const char* additional_headers);
RequestsHandler* req_post(const char* url, const char* data, const char* additional_headers);
RequestsHandler* req_delete(const char* url, const char* additional_headers);
RequestsHandler* req_patch(const char* url, const char* data, const char* additional_headers);
RequestsHandler* req_put(const char* url, const char* data, const char* additional_headers);
RequestsHandler* req_head(const char* url, const char* additional_headers);
const char* req_get_header_value(RequestsHandler* handler, const char* header_name);
short int req_get_status_code(RequestsHandler* handler);
void req_display_headers(RequestsHandler* handler);
int req_read_output_body(RequestsHandler* handler, char* buffer, int buffer_size);
void req_close_connection(RequestsHandler** ppr);