#include "libwconr_private.h"

#include <stdlib.h>
#include <winsock2.h>
#include <unistd.h>
#include <string.h>

volatile unsigned char __WIN_INITED = 0;
volatile unsigned char __WIN_INITED_REF = 0;

void
end_http_connection(http_connection)
    struct _http_connection_s *http_connection;
{
    if (!http_connection)
        return;
    if (http_connection->dest_socket != INVALID_SOCKET) {
        (void)closesocket(http_connection->dest_socket);
        http_connection->dest_socket = INVALID_SOCKET;
    }
    if (http_connection->address) {
        (void)free(http_connection->address);
        http_connection->address = NULL;
    }
    (void)free(http_connection);

    if (__WIN_INITED) {
        if (__WIN_INITED_REF > 0) {
            --__WIN_INITED_REF;

            if (__WIN_INITED_REF == 0) {
                WSACleanup();
                __WIN_INITED = 0;
            }
        }
    }
}

struct _http_connection_s *
new_http_connection(ip, port)
    const char *ip;
    unsigned short port;
{
    struct _http_connection_s *connection = (struct _http_connection_s *)malloc(sizeof(struct _http_connection_s));
    struct sockaddr_in dest_addr;

    if (!connection)
        return (NULL);

    if (!ip) {
        (void)free(connection);
        return (NULL);
    }

    connection->port = port;
    connection->address = strdup(ip);

    if (!__WIN_INITED) {
        WSADATA wsaData = {0};

        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
            return (NULL);
        __WIN_INITED = 1;
    }
    ++__WIN_INITED_REF;

    if (!connection->address) {
        (void)end_http_connection(connection);
        return (NULL);
    }

    connection->dest_socket = socket(AF_INET, SOCK_STREAM, 0);

    if (connection->dest_socket == INVALID_SOCKET) {
        (void)end_http_connection(connection);
        return (NULL);
    }

    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(connection->port);
    dest_addr.sin_addr.s_addr = inet_addr(connection->address);

    if ((connect(connection->dest_socket, (struct sockaddr *)&dest_addr, sizeof(dest_addr))) == SOCKET_ERROR) {
        (void)end_http_connection(connection);
        return (NULL);
   }
    return (connection);
}
