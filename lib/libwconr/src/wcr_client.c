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
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

static int wcr_send_raw(int sockfd, const char *payload)
{
    unsigned char header[14];
    size_t payload_len = strlen(payload);
    unsigned int magic = WCR_MAGIC;
    ssize_t sent;

    header[0] = (unsigned char)((magic >> 24) & 0xFF);
    header[1] = (unsigned char)((magic >> 16) & 0xFF);
    header[2] = (unsigned char)((magic >> 8) & 0xFF);
    header[3] = (unsigned char)(magic & 0xFF);

    header[4] = 0;
    header[5] = 0;

    header[6] = (unsigned char)((payload_len >> 56) & 0xFF);
    header[7] = (unsigned char)((payload_len >> 48) & 0xFF);
    header[8] = (unsigned char)((payload_len >> 40) & 0xFF);
    header[9] = (unsigned char)((payload_len >> 32) & 0xFF);
    header[10] = (unsigned char)((payload_len >> 24) & 0xFF);
    header[11] = (unsigned char)((payload_len >> 16) & 0xFF);
    header[12] = (unsigned char)((payload_len >> 8) & 0xFF);
    header[13] = (unsigned char)(payload_len & 0xFF);

    sent = send(sockfd, header, 14, 0);
    if (sent != 14) {
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
    unsigned char header[14];
    size_t payload_size;
    size_t total_read = 0;
    char *content;
    ssize_t n;

    n = recv(sockfd, header, 14, MSG_WAITALL);
    if (n != 14) {
        fprintf(stderr, "[ERROR] wcr_recv_raw: failed to read header (got %zd)\n", n);
        return NULL;
    }

    unsigned int magic = ((unsigned int)header[0] << 24) |
                         ((unsigned int)header[1] << 16) |
                         ((unsigned int)header[2] << 8) |
                         (unsigned int)header[3];

    if (magic != WCR_MAGIC) {
        fprintf(stderr, "[ERROR] wcr_recv_raw: invalid magic 0x%08x\n", magic);
        return NULL;
    }

    payload_size = ((size_t)header[6] << 56) |
                   ((size_t)header[7] << 48) |
                   ((size_t)header[8] << 40) |
                   ((size_t)header[9] << 32) |
                   ((size_t)header[10] << 24) |
                   ((size_t)header[11] << 16) |
                   ((size_t)header[12] << 8) |
                   (size_t)header[13];

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

int wcr_auth(wcr_conn *conn, const char *access_key)
{
    char payload[512];
    char *response;

    if (!conn) return -1;
    if (!access_key) access_key = "";

    snprintf(payload, sizeof(payload),
        "{\"action\":\"user\",\"data\":{\"password\":\"%s\"}}", access_key);

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
    char payload[256];

    if (!conn || conn->session_id[0] == '\0') return NULL;

    snprintf(payload, sizeof(payload),
        "{\"action\":\"get_listing\",\"data\":{\"session_id\":%s}}", conn->session_id);

    return wcr_send_recv(conn, payload);
}

char *wcr_get_hash(wcr_conn *conn)
{
    char payload[256];

    if (!conn || conn->session_id[0] == '\0') return NULL;

    snprintf(payload, sizeof(payload),
        "{\"action\":\"get_hash\",\"data\":{\"session_id\":%s}}", conn->session_id);

    return wcr_send_recv(conn, payload);
}

char *wcr_get_package_listing(wcr_conn *conn, const char *package_name)
{
    char payload[512];

    if (!conn || !package_name || conn->session_id[0] == '\0') return NULL;

    snprintf(payload, sizeof(payload),
        "{\"action\":\"get_package_listing\",\"data\":{\"session_id\":%s,\"package_name\":\"%s\"}}",
        conn->session_id, package_name);

    return wcr_send_recv(conn, payload);
}

char *wcr_get_package_metadata(wcr_conn *conn, const char *package_name, const char *location_hash)
{
    char payload[512];

    if (!conn || !package_name || !location_hash || conn->session_id[0] == '\0') return NULL;

    snprintf(payload, sizeof(payload),
        "{\"action\":\"get_package_metadata\",\"data\":{\"session_id\":%s,\"package_name\":\"%s\",\"location_hash\":\"%s\"}}",
        conn->session_id, package_name, location_hash);

    return wcr_send_recv(conn, payload);
}

#endif