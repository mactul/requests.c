# requests.c

requests.c is an easy to use library, influenced by requests.py, to make http requests

## Change logs

Since the last version, many things have been changed in the library.  
The `req_read_output_body` is now cleaner and more reliable.  
It's no longer possible to use the `req_read_output` function, but now, all headers are automatically parsed.  
You can use the `req_get_header_value` function to get the value returned by the server for a specific header.
For debugging purpose, you can use the `req_display_headers` function to see all the headers parsed.

Now, all the connections are by default in keep-alive. The request functions now need a new parameter, the handler of the precedent connection. If the connection is the first, you have to pass NULL.

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

### On other distributions

You have to install openssl and openssl-dev.  
Then, you can use the makefile like that:

```sh
make test
```

The Makefile is here to create the bin/requests.a file  
You can just copy this file and use it in your code like that:
```sh
gcc -o program ./your_code.c ./requests.a -lcrypto -lssl
```

### Windows

This program is compatible with windows, but you can't use the makefile to build a static library.


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
    
    handler = req_get(NULL, "https://raw.githubusercontent.com/mactul/requests.c/master/requests.c", "");  // "" is for no additionals headers
    
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
RequestsHandler* req_get(RequestsHandler* handler, char* url, char* additional_headers);
RequestsHandler* req_post(RequestsHandler* handler, char* url, char* data, char* additional_headers);
RequestsHandler* req_delete(RequestsHandler* handler, char* url, char* additional_headers);
RequestsHandler* req_patch(RequestsHandler* handler, char* url, char* data, char* additional_headers);
RequestsHandler* req_put(RequestsHandler* handler, char* url, char* data, char* additional_headers);
RequestsHandler* req_head(RequestsHandler* handler, char* url, char* additional_headers);
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

An example of a post in `Connection: close`:
```c
#include <stdio.h>
#include "requests.h"

int main()
{
    char buffer[1025];
    RequestsHandler* handler;
    int size;

    req_init();  // This is for Windows compatibility, it do nothing on Linux. If you forget it, the program will fail silently.
    
    handler = req_post(NULL, "https://example.com", "user=MY_USER&password=MY_PASSWORD", "Connection: close\r\n");
    
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

### Keep-alive
All the requests are keep-alive by default. However, if you provide NULL for the handler parameter in the request, you will never exploit this acceleration.
When you do multiple requests to the same server, you can provide the handler of the precedent request to the new one, to avoid retablishing the connection.

/!\ Warning ! Make sure to always update the handler when you create connections with an old handler, like this:
```c
handler = req_get(handler, ...)
```
If you don't do this, if the connection falls, your handler will be replaced by a new one that you'll never get back.

Here is a full example that download on the same host 3 times, then download another host (this will cause the handler to change).
```c
#include <stdio.h>
#include "requests.h"

int main()
{
    char buffer[1025];
    RequestsHandler* handler = NULL;
    int size;
    char* url_array[] = {
        "https://raw.githubusercontent.com/mactul/requests.c/master/requests.c",
        "https://raw.githubusercontent.com/mactul/requests.c/master/requests.h",
        "https://raw.githubusercontent.com/mactul/requests.c/master/Makefile",
        "https://example.com/test"
    };

    req_init();  // This is for Windows compatibility, it do nothing on Linux. If you forget it, the program will fail silently.
    
    for(int i = 0; i < 4; i++)
    {
        handler = req_get(handler, url_array[i], "");  // "" is for no additionals headers
        
        if(handler != NULL)
        {
            req_display_headers(handler);

            while((size = req_read_output_body(handler, buffer, 1024)) > 0)
            {
                buffer[size] = '\0';
                printf("%s", buffer);
            }
            printf("\n");
        }
        else
        {
            printf("error code: %d\n", req_get_last_error());
        }
    }


    req_close_connection(&handler);  // note that the close is only done for the last connection

    req_cleanup();  // again, for Windows compatibility
}
```

### Functions documentation:

- `void req_init(void);`
    - This function must be set when the program starts. If your program use multiple threads, make sure to call it a single time.
    - It's just for Windows compatibility, it do nothing on Linux, but if you forget it on windows, you will have weird bugs.

- `void req_cleanup(void);`
    - This function must be set at the end of the program, on windows it will kill all the sockets of your program (be careful if you use threads).
    - It's just for Windows compatibility, it do nothing on Linux.
    - It's not a big issue if you forget it, for a library, it might be better to not use it.

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


- `void req_display_headers(RequestsHandler* handler);`
    - This is for debugging purpose, it will print the list of headers provided by the server.


- `RequestsHandler* req_request(RequestsHandler* handler, const char* method, const char* url, const char* data, const char* additional_headers);`
    - **This is not meant to be used directly**, unless you have exotic HTTP methods.
    - `handler`: must be NULL if it's the first connection, otherwise, it should be an old handler (see [keep-alive](#keep-alive) for more information).
    - `method`: This parameter must be in CAPS LOCK, followed by a space, like `"GET "`, `"POST "`, etc...
    - `url`: It's the url you want to request, it should start with `http://` or `https://`. Currently, port number in the url isn't supported.
    - `data`: it's the body of the request.
    - `additional_headers`: the headers you want to specify, they are separeted by `\r\n` and __they needs__ to finish by `\r\n`.


- `RequestsHandler* req_get(RequestsHandler* handler, const char* url, const char* additional_headers);`
    - send a GET request.
    - `handler`: must be NULL if it's the first connection, otherwise, it should be an old handler (see [keep-alive](#keep-alive) for more information).
    - `url`: It's the url you want to request, it should start with `http://` or `https://`. Currently, port number in the url isn't supported.
    - `additional_headers`: the headers you want to specify, they are separeted by `\r\n` and __they needs__ to finish by `\r\n`.


- `RequestsHandler* req_post(RequestsHandler* handler, const char* url, const char* data, const char* additional_headers);`
    - send a POST request.
    - `handler`: must be NULL if it's the first connection, otherwise, it should be an old handler (see [keep-alive](#keep-alive) for more information).
    - `url`: It's the url you want to request, it should start with `http://` or `https://`. Currently, port number in the url isn't supported.
    - `data`: it's the body of the request.
    - `additional_headers`: the headers you want to specify, they are separeted by `\r\n` and __they needs__ to finish by `\r\n`.


- `RequestsHandler* req_delete(RequestsHandler* handler, const char* url, const char* additional_headers);`
    - send a DELETE request.
    - `handler`: must be NULL if it's the first connection, otherwise, it should be an old handler (see [keep-alive](#keep-alive) for more information).
    - `url`: It's the url you want to request, it should start with `http://` or `https://`. Currently, port number in the url isn't supported.
    - `additional_headers`: the headers you want to specify, they are separeted by `\r\n` and __they needs__ to finish by `\r\n`.


- `RequestsHandler* req_patch(RequestsHandler* handler, const char* url, const char* data, const char* additional_headers);`
    - send a PATCH request.
    - `handler`: must be NULL if it's the first connection, otherwise, it should be an old handler (see [keep-alive](#keep-alive) for more information).
    - `url`: It's the url you want to request, it should start with `http://` or `https://`. Currently, port number in the url isn't supported.
    - `data`: it's the body of the request.
    - `additional_headers`: the headers you want to specify, they are separeted by `\r\n` and __they needs__ to finish by `\r\n`.


- `Handler* req_put(RequestsHandler* handler, const char* url, const char* data, const char* additional_headers);`
    - send a PUT request.
    - `handler`: must be NULL if it's the first connection, otherwise, it should be an old handler (see [keep-alive](#keep-alive) for more information).
    - `url`: It's the url you want to request, it should start with `http://` or `https://`. Currently, port number in the url isn't supported.
    - `data`: it's the body of the request.
    - `additional_headers`: the headers you want to specify, they are separeted by `\r\n` and __they needs__ to finish by `\r\n`.

- `Handler* req_head(RequestsHandler* handler, const char* url, const char* additional_headers);`
    - send a HEAD request.
    - Note: even if the server send a body response to this request (which is not possible in the HTTP standard), you will not be able to get it with `req_read_output_body`.
    - `handler`: must be NULL if it's the first connection, otherwise, it should be an old handler (see [keep-alive](#keep-alive) for more information).
    - `url`: It's the url you want to request, it should start with `http://` or `https://`. Currently, port number in the url isn't supported.
    - `additional_headers`: the headers you want to specify, they are separeted by `\r\n` and __they needs__ to finish by `\r\n`.


- `int req_read_output_body(RequestsHandler* handler, char* buffer, int buffer_size);`
    - This function is what makes this library interesting alongside CURL
    It provides an interface that is like the usual socket read function.
    This function skip the headers and recreate the body from chunks.
    - It's a blocking function, it wait for the data to arrive before transmitting them.
    However, if there is no data left, you can call the function as many times as you want, it will never block you, it will just return 0 or a negative number if there is a problem.
    - This function fill the buffer entirely, excepted for the last one.
    - return:
        - it returns the number of bytes readed and put in `buffer`.
    - `handler`: the handler returned by a request
    - `buffer`: a buffer to fill with the data readed
    - `buffer_size`: the size of the buffer. Don't forget to remove 1 byte from your real buffer size if you are reading text and you want to add an `'\0'` at the end of the buffer.


- `void req_close_connection(RequestsHandler** ppr);`
    - this function will close the connection, destroy the headers parsed tree and free all structures behind the handler and put your handler to `NULL`.
    - `ppr`: the adress of your handler.
