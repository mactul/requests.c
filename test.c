#include <stdio.h>
#include "requests.h"


int main()
{
    char buffer[1024];
    Handler handler;
    int size;
    
    handler = get("https://cdn.discordapp.com/attachments/699999438269186058/1002993123967897770/stats.png", "");  // "" is for no additionals headers
    
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