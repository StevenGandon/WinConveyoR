#include <criterion/criterion.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define TEST_IO_TIMEOUT_SEC 1
#define TEST_TIMEOUT_MARGIN_SEC 3
#define TEST_LOOPBACK "127.0.0.1"
#define TEST_LISTEN_BACKLOG 1
#define TEST_FIRST_CALL 1
#define TEST_SECOND_CALL 2
#define TEST_THIRD_CALL 3
#define WCR_IO_TIMEOUT_SEC TEST_IO_TIMEOUT_SEC

#include "wcr_client.h"

#define TEST_SEND_CHUNK_SIZE 3
#define TEST_PAYLOAD "partial write"

static unsigned char sent_bytes[WCR_HEADER_SIZE + sizeof(TEST_PAYLOAD) - 1];
static size_t sent_size;
static size_t send_calls;

typedef struct {
    int calls;
    int fail_at;
    int fail_ret;
    int fail_errno;
} fault_t;

static fault_t setsockopt_fault;
static fault_t getsockopt_fault;
static fault_t fcntl_fault;
static fault_t select_fault;
static fault_t connect_fault;

ssize_t mock_send(int sockfd, const void *buffer, size_t length, int flags);
int mock_setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen);
int mock_getsockopt(int sockfd, int level, int optname, void *optval, socklen_t *optlen);
int mock_fcntl(int fd, int cmd, ...);
int mock_select(int nfds, fd_set *rfds, fd_set *wfds, fd_set *efds, struct timeval *tv);
int mock_connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);

#define send mock_send
#define setsockopt mock_setsockopt
#define getsockopt mock_getsockopt
#define fcntl mock_fcntl
#define select mock_select
#define connect mock_connect
#define wcr_send wcr_send_with_mock

#include "../../../lib/libwconr/src/wcr_client.c"

#undef wcr_send
#undef connect
#undef select
#undef fcntl
#undef getsockopt
#undef setsockopt
#undef send

static void inject_fault(fault_t *fault, int fail_at, int fail_ret, int fail_errno)
{
    fault->calls = 0;
    fault->fail_at = fail_at;
    fault->fail_ret = fail_ret;
    fault->fail_errno = fail_errno;
}

static int should_fail(fault_t *fault)
{
    fault->calls++;
    if (fault->fail_at == 0 || fault->calls != fault->fail_at)
        return 0;
    errno = fault->fail_errno;
    return 1;
}

int mock_setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen)
{
    if (should_fail(&setsockopt_fault))
        return setsockopt_fault.fail_ret;
    return setsockopt(sockfd, level, optname, optval, optlen);
}

int mock_getsockopt(int sockfd, int level, int optname, void *optval, socklen_t *optlen)
{
    if (should_fail(&getsockopt_fault))
        return getsockopt_fault.fail_ret;
    return getsockopt(sockfd, level, optname, optval, optlen);
}

int mock_fcntl(int fd, int cmd, ...)
{
    va_list ap;
    int arg;

    va_start(ap, cmd);
    arg = va_arg(ap, int);
    va_end(ap);
    if (should_fail(&fcntl_fault))
        return fcntl_fault.fail_ret;
    return fcntl(fd, cmd, arg);
}

int mock_select(int nfds, fd_set *rfds, fd_set *wfds, fd_set *efds, struct timeval *tv)
{
    if (should_fail(&select_fault))
        return select_fault.fail_ret;
    return select(nfds, rfds, wfds, efds, tv);
}

int mock_connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
    if (should_fail(&connect_fault))
        return connect_fault.fail_ret;
    return connect(sockfd, addr, addrlen);
}

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

static int start_connect(int port)
{
    struct sockaddr_in addr;
    int fd = socket(AF_INET, SOCK_STREAM, 0);

    if (fd < 0)
        return -1;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((unsigned short)port);
    inet_pton(AF_INET, TEST_LOOPBACK, &addr.sin_addr);
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
    connect(fd, (struct sockaddr *)&addr, sizeof(addr));
    return fd;
}

static struct sockaddr_in loopback_addr(int port)
{
    struct sockaddr_in addr;

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((unsigned short)port);
    inet_pton(AF_INET, TEST_LOOPBACK, &addr.sin_addr);
    return addr;
}

Test(set_io_timeouts, fails_when_rcvtimeo_cannot_be_set)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);

    cr_assert_geq(fd, 0);
    inject_fault(&setsockopt_fault, TEST_FIRST_CALL, -1, EINVAL);
    cr_assert_eq(set_io_timeouts(fd), -1);
    close(fd);
}

Test(set_io_timeouts, fails_when_sndtimeo_cannot_be_set)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);

    cr_assert_geq(fd, 0);
    inject_fault(&setsockopt_fault, TEST_SECOND_CALL, -1, EINVAL);
    cr_assert_eq(set_io_timeouts(fd), -1);
    close(fd);
}

Test(wcr_open, returns_null_when_timeouts_cannot_be_set)
{
    int port;
    int listener = open_listener(&port);

    cr_assert_geq(listener, 0);
    inject_fault(&setsockopt_fault, TEST_FIRST_CALL, -1, EINVAL);
    cr_assert_null(wcr_open(TEST_LOOPBACK, port));
    close(listener);
}

Test(connect_with_timeout, fails_when_flags_cannot_be_read)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr = loopback_addr(WCR_DEFAULT_PORT);

    cr_assert_geq(fd, 0);
    inject_fault(&fcntl_fault, TEST_FIRST_CALL, -1, EBADF);
    cr_assert_eq(connect_with_timeout(fd, (struct sockaddr *)&addr, sizeof(addr)), -1);
    close(fd);
}

Test(connect_with_timeout, fails_when_nonblocking_cannot_be_set)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr = loopback_addr(WCR_DEFAULT_PORT);

    cr_assert_geq(fd, 0);
    inject_fault(&fcntl_fault, TEST_SECOND_CALL, -1, EBADF);
    cr_assert_eq(connect_with_timeout(fd, (struct sockaddr *)&addr, sizeof(addr)), -1);
    close(fd);
}

Test(connect_with_timeout, fails_when_blocking_cannot_be_restored)
{
    int port;
    int listener = open_listener(&port);
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr = loopback_addr(port);

    cr_assert_geq(listener, 0);
    cr_assert_geq(fd, 0);
    inject_fault(&fcntl_fault, TEST_THIRD_CALL, -1, EBADF);
    cr_assert_eq(connect_with_timeout(fd, (struct sockaddr *)&addr, sizeof(addr)), -1);
    close(fd);
    close(listener);
}

Test(connect_with_timeout, succeeds_on_immediate_connect)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr = loopback_addr(WCR_DEFAULT_PORT);

    cr_assert_geq(fd, 0);
    inject_fault(&connect_fault, TEST_FIRST_CALL, 0, 0);
    cr_assert_eq(connect_with_timeout(fd, (struct sockaddr *)&addr, sizeof(addr)), 0);
    close(fd);
}

Test(connect_with_timeout, fails_on_immediate_refusal)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr = loopback_addr(WCR_DEFAULT_PORT);

    cr_assert_geq(fd, 0);
    inject_fault(&connect_fault, TEST_FIRST_CALL, -1, ECONNREFUSED);
    cr_assert_eq(connect_with_timeout(fd, (struct sockaddr *)&addr, sizeof(addr)), -1);
    close(fd);
}

Test(wait_connected, reports_timeout_when_select_expires)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);

    cr_assert_geq(fd, 0);
    inject_fault(&select_fault, TEST_FIRST_CALL, 0, 0);
    cr_assert_eq(wait_connected(fd), -1);
    cr_assert_eq(errno, ETIMEDOUT);
    close(fd);
}

Test(wait_connected, fails_when_select_fails)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);

    cr_assert_geq(fd, 0);
    inject_fault(&select_fault, TEST_FIRST_CALL, -1, EBADF);
    cr_assert_eq(wait_connected(fd), -1);
    close(fd);
}

Test(wait_connected, retries_select_after_eintr)
{
    int port;
    int listener = open_listener(&port);
    int fd;

    cr_assert_geq(listener, 0);
    fd = start_connect(port);
    cr_assert_geq(fd, 0);
    inject_fault(&select_fault, TEST_FIRST_CALL, -1, EINTR);
    cr_assert_eq(wait_connected(fd), 0);
    cr_assert_eq(select_fault.calls, TEST_SECOND_CALL);
    close(fd);
    close(listener);
}

Test(wait_connected, fails_when_so_error_cannot_be_read)
{
    int port;
    int listener = open_listener(&port);
    int fd;

    cr_assert_geq(listener, 0);
    fd = start_connect(port);
    cr_assert_geq(fd, 0);
    inject_fault(&getsockopt_fault, TEST_FIRST_CALL, -1, EBADF);
    cr_assert_eq(wait_connected(fd), -1);
    close(fd);
    close(listener);
}