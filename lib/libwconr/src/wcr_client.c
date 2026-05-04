#include "wcr_client.h"
#include "pkg_parsing.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32

wcr_conn *wcr_open(const char *host, int port)
{
    (void)host; (void)port;
    fprintf(stderr, "[ERROR] wcr_open: WCR protocol not supported on Windows yet\n");
    return NULL;
}

void wcr_close(wcr_conn *conn) { free(conn); }

char *wcr_send_recv(wcr_conn *conn, const char *json_payload)
{
    (void)conn; (void)json_payload;
    return NULL;
}

int wcr_auth(wcr_conn *conn, const char *access_key)
{
    (void)conn; (void)access_key;
    fprintf(stderr, "[ERROR] wcr_auth: WCR protocol not supported on Windows yet\n");
    return -1;
}

char *wcr_get_listing(wcr_conn *conn) { (void)conn; return NULL; }
char *wcr_get_hash(wcr_conn *conn) { (void)conn; return NULL; }
char *wcr_get_package_listing(wcr_conn *conn, const char *package_name) { (void)conn; (void)package_name; return NULL; }
char *wcr_get_package_metadata(wcr_conn *conn, const char *package_name, const char *location_hash) { (void)conn; (void)package_name; (void)location_hash; return NULL; }

#else

#include <unistd.h>
#include <stdarg.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

static void write_be64(unsigned char *buf, uint64_t val)
{
    buf[0] = (unsigned char)(val >> 56);
    buf[1] = (unsigned char)(val >> 48);
    buf[2] = (unsigned char)(val >> 40);
    buf[3] = (unsigned char)(val >> 32);
    buf[4] = (unsigned char)(val >> 24);
    buf[5] = (unsigned char)(val >> 16);
    buf[6] = (unsigned char)(val >> 8);
    buf[7] = (unsigned char)(val);
}

static uint64_t read_be64(const unsigned char *buf)
{
    return ((uint64_t)buf[0] << 56) | ((uint64_t)buf[1] << 48) |
           ((uint64_t)buf[2] << 40) | ((uint64_t)buf[3] << 32) |
           ((uint64_t)buf[4] << 24) | ((uint64_t)buf[5] << 16) |
           ((uint64_t)buf[6] << 8)  | (uint64_t)buf[7];
}

static int wcr_send_raw(int sockfd, const char *payload)
{
    unsigned char header[WCR_HEADER_SIZE];
    size_t payload_len = strlen(payload);
    uint32_t magic_be = htonl(WCR_MAGIC);
    ssize_t sent;

    memcpy(header, &magic_be, 4);
    header[4] = 0;
    header[5] = 0;
    write_be64(header + 6, (uint64_t)payload_len);

    sent = send(sockfd, header, WCR_HEADER_SIZE, 0);
    if (sent != WCR_HEADER_SIZE) {
        fprintf(stderr, "[ERROR] wcr_send_raw: failed to send header\n");
        return -1;
    }

    sent = send(sockfd, payload, payload_len, 0);
    if (sent != (ssize_t)payload_len) {
        fprintf(stderr, "[ERROR] wcr_send_raw: failed to send payload\n");
        return -1;
    }

    return 0;
}

static char *wcr_recv_raw(int sockfd)
{
    unsigned char header[WCR_HEADER_SIZE];
    size_t payload_size;
    size_t total_read = 0;
    char *content;
    ssize_t n;

    n = recv(sockfd, header, WCR_HEADER_SIZE, MSG_WAITALL);
    if (n != WCR_HEADER_SIZE) {
        fprintf(stderr, "[ERROR] wcr_recv_raw: failed to read header (got %zd)\n", n);
        return NULL;
    }

    uint32_t magic_be;
    memcpy(&magic_be, header, 4);
    unsigned int magic = ntohl(magic_be);

    if (magic != WCR_MAGIC) {
        fprintf(stderr, "[ERROR] wcr_recv_raw: invalid magic 0x%08x\n", magic);
        return NULL;
    }

    payload_size = read_be64(header + 6);

    if (payload_size == 0) {
        content = malloc(1);
        if (content) content[0] = '\0';
        return content;
    }

    content = malloc(payload_size + 1);
    if (!content) {
        fprintf(stderr, "[ERROR] wcr_recv_raw: malloc failed for %zu bytes\n", payload_size);
        return NULL;
    }

    while (total_read < payload_size) {
        n = recv(sockfd, content + total_read, payload_size - total_read, 0);
        if (n <= 0) {
            fprintf(stderr, "[ERROR] wcr_recv_raw: recv failed at %zu/%zu\n", total_read, payload_size);
            free(content);
            return NULL;
        }
        total_read += (size_t)n;
    }

    content[payload_size] = '\0';
    return content;
}

wcr_conn *wcr_open(const char *host, int port)
{
    wcr_conn *conn;
    struct sockaddr_in addr;
    struct hostent *he;

    if (!host) {
        fprintf(stderr, "[ERROR] wcr_open: host is NULL\n");
        return NULL;
    }

    conn = calloc(1, sizeof(wcr_conn));
    if (!conn) {
        fprintf(stderr, "[ERROR] wcr_open: calloc failed\n");
        return NULL;
    }

    conn->sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (conn->sockfd < 0) {
        fprintf(stderr, "[ERROR] wcr_open: socket() failed\n");
        free(conn);
        return NULL;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((unsigned short)port);

    if (inet_pton(AF_INET, host, &addr.sin_addr) <= 0) {
        he = gethostbyname(host);
        if (!he) {
            fprintf(stderr, "[ERROR] wcr_open: cannot resolve host '%s'\n", host);
            close(conn->sockfd);
            free(conn);
            return NULL;
        }
        memcpy(&addr.sin_addr, he->h_addr_list[0], (size_t)he->h_length);
    }

    if (connect(conn->sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        fprintf(stderr, "[ERROR] wcr_open: connect() to %s:%d failed\n", host, port);
        close(conn->sockfd);
        free(conn);
        return NULL;
    }

    printf("[DEBUG] wcr_open: connected to %s:%d\n", host, port);
    conn->session_id[0] = '\0';
    return conn;
}

void wcr_close(wcr_conn *conn)
{
    if (!conn)
        return;

    if (conn->sockfd >= 0) {
        wcr_send_raw(conn->sockfd, "{\"action\":\"goodbye\",\"data\":{}}");
        char *resp = wcr_recv_raw(conn->sockfd);
        free(resp);
        close(conn->sockfd);
    }

    free(conn);
}

char *wcr_send_recv(wcr_conn *conn, const char *json_payload)
{
    if (!conn || !json_payload) {
        fprintf(stderr, "[ERROR] wcr_send_recv: NULL argument\n");
        return NULL;
    }

    if (wcr_send_raw(conn->sockfd, json_payload) != 0) {
        return NULL;
    }

    return wcr_recv_raw(conn->sockfd);
}

static int extract_session_id(const char *json, char *out, size_t out_sz)
{
    const char *key = "\"session_id\"";
    const char *p = strstr(json, key);
    size_t i = 0;

    if (!p) return -1;
    p += strlen(key);

    while (*p == ' ' || *p == ':') p++;

    while (*p && *p != ',' && *p != '}' && *p != ' ' && i < out_sz - 1) {
        out[i++] = *p++;
    }
    out[i] = '\0';

    return (i > 0) ? 0 : -1;
}

static int wcr_build_payload(char *buf, size_t buf_size, const char *action,
                             const char *session_id, const char *fmt, ...)
{
    int offset;
    int written;
    va_list args;

    if (session_id) {
        offset = snprintf(buf, buf_size,
            "{\"action\":\"%s\",\"data\":{\"session_id\":%s", action, session_id);
    } else {
        offset = snprintf(buf, buf_size,
            "{\"action\":\"%s\",\"data\":{", action);
    }
    if (offset < 0 || (size_t)offset >= buf_size)
        return -1;
    if (fmt) {
        if (session_id)
            buf[offset++] = ',';
        va_start(args, fmt);
        written = vsnprintf(buf + offset, buf_size - (size_t)offset, fmt, args);
        va_end(args);
        if (written < 0 || (size_t)(offset + written) >= buf_size)
            return -1;
        offset += written;
    }
    if ((size_t)(offset + 2) >= buf_size)
        return -1;
    buf[offset++] = '}';
    buf[offset++] = '}';
    buf[offset] = '\0';
    return 0;
}

int wcr_auth(wcr_conn *conn, const char *access_key)
{
    char payload[WCR_PAYLOAD_LARGE];
    char *response;

    if (!conn) return -1;
    if (!access_key) access_key = "";

    wcr_build_payload(payload, sizeof(payload), "user", NULL,
        "\"password\":\"%s\"", access_key);

    response = wcr_send_recv(conn, payload);
    if (!response) {
        fprintf(stderr, "[ERROR] wcr_auth: no response\n");
        return -1;
    }

    if (extract_session_id(response, conn->session_id, sizeof(conn->session_id)) == 0) {
        free(response);
        printf("[DEBUG] wcr_auth: authenticated, session_id=%s\n", conn->session_id);
        return 0;
    }

    fprintf(stderr, "[ERROR] wcr_auth: could not extract session_id from: %s\n", response);
    free(response);
    return -1;
}

char *wcr_get_listing(wcr_conn *conn)
{
    char payload[WCR_PAYLOAD_SMALL];

    if (!conn || conn->session_id[0] == '\0') return NULL;

    wcr_build_payload(payload, sizeof(payload), "get_listing",
        conn->session_id, NULL);

    return wcr_send_recv(conn, payload);
}

char *wcr_get_hash(wcr_conn *conn)
{
    char payload[WCR_PAYLOAD_SMALL];

    if (!conn || conn->session_id[0] == '\0') return NULL;

    wcr_build_payload(payload, sizeof(payload), "get_hash",
        conn->session_id, NULL);

    return wcr_send_recv(conn, payload);
}

char *wcr_get_package_listing(wcr_conn *conn, const char *package_name)
{
    char payload[WCR_PAYLOAD_LARGE];

    if (!conn || !package_name || conn->session_id[0] == '\0') return NULL;

    wcr_build_payload(payload, sizeof(payload), "get_package_listing",
        conn->session_id, "\"package_name\":\"%s\"", package_name);

    return wcr_send_recv(conn, payload);
}

char *wcr_get_package_metadata(wcr_conn *conn, const char *package_name, const char *location_hash)
{
    char payload[WCR_PAYLOAD_LARGE];

    if (!conn || !package_name || !location_hash || conn->session_id[0] == '\0') return NULL;

    wcr_build_payload(payload, sizeof(payload), "get_package_metadata",
        conn->session_id, "\"package_name\":\"%s\",\"location_hash\":\"%s\"",
        package_name, location_hash);

    return wcr_send_recv(conn, payload);
}

#endif