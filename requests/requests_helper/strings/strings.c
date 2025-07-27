#include <ctype.h>
#include <string.h>
#include "requests_helper/strings/strings.h"


/*
Copy the string SRC in DEST.
Returns the number of characters copied, '\0' included
If the length of SRC is bigger than DEST_BUFFER_LENGTH, only the first bytes are copied.
In any case, DEST is always terminated by '\0'.
*/
size_t rh_strncpy(char* dest, const char* src, size_t dest_buffer_length)
{
    size_t i;
    for(i = 0; i < dest_buffer_length-1 && src[i] != '\0'; i++)
    {
        dest[i] = src[i];
    }
    dest[i] = '\0';
    return i;
}

/*
Copy the content of SRC into DEST
DEST should be at least the size of SRC
return a pointer to the end of the string, the '\0'
*/
char* rh_strcpy(char* dest, const char* src)
{
    while(*src != '\0')
    {
        *dest = *src;
        dest++;
        src++;
    }
    *dest = '\0';
    return dest;
}

/*
Concatenate the string ADD to the end of DEST.
DEST should be big enough to contains the two strings concatenated
*/
void rh_strcat(char* dest, const char* add)
{
    while(*dest != '\0')
    {
        dest++;
    }
    while(*add != '\0')
    {
        *dest = *add;
        dest++;
        add++;
    }
    *dest = '\0';
}

/*
Modify the string STR with whitespace stripped from the beginning and end of STR
The characters removed are ' ', '\t', '\b', '\n', '\r' and '\v'.
The string returned is the new beginning of the trimmed string.
*/
char* rh_strtrim_inplace(char* str)
{
    size_t i;
    while(strchr(" \t\b\n\r\v", *str))
    {
        str++;
    }
    i = strlen(str) - 1;
    while(strchr(" \t\b\n\r\v", str[i]))
    {
        str[i] = '\0';
        i--;
    }

    return str;
}


/*
Reverse the string, for example "hello" will be transformed to "olleh".
If you have no idea about the len of the string, you can use strlen, like this:
```c
reverse_string(str, strlen(str));
```
*/
void rh_reverse_string(char* str, size_t len_str)
{
    size_t i = 0;
    while(i < len_str)
    {
        char c = str[i];
        str[i] = str[len_str];
        str[len_str] = c;
        i++;
        len_str--;
    }
}


/*
Transform the unsigned integer n to a string and put it in result.
It also returns result this allows the function to be packed in another, like this:
```c
puts(rh_uint64_to_str(result, 57));
```
*/
char* rh_uint64_to_str(char* result, uint64_t n)
{
    size_t i = 0;
    do
    {
        result[i] = (char)(n % 10) + '0';
        i++;
    } while ((n /= 10) > 0);
    result[i] = '\0';
    i--;

    rh_reverse_string(result, i);

    return result;
}


/*
This transform a string to an uint64_t.
If the string is not the representation of an uint64_t, for example if it contains symbols like '-',
the function will returns UINT64_MAX.
*/
uint64_t rh_str_to_uint64(const char* str)
{
    uint64_t result = 0;
    do
    {
        if(!RH_CHAR_IS_DIGIT(*str))
        {
            return UINT64_MAX;
        }
        result *= 10;
        result += (uint64_t)(*str - '0');
        str++;
    } while(*str != '\0');
    return result;
}


/*
compare str1 and str2 with alphanumeric order.
This function is case independent, use rh_strcmp if you want to be case dependent
If str1 < str2, it will returns -1
If str1 > str2 it will returns 1
If str1 == str2, it will returns 0
*/
signed char rh_strcasecmp(const char* str1, const char* str2)
{
    while(*str1 != '\0' && *str2 != '\0' && tolower(*str1) == tolower(*str2))
    {
        str1++;
        str2++;
    }
    if(tolower(*str1) == tolower(*str2))
    {
        return 0;
    }
    if(tolower(*str1) < tolower(*str2))
    {
        return -1;
    }
    return 1;
}

/*
Returns true if the first characters of STR matches with REF.
*/
bool rh_startswith(const char* str, const char* ref)
{
    while(*str != '\0' && *ref != '\0' && *ref == *str)
    {
        str++;
        ref++;
    }
    return *ref == '\0';
}


/*
Returns the position of the string EXPR in STR
If EXPR is not in STR, it returns -1
This function is case unsensitive
*/
int rh_str_search_case_unsensitive(const char* str, const char* expr)
{
    int i = 0;
    int pos = 0;
    while(expr[i] != '\0' && str[i] != '\0')
    {
        if(tolower(str[i]) == tolower(expr[i]))
        {
            i++;
        }
        else
        {
            str += i+1;
            pos += i+1;
            i = 0;
        }
    }
    if(expr[i] != '\0')
    {
        return -1;
    }
    return pos;
}

uint64_t rh_hex_to_uint64(const char* str)
{
    uint64_t result = 0;
    do
    {
        if(!RH_CHAR_IS_HEXDIGIT(*str))
        {
            return UINT64_MAX;
        }
        result *= 16;

        if(*str >= 'A' && *str <= 'F')
            result += (uint64_t)(10 + *str - 'A');
        else if(*str >= 'a' && *str <= 'f')
            result += (uint64_t)(10 + *str - 'a');
        else
            result += (uint64_t)(*str - '0');

        str++;
    } while(*str != '\0');

    return result;
}