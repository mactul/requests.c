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
        while((size = read_output_body(handler, buffer, 1024)) > 0)
        {
            buffer[size] = '\0';
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
\
\
To read the server response, you have two choices.\
The first one returns all the response, with headers.\
The second one returns only the body of the response, it is more easy, especially if you want to decode a string, like a json.\
Both solutions can read binary files like images
\
For both solutions, you need a buffer and you will fill it and use it in a loop, while there is data.\
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
        while((size = read_output(handler, buffer, 1024)) > 0)
        {
            buffer[size] = '\0';
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
The easiest solution is to do that\
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
        while((size = read_output_body(handler, buffer, 1024)) > 0)
        {
            buffer[size] = '\0';
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

You can notice that the 2 solutions have only the name of the reading function different.