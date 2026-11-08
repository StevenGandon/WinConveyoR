#include <criterion/criterion.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#define TEST_IO_TIMEOUT_SEC 1
#define TEST_TIMEOUT_MARGIN_SEC 3
#define TEST_LOOPBACK "127.0.0.1"
#define TEST_LISTEN_BACKLOG 1
#define WCR_IO_TIMEOUT_SEC TEST_IO_TIMEOUT_SEC

#include "wcr_client.h"

#define TEST_SEND_CHUNK_SIZE 3
#define TEST_PAYLOAD "partial write"

static unsigned char sent_bytes[WCR_HEADER_SIZE + sizeof(TEST_PAYLOAD) - 1];
static size_t sent_size;
static size_t send_calls;

#define send mock_send
#define wcr_send wcr_send_with_mock

#include "../../../lib/libwconr/src/wcr_client.c"

#undef wcr_send
#undef send

ssize_t mock_send(int sockfd, const void *buffer, size_t length, int flags)
{
    size_t chunk_size = length < TEST_SEND_CHUNK_SIZE ? length : TEST_SEND_CHUNK_SIZE;

    (void)sockfd;
    (void)flags;

    memcpy(sent_bytes + sent_size, buffer, chunk_size);

    sent_size += chunk_size;

    send_calls++;

    return (ssize_t)chunk_size;
}

Test(wcr_send, retries_until_header_and_payload_are_fully_sent)

{
    const unsigned char expected_header[WCR_HEADER_SIZE] = {
        0xff, 0xc4, 0x07, 0xec,
        0x12, 0x34,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0d
    };
    wcr_conn connection = {.sockfd = 42};
    wcr_msg *message = wcr_msg_new(TEST_PAYLOAD, 0x1234);

    sent_size = 0;
    send_calls = 0;

    cr_assert_not_null(message);
    cr_assert_eq(wcr_send_with_mock(&connection, message), 0);
    cr_assert_gt(send_calls, 2);
    cr_assert_eq(sent_size, sizeof(sent_bytes));
    cr_assert_arr_eq(sent_bytes, expected_header, WCR_HEADER_SIZE);
    cr_assert_arr_eq(
        sent_bytes + WCR_HEADER_SIZE,
        TEST_PAYLOAD,
        sizeof(TEST_PAYLOAD) - 1
    );

    wcr_msg_free(message);
}

static int open_listener(int *port)
{
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    int fd = socket(AF_INET, SOCK_STREAM, 0);

    if (fd < 0)
        return -1;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = 0;
    inet_pton(AF_INET, TEST_LOOPBACK, &addr.sin_addr);
    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0
        || listen(fd, TEST_LISTEN_BACKLOG) < 0
        || getsockname(fd, (struct sockaddr *)&addr, &len) < 0) {
        close(fd);
        return -1;
    }
    *port = ntohs(addr.sin_port);
    return fd;
}

static void free_conn(wcr_conn *conn)
{
    close(conn->sockfd);
    free(conn);
}

static double elapsed_sec(const struct timespec *start)
{
    struct timespec now;

    clock_gettime(CLOCK_MONOTONIC, &now);
    return (double)(now.tv_sec - start->tv_sec)
        + (double)(now.tv_nsec - start->tv_nsec) / 1e9;
}

Test(wcr_open, returns_null_on_null_host)
{
    cr_assert_null(wcr_open(NULL, WCR_DEFAULT_PORT));
}

Test(wcr_open, fails_fast_on_closed_port)
{
    int port;
    int listener = open_listener(&port);
    struct timespec start;

    cr_assert_geq(listener, 0);
    close(listener);

    clock_gettime(CLOCK_MONOTONIC, &start);
    cr_assert_null(wcr_open(TEST_LOOPBACK, port));
    cr_assert_lt(elapsed_sec(&start), TEST_IO_TIMEOUT_SEC);
}

Test(wcr_open, restores_blocking_mode_and_sets_io_timeouts)
{
    int port;
    int listener = open_listener(&port);
    wcr_conn *conn;
    struct timeval rcv;
    struct timeval snd;
    socklen_t len = sizeof(struct timeval);

    cr_assert_geq(listener, 0);
    conn = wcr_open("localhost", port);
    cr_assert_not_null(conn);

    cr_assert_eq(fcntl(conn->sockfd, F_GETFL, 0) & O_NONBLOCK, 0);
    cr_assert_eq(getsockopt(conn->sockfd, SOL_SOCKET, SO_RCVTIMEO, &rcv, &len), 0);
    len = sizeof(struct timeval);
    cr_assert_eq(getsockopt(conn->sockfd, SOL_SOCKET, SO_SNDTIMEO, &snd, &len), 0);
    cr_assert_eq(rcv.tv_sec, TEST_IO_TIMEOUT_SEC);
    cr_assert_eq(snd.tv_sec, TEST_IO_TIMEOUT_SEC);

    free_conn(conn);
    close(listener);
}

Test(wcr_recv, times_out_on_silent_mirror)
{
    int port;
    int listener = open_listener(&port);
    wcr_conn *conn;
    struct timespec start;
    double elapsed;

    cr_assert_geq(listener, 0);
    conn = wcr_open(TEST_LOOPBACK, port);
    cr_assert_not_null(conn);

    clock_gettime(CLOCK_MONOTONIC, &start);
    cr_assert_null(wcr_recv(conn));
    elapsed = elapsed_sec(&start);
    cr_assert_geq(elapsed, TEST_IO_TIMEOUT_SEC * 0.9);
    cr_assert_lt(elapsed, TEST_IO_TIMEOUT_SEC + TEST_TIMEOUT_MARGIN_SEC);

    free_conn(conn);
    close(listener);
}

Test(wait_connected, reports_error_on_refused_connection)
{
    int port;
    int listener = open_listener(&port);
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    int flags;

    cr_assert_geq(listener, 0);
    cr_assert_geq(fd, 0);
    close(listener);

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((unsigned short)port);
    inet_pton(AF_INET, TEST_LOOPBACK, &addr.sin_addr);

    flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0 && errno == EINPROGRESS)
        cr_assert_eq(wait_connected(fd), -1);
    close(fd);
}