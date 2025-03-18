#include "libwconr_private.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifdef __WIN32
    #include <winsock2.h>
    #define send_data(socket, buffer, size) send(socket, (const char *)(buffer), (int)(size), 0)
#else
    #include <unistd.h>
    #define send_data(socket, buffer, size) write(socket, buffer, size)
#endif

static size_t
calculate_request_size(struct _http_request_parser_s *request_parser)
{
    size_t buffer_size = 0;
    size_t i = 0;

    buffer_size = strlen((const char *)request_parser->method) + 1;
    buffer_size += strlen((const char *)request_parser->route) + 1;
    buffer_size += strlen((const char *)request_parser->protocol) + 1;
    buffer_size += 5;

    for (i = 0; i < request_parser->header_size; i++) {
        if (request_parser->headers[i]) {
            buffer_size += strlen((const char *)request_parser->headers[i]->key);
            buffer_size += 2;
            buffer_size += strlen((const char *)request_parser->headers[i]->value);
            buffer_size += 2;
        }
    }
    
    buffer_size += 2;

    return buffer_size;
}

static char *
build_request(struct _http_request_parser_s *request_parser, size_t buffer_size)
{
    char *request_buffer = NULL;
    size_t current_size = 0;
    size_t i = 0;

    request_buffer = (char *)malloc(buffer_size + 1);
    if (!request_buffer)
        return NULL;

    current_size = (size_t)sprintf(request_buffer, "%s %s %s/%d.%d\r\n",
        request_parser->method,
        request_parser->route,
        request_parser->protocol,
        (request_parser->version >> 8) & 0xFF,
        request_parser->version & 0xFF);

    for (i = 0; i < request_parser->header_size; i++) {
        if (request_parser->headers[i]) {
            current_size += (size_t)sprintf(request_buffer + current_size, "%s: %s\r\n",
                request_parser->headers[i]->key,
                request_parser->headers[i]->value);
        }
    }

    current_size += (size_t)sprintf(request_buffer + current_size, "\r\n");

    return request_buffer;
}

void
request_ressource(struct _http_request_parser_s *request_parser)
{
    char *request_buffer = NULL;
    size_t buffer_size = 0;
    size_t current_size = 0;

    if (!request_parser || !request_parser->client)
        return;

    buffer_size = calculate_request_size(request_parser);

    request_buffer = build_request(request_parser, buffer_size);
    if (!request_buffer)
        return;

    current_size = strlen(request_buffer);

    send_data(request_parser->client->dest_socket, request_buffer, current_size);

    free(request_buffer);
}