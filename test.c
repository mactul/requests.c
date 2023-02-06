#include <stdio.h>
#include "requests.h"
#include <windows.h>

int main()
{
    char buffer[1024];
    RequestsHandler* handler;
    int size;

    req_init();  // This is for Windows compatibility, it do nothing on Linux. If you forget it, the program will fail silently.
    
    handler = req_get("https://cdd-cloud.ml", "");  // "" is for no additionals headers
    
    if(handler != NULL)
    {
        while((size = req_read_output_body(handler, buffer, 1024)) > 0)
        {
            buffer[size] = '\0';
            printf("%s", buffer);
        }
        printf("--\n");
        req_close_connection(&handler);
    }
    else
    {
        printf("error code: %d\n", req_get_last_error());
    }

    req_cleanup();  // again, for Windows compatibility
}