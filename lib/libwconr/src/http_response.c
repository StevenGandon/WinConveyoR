#include "libwconr_private.h"

#include <stdlib.h>
#include <string.h>

#define HEADER_END "\r\n\r\n"
#define HEADER_END_LEN 4
#define INITIAL_CHUNK_SIZE 128

void
fetch_response(struct _http_response_parser_s *response_parser)
{
    unsigned char *buffer = NULL;
    size_t total_size = 0;
    
    if (!response_parser || !response_parser->client)
        return;

    buffer = read_http_headers(response_parser->client, &total_size);
    
    if (!buffer || total_size == 0)
        return;

    if (!parse_status_line(response_parser, buffer)) {
        if (buffer) free(buffer);
        return;
    }

    parse_headers(response_parser, buffer, total_size);
}