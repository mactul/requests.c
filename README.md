- [Requests](#requests)
  - [__Functions__](#functions)
    - [req\_get](#req_get)
    - [req\_post](#req_post)
    - [req\_delete](#req_delete)
    - [req\_patch](#req_patch)
    - [req\_put](#req_put)
    - [req\_head](#req_head)
    - [req\_request](#req_request)
    - [req\_read\_output\_body](#req_read_output_body)
    - [req\_close\_connection](#req_close_connection)
    - [req\_get\_header\_value](#req_get_header_value)
    - [req\_get\_status\_code](#req_get_status_code)
    - [req\_display\_headers](#req_display_headers)
  - [__Concepts__](#concepts)
    - [Url formatting](#url-formatting)
    - [Data formatting](#data-formatting)
    - [Headers formatting](#headers-formatting)
    - [Keep-alive](#keep-alive)
  - [__Examples__](#examples)
    - [Post - keep-alive disabled](#post---keep-alive-disabled)
    - [Get - Keep-alive enabled](#get---keep-alive-enabled)


# Requests

Requests is an easy to use library, influenced by requests.py, to make http requests


## __Functions__

### req_get
```c
RequestsHandler* req_get(RequestsHandler* handler, const char* url, const char* additional_headers);
```
- send a GET request.
- **parameters**
    - `handler`: must be NULL if it's the first connection, otherwise, it should be an old handler (see [keep-alive](#keep-alive) for more information).
    - `url`: It's the url you want to request, it should start with `http://` or `https://`.
    - `additional_headers`: the headers you want to specify, they are separated by `\r\n` and __they need__ to finish by `\r\n`.
- **returns**
    - When it succeeds, it returns a pointer to a structure handler.
    - When it fails, it returns NULL

### req_post
```c
RequestsHandler* req_post(RequestsHandler* handler, const char* url, const char* data, const char* additional_headers);
```
- send a POST request.
- **parameters**
    - `handler`: must be NULL if it's the first connection, otherwise, it should be an old handler (see [keep-alive](#keep-alive) for more information).
    - `url`: It's the url you want to request, it should start with `http://` or `https://`.
    - `data`: it's the body of the request.
    - `additional_headers`: the headers you want to specify, they are separated by `\r\n` and __they needs__ to finish by `\r\n`.
- **returns**
    - When it succeeds, it returns a pointer to a structure handler.
    - When it fails, it returns NULL

### req_delete
```c
RequestsHandler* req_delete(RequestsHandler* handler, const char* url, const char* additional_headers);
```
- send a DELETE request.
- It works exactly like [req_get](#req_get).

### req_patch
```c
RequestsHandler* req_patch(RequestsHandler* handler, const char* url, const char* data, const char* additional_headers);
```
- send a PATCH request.
- It works exactly like [req_post](#req_post).

### req_put
```c
RequestsHandler* req_put(RequestsHandler* handler, const char* url, const char* data, const char* additional_headers);
```
- send a PUT request.
- It works exactly like [req_post](#req_post).

### req_head
```c
RequestsHandler* req_head(RequestsHandler* handler, const char* url, const char* additional_headers);
```
- send a HEAD request.
- Note: even if the server send a body response to this request (which is not possible in the HTTP standard), you will not be able to get it with [req_read_output_body](#req_read_output_body).
- For the rest, it works like [req_get](#req_get)

### req_request
```c
RequestsHandler* req_request(RequestsHandler* handler, const char* method, const char* url, const char* data, const char* additional_headers);
```
- **This is not meant to be used directly**, unless you have exotic HTTP methods.  
It's the generic method for all other HTTP methods.
- **parameters**
    - `handler`: must be NULL if it's the first connection, otherwise, it should be an old handler (see [keep-alive](#keep-alive) for more information).
    - `method`: This parameter must be in CAPS LOCK, followed by a space, like `"GET "`, `"POST "`, etc...
    - `url`: It's the url you want to request, it should start with `http://` or `https://`.
    - `data`: it's the body of the request.
    - `additional_headers`: the headers you want to specify, they are separated by `\r\n` and __they needs__ to finish by `\r\n`.
- **returns**
    - When it succeeds, it returns a pointer to a structure handler.
    - When it fails, it returns NULL


### req_read_output_body
```c
int req_read_output_body(RequestsHandler* handler, char* buffer, int buffer_size);
```
- This function is what makes this library interesting alongside CURL.  
It provides an interface that is like the usual socket read function.  
This function skip the headers and recreate the body from chunks.
- It's a blocking function, it waits for the data to arrive before transmitting them.
However, if there is no data left, you can call the function as many times as you want, it will never block you, it will just return 0 or a negative number if there is a problem.
- This function fill the buffer entirely, excepted for the last one.
- **parameters**
  - `handler`: the handler returned by a request
  - `buffer`: a buffer to fill with the data read
  - `buffer_size`: the size of the buffer. Don't forget to remove 1 byte from your real buffer size if you are reading text and you want to add an `'\0'` at the end of the buffer.
- **returns**:
    - If it succeeded, it returns the number of bytes read and put in `buffer`.
    - If it fails, it returns -1 and errno contains more information.


### req_close_connection
```c
void req_close_connection(RequestsHandler** ppr);
```
- this function will close the connection, destroy the headers parsed tree, free all structures behind the handler and put your handler to `NULL`.
- **parameters**
  - `ppr`: the address of your handler. It's a pointer to a pointer


### req_get_header_value
```c
const char* req_get_header_value(RequestsHandler* handler, char* header_name);
```
- Get one of the parsed headers in the server response.
- **parameters**
    - `handler`: the handler returned by a request.
    - `header_name`: the name of the header you want to get, for example `"content-length"` (this is case unsensitive).
- **returns**
    - if the header exists in the server response, it will returns his value as a string.  
    **Warning !** If you have to modify the string returned, copy it in a new buffer.
    - if the headers is not returned by the server, it returns NULL.
- **example**:
    ```c
    const char* content_type = req_get_header_value(handler, "content-type");
    ```
    Will set what the server returned as content-type in the content_type string, or NULL if the server does not provide this information.


### req_get_status_code
```c
unsigned short int req_get_status_code(RequestsHandler* handler);
```
- This will return the server response code, you can usually check if this code is >= 400 to know if there is an error. (see http status code for more information)
- **parameters**
    - `handler`: the handler returned by a request.
- **returns**
    - an `unsigned short` which is the HTTP code


### req_display_headers
```c
void req_display_headers(RequestsHandler* handler);
```
- This is for debugging purpose, it will print the list of headers provided by the server.
- **parameters**
    - `handler`: the handler returned by a request.


## __Concepts__

### Url formatting

Requests is able to automatically parse urls, extract the domain the port, and the relative uri.  
However, to work, url needs to start with `http://` or `https://`.  
Url starting with https:// use a layer of SSL cyphering.
You can change the default port of the url (which is 80 for http and 443 for https)
`https://example.com:7890/test` will use the port 7890 and will be secured over SSL.
`http://example.com:4706/test` will use the port 4706 and will **not** be secured over SSL.

### Data formatting

If you send data (via post, put or patch), they need to be formatted like the server want
for example, for a standard post *(application/x-www-form-urlencoded)*, it will be like that
```c
"key1=value1&key2=value2&key3=value3"
```

### Headers formatting

You can always provide a empty string to the `additional_headers` parameter; However, if you provide any additional headers, they are separated by `\r\n` and __they need__ to finish by `\r\n`\
for example, I can add this header
```c
"Content-Type: application/json\r\n"
```
Or these 2 headers:
```c
"Content-Type: application/json\r\nUser-Agent: requests.c\r\n"
```
If you forget the ending `\r\n` you will have a failure.


### Keep-alive
All the requests are keep-alive by default. However, if you provide NULL for the handler parameter in the request, you will never exploit this acceleration.
When you do multiple requests to the same server, you can provide the handler of the precedent request to the new one, to avoid reconnection.

**/!\\ Warning /!\\** Make sure to always update the handler when you create connections with an old handler, like this:
```c
handler = req_get(handler, ...)
```
If you don't do this, if the connection falls, your handler will be replaced by a new one that you'll never get back.  
*Note: on GNU systems, the compiler will warn you if you do that.*

To see a keep-alive example, see [Get - keep-alive enabled](#get---keep-alive-enabled).

## __Examples__

### Post - keep-alive disabled

An example of a post in `Connection: close`:
```c
#include <stdio.h>
#include "requests.h"

int main()
{
    char buffer[1024];
    RequestsHandler* handler;
    int size;

    req_init();  // if you forget that, your code will never work on Windows and the socket creation will just fail without a warning

    handler = req_post(NULL, "https://example.com", "user=MY_USER&password=MY_PASSWORD", "Connection: close\r\n");

    if(handler == NULL)
    {
        goto PROGRAM_END;
    }

    req_display_headers(handler);

    while((size = req_read_output_body(handler, buffer, sizeof(buffer)-1)) > 0)
    {
        buffer[size] = '\0';
        printf("%s", buffer);
    }
    putchar('\n');

    req_close_connection(&handler);

PROGRAM_END:
    req_destroy();
}
```


### Get - Keep-alive enabled

This example download on the same host 3 times, then download another host (this will cause the handler to change).
```c
#include <stdio.h>
#include "requests.h"

int main()
{
    char buffer[1024];
    RequestsHandler* handler = NULL;  // It's very important to set the handler to NULL for the first connection, otherwise, you will have unexpected behaviors.
    int size;
    char* url_array[] = {
        "https://raw.githubusercontent.com/mactul/system_abstraction/main/requests_helper/network/requests.c",
        "https://raw.githubusercontent.com/mactul/system_abstraction/main/requests_helper/network/requests.h",
        "https://raw.githubusercontent.com/mactul/system_abstraction/main/requests_helper/network/easy_tcp_tls.h",
        "https://example.com/test"
    };

    req_init();  // if you forget that, your code will never work on Windows and the socket creation will just fail without a warning

    for(int i = 0; i < 4; i++)
    {
        handler = req_get(handler, url_array[i], "");  // "" is for no additional headers

        if(handler == NULL)
        {
            goto PROGRAM_END;
        }

        req_display_headers(handler);

        while((size = req_read_output_body(handler, buffer, sizeof(buffer)-1)) > 0)
        {
            buffer[size] = '\0';
            printf("%s", buffer);
        }
        putchar('\n');
    }

PROGRAM_END:
    req_close_connection(&handler);  // Please note that the close is only done for the last get. If you do it in the loop, the program will work, however, you will not benefit from the keep-alive speed improvement.

    req_destroy();
}
```