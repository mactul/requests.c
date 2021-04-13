enum errors {
    ERROR_MAX_CONNECTIONS = -1,
    ERROR_SSL = -2,
    ERROR_HOST_CONNECTION = -3,
    ERROR_WRITE = -4,
    ERROR_PROTOCOL = -5
};

typedef int Handler;

Handler http_request(char* hostname, char* headers);
Handler https_request(char* hostname, char* headers);
Handler request(char* method, char* url, char* data, char* headers);
Handler post(char* url, char* data, char* headers);
Handler get(char* url, char* headers);
Handler delete(char* url, char* headers);
Handler patch(char* url, char* data, char* additional_headers);
Handler put(char* url, char* data, char* additional_headers);
int _read_sock(Handler handler, char* buffer, int buffer_size);
char* read_output_body(Handler handler);
char read_output(Handler handler, char* buffer, int buffer_size);
void close_connection(Handler handler);
void int_to_string(int n, char s[]);
void reverse_string(char s[]);