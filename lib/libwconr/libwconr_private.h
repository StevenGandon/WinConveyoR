#ifndef LIBWCONR_PRIVATE_H_
    #define LIBWCONR_PRIVATE_H_

    #include <stddef.h>

    #ifdef __WIN32
        #include <winsock2.h>

        struct _http_connection_s {
            unsigned short port;
            char *address;

            SOCKET dest_socket;
        };
    #else
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
    struct _http_connection_s *new_http_connection(const char *__s, int);
    struct _http_response_parser_s *new_http_response_parser(struct _http_connection_s *);
    struct _http_request_parser_s *new_http_request_parser(struct _http_connection_s *);
    void set_header(struct _http_header_s ***__h, const unsigned char *__k, const unsigned char *__v);
    unsigned char *get_header(const struct _http_header_s **__h, const unsigned char *__k);
    void get_chunk(struct _http_connection_s *__c, size_t __s, unsigned char *__d);
    void request_ressource(struct _http_request_parser_s *__r);
    void fetch_response(struct _http_response_parser_s *__r);

#endif /* !LIBWCONR_PRIVATE_H_ */
