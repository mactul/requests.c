#include <stdio.h>
#include <unistd.h>
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