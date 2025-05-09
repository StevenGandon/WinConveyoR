#ifndef LIBWCONR_PRIVATE_H_
    #define LIBWCONR_PRIVATE_H_

    #include <stddef.h>
    #include <stdlib.h>
    #include "src/platform_types.h"

    #ifdef _WIN32
        #include <winsock2.h>

            struct _http_connection_s {
            unsigned short port;
            char *address;
            SOCKET dest_socket;
        };
    #else
        #include <unistd.h>

            struct _http_connection_s {
            unsigned short port;
            char *address;
            int dest_socket;
        };
    #endif

    struct _http_header_s {
        unsigned char *key;
        unsigned char *value;
    };

    struct _http_request_parser_s {
        struct _http_connection_s *client;

        unsigned char *method;

        unsigned char *route;

        unsigned char *protocol;
        unsigned short version;

        struct _http_header_s **headers;
        size_t header_size;
    };

    struct _http_response_parser_s {
        struct _http_connection_s *client;

        unsigned char *protocol;
        unsigned short version;

        unsigned short status_code;
        unsigned char *status_message;

        struct _http_header_s **headers;
        size_t header_size;
    };

    void end_http_response_parser(struct _http_response_parser_s *);
    void end_http_request_parser(struct _http_request_parser_s *);
    void end_http_connection(struct _http_connection_s *);
    struct _http_connection_s *new_http_connection(const char *__s, unsigned short __p);
    struct _http_response_parser_s *new_http_response_parser(struct _http_connection_s *);
    struct _http_request_parser_s *new_http_request_parser(struct _http_connection_s *);
    void set_header(struct _http_header_s ***__h, const unsigned char *__k, const unsigned char *__v);
    const unsigned char *get_header(const struct _http_header_s * const *__h, const unsigned char *__k);
    ssize_t get_chunk(struct _http_connection_s *__c, size_t __s, unsigned char *__d);
    void request_ressource(struct _http_request_parser_s *__r, const unsigned char *http_address);
    void fetch_response(struct _http_response_parser_s *__r);
    unsigned char *read_http_headers(struct _http_connection_s *client, size_t *total_size);
    int parse_status_line(struct _http_response_parser_s *parser, unsigned char *buffer);
    void parse_headers(struct _http_response_parser_s *parser, unsigned char *buffer, size_t total_size);


#endif /* !LIBWCONR_PRIVATE_H_ */
