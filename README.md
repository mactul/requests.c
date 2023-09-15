# requests.c

requests.c is an easy to use library, influenced by requests.py, to make http requests

## Change logs

Since the last version, many things have been changed in the library.  
The `req_read_output_body` is now cleaner and more reliable.  
It's no longer possible to use the `req_read_output` function, but now, all headers are automatically parsed.  
You can use the `req_get_header_value` function to get the value returned by the server for a specific header.
For debugging purpose, you can use the `req_display_headers` function to see all the headers parsed.

## Installation

### On debian/Unbuntu
```
git clone https://github.com/mactul/requests.c.git

cd requests.c

sh install_debian.sh
```

The Makefile is here to create the bin/requests.a file  
You can just copy this file and use it in your code like that:

```sh
gcc -o program ./your_code.c ./requests.a -lcrypto -lssl
```

## Documentation

first example:
```c
#include <stdio.h>
#include "requests.h"

int main()
{
    char buffer[1025];
    RequestsHandler* handler;
    int size;

    req_init();  // This is for Windows compatibility, it do nothing on Linux. If you forget it, the program will fail silently.
    
    handler = req_get("https://raw.githubusercontent.com/mactul/requests.c/master/requests.c", "");  // "" is for no additionals headers
    
    if(handler != NULL)
    {
        req_display_headers(handler);

        while((size = req_read_output_body(handler, buffer, 1024)) > 0)
        {
            buffer[size] = '\0';
            printf("%s", buffer);
        }
        printf("\n");

        req_close_connection(&handler);
    }
    else
    {
        printf("error code: %d\n", req_get_last_error());
    }

    req_cleanup();  // again, for Windows compatibility
}
```
\
Here is all the connection functions
```c
RequestsHandler* req_get(char* url, char* additional_headers);
RequestsHandler* req_post(char* url, char* data, char* additional_headers);
RequestsHandler* req_delete(RequestsHandler* handler, char* url, char* additional_headers);
RequestsHandler* req_patch(RequestsHandler* handler, char* url, char* data, char* additional_headers);
RequestsHandler* req_put(RequestsHandler* handler, char* url, char* data, char* additional_headers);
RequestsHandler* req_head(char* url, char* additional_headers);
```
\
url needs to start with `http://` or `https://`\
\
data need to be formated like the server want
for example, for a standard post, it will be like that
```c
"key1=value1&key2=value2&key3=value3"
```
\
headers are separeted by `\r\n` __they needs__ to finish by `\r\n`\
for example, I can add this header
```c
"Content-Type: application/json\r\n"
```

An example of a post in `Connection: keep-alive`:
```c
#include <stdio.h>
#include "requests.h"

int main()
{
    char buffer[1025];
    RequestsHandler* handler;
    int size;

    req_init();  // This is for Windows compatibility, it do nothing on Linux. If you forget it, the program will fail silently.
    
    handler = req_post("https://example.com", "user=MY_USER&password=MY_PASSWORD", "Connection: keep-alive\r\n");
    
    if(handler != NULL)
    {strcase
        req_display_headers(handler);

        while((size = req_read_output_body(handler, buffer, 1024)) > 0)
        {
            buffer[size] = '\0';
            printf("%s", buffer);
        }
        printf("\n");

        req_close_connection(&handler);
    }
    else
    {
        printf("error code: %d\n", req_get_last_error());
    }

    req_cleanup();  // again, for Windows compatibility
}
```

### Functions documentation:

- `void req_init(void);`
    - This function must be set when the program starts. If your program use multiple threads, make sure to call it a single time.
    - It's just for Windows compatibility, it do nothing on Linux, but if you forget it on windows, you will have weird bugs.

- `void req_cleanup(void);`
    - This function must be set at the end of the program, make sure you have killed all sockets in all threads before calling this.
    - It's just for Windows compatibility, it do nothing on Linux.

- `int req_get_last_error(void);`
    - This returns the error code of the last error occured.

- `const char* req_get_header_value(RequestsHandler* handler, char* header_name);`
    - This will return the value of an header in the server response.
    - example:
        - `const char* content_length = req_get_header_value(handler, "content-length");` will set the number of bytes of the response in the string content_length, or NULL if the server does not provide this information.
    - returns NULL if the header is not in the server response.
    - **Warning !** If you have to modify the string returned, copy it in a new buffer.

- `short int req_get_status_code(RequestsHandler* handler);`
    - This will return the server response code, you can usually check if this code is >= 400 to know if there is an error. (see http status code for more information)


- Documentation in progress...