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