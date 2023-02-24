#include "utils.h"
#include <string.h>
#include <ctype.h>

#define is_ascii(c) (('0' <= c && c <= '9') || ('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z') || char_in_str(";,/?:@&=+$-_.!~*'()#%", c))

#define MAX_URL_SIZE 1024

void int_to_string(int n, char s[])
{
    int i, sign;
    void reverse_string(char s[]);

    if ((sign = n) < 0)  /* record sign */
        n = -n;          /* make n positive */
    i = 0;
    do
    {
        s[i++] = n % 10 + '0';   /* get next digit */
    } while ((n /= 10) > 0);     /* delete it */
    if (sign < 0)
        s[i++] = '-';
    s[i] = '\0';
    reverse_string(s);
}

void reverse_string(char s[])
{
    int i, j;
    char c;

    for (i = 0, j = strlen(s)-1; i<j; i++, j--)
    {
        c = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}

void bytescpy(char* dest, const char* src, int n)
{
    int i = 0;
    while(i < n)
    {
        dest[i] = src[i];
        i++;
    }
}

/* return the position of the first occurence's end
this function is non case-sensitive */
int stristr(const char* string, const char* exp)
{
    int string_counter = 0;
    int exp_counter = 0;
    while(string[string_counter] != '\0')
    {
        if(tolower(string[string_counter]) == tolower(exp[0]))
        {
            while(exp[exp_counter] != '\0' && string[string_counter] != '\0' && tolower(string[string_counter]) == tolower(exp[exp_counter]))
            {
                exp_counter++;
                string_counter++;
            }
            if(exp[exp_counter] == '\0')
            {
                return string_counter;
            }
            exp_counter = 0;
        }
        string_counter++;
    }
    return -1;
}

char starts_with(const char* str, const char* ref)
{
    int i = 0;
    while(str[i] != '\0' && ref[i] != '\0' && str[i] == ref[i])
    {
        i++;
    }
    return ref[i] == '\0';
}

char char_in_str(char* str, char c)
{
    while(*str != '\0')
    {
        if(*str == c)
            return 1;
        
        str++;
    }
    return 0;
}

char* strtrim(char* str)
{
    while(!is_ascii(*str))
    {
        str++;
    }
    int i = strlen(str) - 1;
    while(!is_ascii(str[i]))
    {
        str[i] = '\0';
        i--;
    }

    return str;
}

char starts_with_case_unsensitive(char* str, const char* ref)
{
    int i = 0;
    while(str[i] != '\0' && ref[i] != '\0' && tolower(str[i]) == tolower(ref[i]))
    {
        i++;
    }
    return ref[i] == '\0';
}

void retrieve_absolute_url(char* url, const char* reference_url)
{
    if(!starts_with_case_unsensitive(url, "https://") && !starts_with_case_unsensitive(url, "http://"))
    {
        char relative_url[MAX_URL_SIZE];
        int k = 0;
        int n_backward = 0;
        while(url[k] == '/' || url[k+1] == '/' || url[k+2] == '/')
        {
            if(url[k] == '.' && url[k+1] == '.' && url[k+2] == '/')
            {
                n_backward++;
                k += 3;
            }
            else
            {
                k++;
            }
        }
        strcpy(relative_url, &(url[k]));

        int end_of_path = 0;
        int n_slash = 0;
        while(reference_url[end_of_path] != '\0' && reference_url[end_of_path] != '?' && reference_url[end_of_path] != '#')
        {
            if(reference_url[end_of_path] == '/')
                n_slash++;
            end_of_path++;
        }
        k = end_of_path;
        if(n_slash > 2)
        {
            char is_file = 0;
            while(reference_url[k] != '/')
            {
                if(reference_url[k] == '.')
                    is_file = 1;
                k--;
            }
            if(is_file)
                end_of_path = k;
        }
        
        end_of_path--;

        if(reference_url[end_of_path] == '/')
            end_of_path--;

        while(n_backward > 0)
        {
            if(reference_url[end_of_path] == '/')
                n_backward--;
            end_of_path--;
        }
        strncpy(url, reference_url, end_of_path+1);
        url[end_of_path+1] = '/';
        url[end_of_path+2] = '\0';

        strcat(url, relative_url);
    }
}