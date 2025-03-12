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
    void end_http_connection(struct _http_connection_s *);
    struct _http_connection_s *new_http_connection(const char *__s, int);
    struct _http_response_parser_s *new_http_response_parser(struct _http_connection_s *);

#endif /* !LIBWCONR_PRIVATE_H_ */
