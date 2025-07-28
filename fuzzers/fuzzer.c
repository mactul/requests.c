#include <stdint.h>
#include <stdio.h>
#include "requests.h"

void _rh_fuzzer_set_data(const uint8_t* data, size_t size);


int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    _rh_fuzzer_set_data(data, size);

    RequestsHandler* handler = NULL;
    char buffer[1024];

    handler = req_get(NULL, handler, "http://foo.bar/", "");

    if(handler == NULL)
    {
        return 1;
    }

    req_display_headers(handler);

    while(req_read_output_body(handler, buffer, sizeof(buffer)) > 0)
    {
        ;
    }

    req_close_connection(&handler);

    return 0;
}
