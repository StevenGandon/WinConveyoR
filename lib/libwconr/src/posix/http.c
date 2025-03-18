#include "libwconr_private.h"

#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

ssize_t
get_chunk(connection, size, buffer)
    struct _http_connection_s *connection;
    size_t size;
    unsigned char *buffer;
{
    if (!connection)
        return (-1);
    if (!buffer)
        return (-1);
    if (!size)
        return (-1);
    return (read(connection->dest_socket, buffer, size));
}

void
end_http_connection(http_connection)
    struct _http_connection_s *http_connection;
{
    if (!http_connection)
        return;
    if (http_connection->dest_socket >= 0) {
        (void)close(http_connection->dest_socket);
        http_connection->dest_socket = -1;
    }
    if (http_connection->address) {
        (void)free(http_connection->address);
        http_connection->address = NULL;
    }
    (void)free(http_connection);
}

struct _http_connection_s *
new_http_connection(ip, port)
    const char *ip;
    unsigned short port;
{
    struct _http_connection_s *connection = (struct _http_connection_s *)malloc(sizeof(struct _http_connection_s));
    struct sockaddr_in dest_addr;

    if (!connection) {
        return (NULL);
    }

    if (!ip) {
        (void)free(connection);
        return (NULL);
    }

    connection->port = port;
    connection->address = strdup(ip);

    if (!connection->address) {
        (void)end_http_connection(connection);
        return (NULL);
    }

    connection->dest_socket = socket(AF_INET, SOCK_STREAM, 0);

    if (connection->dest_socket < 0) {
        (void)end_http_connection(connection);
        return (NULL);
    }

    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(connection->port);

    if (inet_pton(AF_INET, connection->address, &dest_addr.sin_addr) <= 0) {
        (void)end_http_connection(connection);
        return (NULL);
    }
    if ((connect(connection->dest_socket, (struct sockaddr *)&dest_addr, sizeof(dest_addr))) < 0) {
        (void)end_http_connection(connection);
        return (NULL);
    }

    return (connection);
}
