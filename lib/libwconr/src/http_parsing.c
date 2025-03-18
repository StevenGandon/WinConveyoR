#include "libwconr_private.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HEADER_END "\r\n\r\n"
#define HEADER_END_LEN 4
#define INITIAL_CHUNK_SIZE 128

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

unsigned char *
read_http_headers(client, total_size)
    struct _http_connection_s *client;
    size_t *total_size;
{
    unsigned char *buffer = NULL;
    unsigned char *temp_buffer = NULL;
    unsigned char *header_end = NULL;
    ssize_t read_size = 0;
    
    *total_size = 0;

    while (1) {
        buffer = (unsigned char *)malloc(INITIAL_CHUNK_SIZE);
        if (!buffer)
            return NULL;
            
        read_size = get_chunk(client, INITIAL_CHUNK_SIZE, buffer);
        
        if (read_size <= 0) {
            free(buffer);
            break;
        }

        temp_buffer = realloc(temp_buffer, *total_size + (size_t)read_size + 1);
        if (!temp_buffer) {
            free(buffer);
            return NULL;
        }

        memcpy(temp_buffer + *total_size, buffer, (size_t)read_size);
        *total_size += (size_t)read_size;
        temp_buffer[*total_size] = '\0';
        free(buffer);

        header_end = (unsigned char *)strstr((const char *)temp_buffer, HEADER_END);
        if (header_end) {
            *total_size = (size_t)(header_end - temp_buffer) + HEADER_END_LEN;
            break;
        }
    }
    
    return temp_buffer;
}

int
parse_status_line(parser, buffer)
    struct _http_response_parser_s *parser;
    unsigned char *buffer;
{
    unsigned char *line_end;
    unsigned char *protocol_end;
    unsigned char *status_start;
    unsigned char *status_msg_start;

    line_end = (unsigned char *)strstr((const char *)buffer, "\r\n");
    if (!line_end)
        return 0;

    protocol_end = (unsigned char *)strchr((const char *)buffer, '/');
    if (!protocol_end)
        return 0;
    
    parser->protocol = (unsigned char *)malloc((size_t)(protocol_end - buffer) + 1);
    if (!parser->protocol)
        return 0;
    
    memcpy(parser->protocol, buffer, (size_t)(protocol_end - buffer));
    parser->protocol[protocol_end - buffer] = '\0';

    if (sscanf((const char *)protocol_end + 1, "%u.%u",
               (unsigned int *)((unsigned char *)&parser->version + 1),
               (unsigned int *)((unsigned char *)&parser->version)) != 2)
        return 0;

    status_start = (unsigned char *)strchr((const char *)protocol_end, ' ');
    if (!status_start)
        return 0;
    status_start++;
    
    parser->status_code = (unsigned short)atoi((const char *)status_start);

    status_msg_start = (unsigned char *)strchr((const char *)status_start, ' ');
    if (!status_msg_start)
        return 0;
    status_msg_start++;
    
    parser->status_message = (unsigned char *)malloc((size_t)(line_end - status_msg_start) + 1);
    if (!parser->status_message)
        return 0;

    memcpy(parser->status_message, status_msg_start, (size_t)(line_end - status_msg_start));
    parser->status_message[line_end - status_msg_start] = '\0';
    
    return 1;
}

void
parse_headers(parser, buffer, total_size)
    struct _http_response_parser_s *parser;
    unsigned char *buffer;
    size_t total_size;
{
    unsigned char *line_start;
    unsigned char *line_end;
    unsigned char *colon_pos;
    size_t current_size;

    line_end = (unsigned char *)strstr((const char *)buffer, "\r\n");
    if (!line_end)
        return;

    current_size = (size_t)(line_end - buffer) + 2;
    
    while (current_size < total_size - HEADER_END_LEN) {
        line_start = buffer + current_size;
        line_end = (unsigned char *)strstr((const char *)line_start, "\r\n");
        
        if (!line_end)
            break;

        colon_pos = (unsigned char *)strchr((const char *)line_start, ':');
        if (colon_pos && colon_pos < line_end) {
            unsigned char *key = (unsigned char *)malloc((size_t)(colon_pos - line_start) + 1);
            unsigned char *value = NULL;
            unsigned char *value_start = colon_pos + 1;

            while (value_start < line_end && *value_start == ' ')
                value_start++;
            
            value = (unsigned char *)malloc((size_t)(line_end - value_start) + 1);
            
            if (key && value) {
                memcpy(key, line_start, (size_t)(colon_pos - line_start));
                key[(size_t)(colon_pos - line_start)] = '\0';
                
                memcpy(value, value_start, (size_t)(line_end - value_start));
                value[(size_t)(line_end - value_start)] = '\0';
                
                set_header(&parser->headers, key, value);
                
                free(key);
                free(value);
            } else {
                if (key) free(key);
                if (value) free(value);
            }
        }
        
        current_size = (size_t)(line_end - buffer) + 2;
    }
}
