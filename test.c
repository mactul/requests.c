#include <stdio.h>
#include "requests.h"


int main()
{
    char buffer[1024];
    RequestsHandler handler;
    int size;
    int error;

    requests_init();
    
    error = get(&handler, "https://example.com", "");  // "" is for no additionals headers
    
    if(error >= 0)
    {
        while((size = read_output_body(&handler, buffer, 1024)) > 0)
        {
            buffer[size] = '\0';
            printf("%s", buffer);
        }
        
        close_connection(&handler);
    }
    else
    {
        printf("error code: %d\n", error);
    }

    requests_cleanup();
}