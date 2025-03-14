#include "libwconr_private.h"

#include <stdlib.h>

void
end_http_response_parser(http_parser)
    struct _http_response_parser_s *http_parser;
{
    size_t i = 0;

    if (!http_parser)
        return;
    if (http_parser->headers) {
        for (; i < http_parser->header_size; ++i) {
            if (http_parser->headers[i]) {
                if (http_parser->headers[i]->key)
                    (void)free(http_parser->headers[i]->key);
                if (http_parser->headers[i]->value)
                    (void)free(http_parser->headers[i]->value);
                (void)free(http_parser->headers[i]);
                http_parser->headers[i] = NULL;
            }
        }
        http_parser->header_size = 0;
        (void)free(http_parser->headers);
        http_parser->headers = NULL;
    }
    if (http_parser->protocol) {
        (void)free(http_parser->protocol);
        http_parser->protocol = NULL;
    }
    if (http_parser->client)
        http_parser->client = NULL;
    if (http_parser->status_message) {
        (void)free(http_parser->status_message);
        http_parser->status_message = NULL;
    }
    (void)free(http_parser);
}

void
end_http_request_parser(http_parser)
    struct _http_request_parser_s *http_parser;
{
    size_t i = 0;

    if (!http_parser)
        return;
    if (http_parser->headers) {
        for (; i < http_parser->header_size; ++i) {
            if (http_parser->headers[i]) {
                (void)free(http_parser->headers[i]);
                http_parser->headers[i] = NULL;
            }
        }
        http_parser->header_size = 0;
        (void)free(http_parser->headers);
        http_parser->headers = NULL;
    }
    if (http_parser->protocol) {
        (void)free(http_parser->protocol);
        http_parser->protocol = NULL;
    }
    if (http_parser->client)
        http_parser->client = NULL;
    if (http_parser->method) {
        (void)free(http_parser->method);
        http_parser->method = NULL;
    }
    if (http_parser->route) {
        (void)free(http_parser->route);
        http_parser->route = NULL;
    }
    (void)free(http_parser);
}

struct _http_request_parser_s *
new_http_request_parser(http_connection)
    struct _http_connection_s *http_connection;
{
    struct _http_request_parser_s *parser = (struct _http_request_parser_s *)malloc(sizeof(struct _http_request_parser_s));

    if (!parser || !http_connection)
        return (NULL);
    parser->client = http_connection;
    parser->headers = NULL;
    parser->header_size = 0;
    parser->protocol = NULL;
    parser->route = NULL;
    parser->method = NULL;

    *(((unsigned char *)(&parser->version))) = 0;
    *(((unsigned char *)(&parser->version)) + 1) = 0;
    return (parser);
}

struct _http_response_parser_s *
new_http_response_parser(http_connection)
    struct _http_connection_s *http_connection;
{
    struct _http_response_parser_s *parser = (struct _http_response_parser_s *)malloc(sizeof(struct _http_response_parser_s));

    if (!parser || !http_connection)
        return (NULL);
    parser->client = http_connection;
    parser->headers = NULL;
    parser->header_size = 0;
    parser->protocol = NULL;
    parser->status_code = 0;
    parser->status_message = NULL;

    *(((unsigned char *)(&parser->version))) = 0;
    *(((unsigned char *)(&parser->version)) + 1) = 0;
    return (parser);
}
