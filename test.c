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