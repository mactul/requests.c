# requests.c

requests.c is an easy to use library, influenced by requests.py, to make http requests

## Installation

On debian
```
sudo apt-get install openssl

git clone https://github.com/mactul/requests.c.git

cd requests.c

sh install_debian.sh
```


## Documentation

first example:
```c
#include <stdio.h>
#include "requests.h"


int main()
{
    char buffer[1024];
    Handler handler;
    
    handler = get("http://info.cern.ch/hypertext/WWW/TheProject.html", "");  // "" is for no additionals headers
    
    if(handler >= 0)
    {
        while(read_output(handler, buffer, 1024))
        {
            printf("%s", buffer);
        }
        
        close_connection(handler);
    }
    else
    {
        printf("error code: %d\n", handler);
    }
}
```
\
Here is all the connection functions
```c
Handler post(char* url, char* data, char* headers);
Handler get(char* url, char* headers);
Handler delete(char* url, char* headers);
Handler patch(char* url, char* data, char* headers);
Handler put(char* url, char* data, char* headers);
```
\
url needs to start with http:// or https://\
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
