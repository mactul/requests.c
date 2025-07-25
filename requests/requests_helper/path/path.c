#include <stdarg.h>
#include "requests_helper/path/path.h"
#include "requests_helper/strings/strings.h"


/*
Remove unwanted ../ and ./ in a path
For example, "abc/fgh/../ijk" => "abc/ijk"
DEST should be at least the size of SRC
*/
void rh_simplify_path(char* dest, const char* src)
{
    int i = 0;
    int j = 0;
    while(src[j] == '.' || src[j] == '/')
    {
        if(src[j] == '/' && src[j+1] == '.' && src[j+2] == '/')
        {
            j += 2;
        }
        else
        {
            dest[i] = src[j];
            i++;
            j++;
        }
    }
    while(src[j] != '\0')
    {
        dest[i] = src[j];

        if(i >= 4 && (dest[i] == '/' || dest[i] == '\0') && dest[i-1] == '.' && dest[i-2] == '.' && dest[i-3] == '/')
        {
            i -= 4;
            while(i >= 0 && dest[i] != '/')
            {
                i--;
            }
        }
        else if(i >= 3 && (dest[i] == '/' || dest[i] == '\0') && dest[i-1] == '.' && dest[i-2] == '/')
        {
            i -= 2;
        }
        i++;
        j++;
    }
    dest[i] = '\0';
}

/*
Concatenate each variadic element in PATH_DEST with '/' between each.
NB_ELEMENTS must contains the number of variadic parameters.
*/
void rh_path_join(char* path_dest, size_t dest_buffer_size, int nb_elements, ...)
{
    va_list args;
    va_start(args, nb_elements);

    for(int i =0; i < nb_elements && dest_buffer_size > 0; i++)
    {
        const char* src = va_arg(args, char*);
        while(*src == '/')
        {
            src++;
        }
        size_t n = rh_strncpy(path_dest, src, dest_buffer_size);
        path_dest += n;
        dest_buffer_size -= n;
        if(i != nb_elements - 1 && n > 0 && *(path_dest-1) != '/')
        {
            *path_dest = '/';
            path_dest++;
            dest_buffer_size--;
        }
    }

    va_end(args);
}