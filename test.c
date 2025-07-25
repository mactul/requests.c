#include <stdio.h>
#include "requests.h"

int main()
{
    char buffer[1025];
    RequestsHandler* handler = NULL;
    size_t size;
    char* url_array[] = {
        "https://raw.githubusercontent.com/mactul/requests.c/master/README.md",
        "https://raw.githubusercontent.com/mactul/requests.c/master/requests/requests.h",
        "https://raw.githubusercontent.com/mactul/requests.c/master/makefile.py",
        "https://example.com/test"
    };

    req_init();  // This is for Windows compatibility, it do nothing on Linux. If you forget it, the program will fail silently.

    for(size_t i = 0; i < sizeof(url_array) / sizeof(char*); i++)
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
            printf("error\n");
        }
    }


    req_close_connection(&handler);  // note that the close is only done for the last connection

    req_destroy();  // again, for Windows compatibility
}