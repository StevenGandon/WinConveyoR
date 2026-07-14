#ifndef LIBWCONR_PRIVATE_H_
#define LIBWCONR_PRIVATE_H_

#include <stddef.h>
#include <stdlib.h>
#include "src/platform_types.h"

#ifdef _WIN32
    #include <winsock2.h>

    /**
     * @brief Represents an active HTTP connection (Windows).
     *
     * Holds the remote address, port, and the underlying Winsock socket
     * used to send and receive data.
     */
    struct _http_connection_s {
        unsigned short port;    /**< Remote port number. */
        char *address;          /**< Heap-allocated string holding the remote IP address. */
        SOCKET dest_socket;     /**< Winsock socket handle for this connection. */
    };
#else
    #include <unistd.h>

    /**
     * @brief Represents an active HTTP connection (POSIX).
     *
     * Holds the remote address, port, and the underlying file descriptor
     * used to send and receive data.
     */
    struct _http_connection_s {
        unsigned short port;    /**< Remote port number. */
        char *address;          /**< Heap-allocated string holding the remote IP address. */
        int dest_socket;        /**< POSIX socket file descriptor for this connection. */
    };
#endif

/**
 * @brief A single HTTP header key/value pair.
 */
struct _http_header_s {
    unsigned char *key;     /**< Heap-allocated header field name (e.g. "Content-Type"). */
    unsigned char *value;   /**< Heap-allocated header field value. */
};

/**
 * @brief Parser state for an outgoing HTTP request.
 *
 * Aggregates the connection, request line components, and headers
 * that will be serialised and sent to the remote server.
 */
struct _http_request_parser_s {
    struct _http_connection_s *client;  /**< Non-owning pointer to the underlying connection. */
    unsigned char *method;              /**< Heap-allocated HTTP method string (e.g. "GET"). */
    unsigned char *route;               /**< Heap-allocated request target (e.g. "/index.html"). */
    unsigned char *protocol;            /**< Heap-allocated protocol name (e.g. "HTTP"). */
    unsigned short version;             /**< Encoded protocol version: high byte = major, low byte = minor. */
    struct _http_header_s **headers;    /**< NULL-terminated array of heap-allocated header pairs. */
    size_t header_size;                 /**< Number of entries currently stored in @p headers. */
};

/**
 * @brief Parser state for an incoming HTTP response.
 *
 * Populated by @ref fetch_response after reading and parsing the response
 * status line and headers from the connection.
 */
struct _http_response_parser_s {
    struct _http_connection_s *client;  /**< Non-owning pointer to the underlying connection. */
    unsigned char *protocol;            /**< Heap-allocated protocol name (e.g. "HTTP"). */
    unsigned short version;             /**< Encoded protocol version: high byte = major, low byte = minor. */
    unsigned short status_code;         /**< HTTP status code (e.g. 200, 404). */
    unsigned char *status_message;      /**< Heap-allocated status reason phrase (e.g. "OK"). */
    struct _http_header_s **headers;    /**< NULL-terminated array of heap-allocated header pairs. */
    size_t header_size;                 /**< Number of entries currently stored in @p headers. */
};

/**
 * @brief Releases all resources owned by an HTTP response parser.
 *
 * Frees every header pair, the protocol string, the status message, and
 * the parser struct itself. The associated connection is not closed.
 *
 * @param[in] parser  Parser to destroy. No-op if NULL.
 */
void end_http_response_parser(struct _http_response_parser_s *parser);

/**
 * @brief Releases all resources owned by an HTTP request parser.
 *
 * Frees every header pair, the protocol, method, route strings, and
 * the parser struct itself. The associated connection is not closed.
 *
 * @param[in] parser  Parser to destroy. No-op if NULL.
 */
void end_http_request_parser(struct _http_request_parser_s *parser);

/**
 * @brief Closes and frees an HTTP connection.
 *
 * Closes the socket, frees the address string, and frees the struct.
 * On Windows, decrements the WSA reference counter and calls WSACleanup()
 * when the last connection is closed.
 *
 * @param[in] http_connection  Connection to destroy. No-op if NULL.
 */
void end_http_connection(struct _http_connection_s *http_connection);

/**
 * @brief Creates and connects a new HTTP connection to the given host.
 *
 * Allocates an @ref _http_connection_s, opens a TCP socket, and connects
 * it to @p __s on port @p __p. On Windows, initialises WSA on the first call.
 *
 * @param[in] __s  Remote IP address string (dotted-decimal).
 * @param[in] __p  Remote TCP port number.
 * @return Pointer to the new connection, or NULL on any failure.
 */
struct _http_connection_s *new_http_connection(const char *__s, unsigned short __p);

/**
 * @brief Allocates and initialises a new HTTP response parser.
 *
 * All fields are zeroed or set to NULL. The parser does not own the
 * connection; the caller remains responsible for its lifetime.
 *
 * @param[in] http_connection  Connection this parser will read from.
 * @return Pointer to the new parser, or NULL on allocation failure or
 *         if @p http_connection is NULL.
 */
struct _http_response_parser_s *new_http_response_parser(struct _http_connection_s *http_connection);

/**
 * @brief Allocates and initialises a new HTTP request parser.
 *
 * All fields are zeroed or set to NULL. The parser does not own the
 * connection; the caller remains responsible for its lifetime.
 *
 * @param[in] http_connection  Connection this parser will write to.
 * @return Pointer to the new parser, or NULL on allocation failure or
 *         if @p http_connection is NULL.
 */
struct _http_request_parser_s *new_http_request_parser(struct _http_connection_s *http_connection);

/**
 * @brief Adds or updates a header in a NULL-terminated header array.
 *
 * If a header with the same key already exists (case-sensitive), its value
 * is replaced. Otherwise a new entry is appended and the array is reallocated.
 *
 * @param[in,out] __h  Address of the NULL-terminated header array pointer.
 *                     The array may be reallocated; the pointer is updated.
 * @param[in]     __k  Header field name. The string is duplicated internally.
 * @param[in]     __v  Header field value. The string is duplicated internally.
 */
void set_header(struct _http_header_s ***__h, const unsigned char *__k, const unsigned char *__v);

/**
 * @brief Looks up a header value by key in a NULL-terminated header array.
 *
 * The comparison is case-sensitive.
 *
 * @param[in] __h  NULL-terminated array of header pairs.
 * @param[in] __k  Header field name to search for.
 * @return Pointer to the header value string (owned by the array), or NULL
 *         if not found or if @p __h is NULL.
 */
const unsigned char *get_header(const struct _http_header_s * const *__h, const unsigned char *__k);

/**
 * @brief Reads raw bytes from a connection into a caller-supplied buffer.
 *
 * Wraps @c read() (POSIX) or @c recv() (Windows) for a single non-blocking
 * chunk of up to @p __s bytes.
 *
 * @param[in]  __c  Connection to read from.
 * @param[in]  __s  Maximum number of bytes to read.
 * @param[out] __d  Destination buffer of at least @p __s bytes.
 * @return Number of bytes read on success, -1 on error or if any argument
 *         is NULL / zero.
 */
ssize_t get_chunk(struct _http_connection_s *__c, size_t __s, unsigned char *__d);

/**
 * @brief Sends a fully built HTTP request over the connection.
 *
 * Serialises the request line (method, route, protocol/version), all
 * stored headers, and the mandatory blank line terminator, then writes
 * the result to the socket.
 *
 * @param[in] __r           Request parser containing the request to send.
 * @param[in] http_address  Fallback Host header value when no Host header
 *                          has been set explicitly.
 */
void request_ressource(struct _http_request_parser_s *__r, const unsigned char *http_address);

/**
 * @brief Reads and parses the HTTP response headers from the connection.
 *
 * Populates @p __r with the status code, reason phrase, protocol version,
 * and all response headers. The response body is not consumed.
 *
 * @param[in,out] __r  Response parser to fill. Must have a valid connection.
 */
void fetch_response(struct _http_response_parser_s *__r);

/**
 * @brief Reads raw response data from the connection until the header
 *        terminator (@c \\r\\n\\r\\n) is found.
 *
 * Accumulates chunks until the end-of-headers sequence is detected.
 * The returned buffer is heap-allocated and null-terminated; @p total_size
 * is set to the number of bytes up to and including the terminator.
 *
 * @param[in]  client      Connection to read from.
 * @param[out] total_size  Set to the length of the returned buffer on success.
 * @return Heap-allocated buffer containing the raw header block, or NULL on
 *         failure. The caller is responsible for freeing it.
 */
unsigned char *read_http_headers(struct _http_connection_s *client, size_t *total_size);

/**
 * @brief Parses the HTTP status line from a raw header buffer.
 *
 * Extracts the protocol name, version, status code, and reason phrase into
 * @p parser. The buffer must start at the beginning of the status line.
 *
 * @param[in,out] parser  Response parser to fill.
 * @param[in]     buffer  Raw header buffer starting with the status line.
 * @return 1 on success, 0 on parse failure or allocation error.
 */
int parse_status_line(struct _http_response_parser_s *parser, unsigned char *buffer);

/**
 * @brief Parses all HTTP response headers from a raw header buffer.
 *
 * Skips the status line and iterates over each @c Key: Value\\r\\n line,
 * storing every pair via @ref set_header into @p parser.
 *
 * @param[in,out] parser      Response parser to fill.
 * @param[in]     buffer      Raw header buffer (status line + headers).
 * @param[in]     total_size  Total size of @p buffer in bytes, including the
 *                            final @c \\r\\n\\r\\n terminator.
 */
void parse_headers(struct _http_response_parser_s *parser, unsigned char *buffer, size_t total_size);

#endif /* !LIBWCONR_PRIVATE_H_ */
