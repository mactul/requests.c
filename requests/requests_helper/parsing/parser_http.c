#include <ctype.h>
#include "requests_helper/parsing/parsing.h"
#include "requests_helper/strings/strings.h"

/*
This parse data encoded like `key1=value1&key2=value2&...`
It returns a ParserTree which can be read by `rh_ptree...` functions.
*/
rh_ParserTree* rh_parse_urlencoded_form(const char* data)
{
    rh_ParserTree* tree = rh_ptree_init();
    while(*data != '\0')
    {
        int i = 0;
        while(data[i] != '\0' && data[i] != '=')
        {
            i++;
        }
        if(data[i] == '\0')
        {
            rh_ptree_abort(tree);
            rh_ptree_free(&tree);
            return NULL;
        }
        i++;

        rh_ptree_update_key(tree, data, i);
        data += i;
        i = 0;
        while(data[i] != '\0' && data[i] != '&')
        {
            i++;
        }
        rh_ptree_update_value(tree, data, i+1);
        if(data[i] != '\0')
        {
            i++;
        }
        data += i;

        rh_ptree_push(tree, rh_urldecode_inplace);
    }
    return tree;
}

bool rh_parse_url(const char* url, rh_UrlSplitted* url_splitted)
{
    int i = 0;

    if(rh_startswith(url, "https://"))
    {
        url_splitted->secured = true;
        url_splitted->port = 443;
        url += 8;
    }
    else if(rh_startswith(url, "http://"))
    {
        url_splitted->secured = false;
        url_splitted->port = 80;
        url += 7;
    }
    else
    {
        return false;
    }


    // get the host from url
    i = 0;
    while(i < rh_MAX_CHAR_ON_HOST && *url != '\0' && *url != '/' && *url != '?' && *url != '#' && *url != ':')
    {
        url_splitted->host[i] = *url;
        i++;
        url++;
    }
    url_splitted->host[i] = '\0';

    // get the port if it is specified
    if(*url == ':')
    {
        char port_str[8];
        uint64_t temp_port;
        i = 0;
        url++;
        while(i < 7 && rh_CHAR_IS_DIGIT(*url))
        {
            port_str[i] = *url;
            i++;
            url++;
        }
        port_str[i] = '\0';
        temp_port = rh_str_to_uint64(port_str);
        if(temp_port > (uint16_t)(-1))
        {
            return false;
        }
        if(temp_port != 0)
        {
            url_splitted->port = (uint16_t)temp_port;
        }
    }

    if(*url != '\0' && *url != '/' && *url != '?' && *url != '#' && *url != ':')
    {
        return false;
    }


    // get the relative url from url
    i = 0;
    while(i < rh_MAX_URI_LENGTH && *url != '\0' && *url != '#')
    {
        url_splitted->uri[i] = *url;
        i++;
        url++;
    }
    if(i == 0)
    {
        // There is no relative url
        url_splitted->uri[i] = '/';
        i++;
    }
    url_splitted->uri[i] = '\0';

    if(*url != '\0' && *url != '#')
    {
        return false;
    }

    return true;
}


/*
Decode an urlencoded string in SRC to the buffer DEST.
DEST should be at least the size of SRC (urlencoded string are equals or bigger than decoded ones).
*/
void rh_urldecode(char *dst, const char *src)
{
    char a, b;
    while (*src != '\0')
    {
        if ((*src == '%') && ((a = src[1]) && (b = src[2])) && (isxdigit(a) && isxdigit(b)))
        {
            if (a >= 'a')
                a -= 'a' - 'A';
            if (a >= 'A')
                a -= ('A' - 10);
            else
                a -= '0';
            if (b >= 'a')
                b -= 'a' - 'A';
            if (b >= 'A')
                b -= ('A' - 10);
            else
                b -= '0';
            *dst++ = 16 * a + b;
            src += 3;
        }
        else if (*src == '+')
        {
            *dst++ = ' ';
            src++;
        }
        else
        {
            *dst++ = *src++;
        }
    }
    *dst++ = '\0';
}
