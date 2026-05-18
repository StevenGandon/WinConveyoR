#include "wcr_client.h"
#include "wcr_crypt.h"
#include "pkg_parsing.h"
#include "wcr_event_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32

wcr_conn *wcr_open(const char *host, int port)
{
    (void)host; (void)port;
    wcr_emit(NULL, WCR_EVENT_ERROR, "[ERROR] wcr_open: WCR protocol not supported on Windows yet");
    return NULL;
}

void wcr_close(wcr_conn *conn) { free(conn); }

int wcr_handshake(wcr_conn *conn, const char *server_pubkey_pem)
{
    (void)conn; (void)server_pubkey_pem;
    wcr_emit(NULL, WCR_EVENT_ERROR, "[ERROR] wcr_handshake: WCR protocol not supported on Windows yet");
    return -1;
}

wcr_msg *wcr_msg_new(const char *payload, uint16_t flags) { (void)payload; (void)flags; return NULL; }
void wcr_msg_free(wcr_msg *msg) { (void)msg; }
int wcr_send(wcr_conn *conn, const wcr_msg *msg) { (void)conn; (void)msg; return -1; }
wcr_msg *wcr_recv(wcr_conn *conn) { (void)conn; return NULL; }

char *wcr_send_recv(wcr_conn *conn, const char *json_payload)
{
    (void)conn; (void)json_payload;
    return NULL;
}

int wcr_auth(wcr_conn *conn, const char *access_key)
{
    (void)conn; (void)access_key;
    wcr_emit(NULL, WCR_EVENT_ERROR, "[ERROR] wcr_auth: WCR protocol not supported on Windows yet");
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
#include <openssl/evp.h>
#include <cJSON.h>

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

wcr_msg *wcr_msg_new(const char *payload, uint16_t flags)
{
    wcr_msg *msg = calloc(1, sizeof(wcr_msg));

    if (!msg)
        return NULL;
    msg->magic = WCR_MAGIC;
    msg->flags = flags;
    if (payload) {
        msg->payload_len = strlen(payload);
        msg->payload = malloc(msg->payload_len + 1);
        if (!msg->payload) {
            free(msg);
            return NULL;
        }
        memcpy(msg->payload, payload, msg->payload_len + 1);
    }
    return msg;
}

void wcr_msg_free(wcr_msg *msg)
{
    if (!msg)
        return;
    free(msg->payload);
    free(msg);
}

int wcr_send(wcr_conn *conn, const wcr_msg *msg)
{
    unsigned char header[WCR_HEADER_SIZE];
    uint32_t magic_be = htonl(msg->magic);
    uint16_t flags_be = htons(msg->flags);
    ssize_t sent;

    memcpy(header, &magic_be, 4);
    memcpy(header + 4, &flags_be, 2);
    write_be64(header + 6, (uint64_t)msg->payload_len);

    sent = send(conn->sockfd, header, WCR_HEADER_SIZE, 0);
    if (sent != WCR_HEADER_SIZE) {
        wcr_emit(NULL, WCR_EVENT_ERROR, "[ERROR] wcr_send: failed to send header");
        return -1;
    }

    if (msg->payload_len > 0) {
        sent = send(conn->sockfd, msg->payload, msg->payload_len, 0);
        if (sent != (ssize_t)msg->payload_len) {
            wcr_emit(NULL, WCR_EVENT_ERROR, "[ERROR] wcr_send: failed to send payload");
            return -1;
        }
    }

    return 0;
}

static char *recv_payload(int sockfd, size_t payload_size)
{
    char *raw;
    size_t total_read = 0;
    ssize_t n;

    if (payload_size == 0) {
        raw = malloc(1);
        if (raw) raw[0] = '\0';
        return raw;
    }

    raw = malloc(payload_size + 1);
    if (!raw) {
        wcr_emit(NULL, WCR_EVENT_ERROR, "[ERROR] recv_payload: malloc failed for %zu bytes", payload_size);
        return NULL;
    }

    while (total_read < payload_size) {
        n = recv(sockfd, raw + total_read, payload_size - total_read, 0);
        if (n <= 0) {
            wcr_emit(NULL, WCR_EVENT_ERROR, "[ERROR] recv_payload: recv failed at %zu/%zu", total_read, payload_size);
            free(raw);
            return NULL;
        }
        total_read += (size_t)n;
    }
    raw[payload_size] = '\0';
    return raw;
}

wcr_msg *wcr_recv(wcr_conn *conn)
{
    unsigned char header[WCR_HEADER_SIZE];
    uint32_t magic_be;
    uint16_t flags_be;
    size_t payload_size;
    char *raw;
    wcr_msg *msg;
    ssize_t n;

    n = recv(conn->sockfd, header, WCR_HEADER_SIZE, MSG_WAITALL);
    if (n != WCR_HEADER_SIZE) {
        wcr_emit(NULL, WCR_EVENT_ERROR, "[ERROR] wcr_recv: failed to read header (got %zd)", n);
        return NULL;
    }

    memcpy(&magic_be, header, 4);
    memcpy(&flags_be, header + 4, 2);

    if (ntohl(magic_be) != WCR_MAGIC) {
        wcr_emit(NULL, WCR_EVENT_ERROR, "[ERROR] wcr_recv: invalid magic 0x%08x", ntohl(magic_be));
        return NULL;
    }

    payload_size = read_be64(header + 6);
    raw = recv_payload(conn->sockfd, payload_size);
    if (!raw)
        return NULL;

    if (conn->encrypted && conn->client_privkey && payload_size > 0) {
        size_t decrypted_len;
        unsigned char *decrypted = wcr_decrypt(conn->client_privkey,
            (unsigned char *)raw, payload_size, &decrypted_len);
        free(raw);
        if (!decrypted) {
            wcr_emit(NULL, WCR_EVENT_ERROR, "[ERROR] wcr_recv: decryption failed");
            return NULL;
        }
        raw = (char *)decrypted;
        payload_size = decrypted_len;
    }

    msg = calloc(1, sizeof(wcr_msg));
    if (!msg) {
        free(raw);
        return NULL;
    }
    msg->magic = WCR_MAGIC;
    msg->flags = ntohs(flags_be);
    msg->payload = raw;
    msg->payload_len = payload_size;
    return msg;
}

wcr_conn *wcr_open(const char *host, int port)
{
    wcr_conn *conn;
    struct sockaddr_in addr;
    struct hostent *he;

    if (!host) {
        wcr_emit(NULL, WCR_EVENT_ERROR, "[ERROR] wcr_open: host is NULL");
        return NULL;
    }

    conn = calloc(1, sizeof(wcr_conn));
    if (!conn) {
        wcr_emit(NULL, WCR_EVENT_ERROR, "[ERROR] wcr_open: calloc failed");
        return NULL;
    }

    conn->sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (conn->sockfd < 0) {
        wcr_emit(NULL, WCR_EVENT_ERROR, "[ERROR] wcr_open: socket() failed");
        free(conn);
        return NULL;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((unsigned short)port);

    if (inet_pton(AF_INET, host, &addr.sin_addr) <= 0) {
        he = gethostbyname(host);
        if (!he) {
            wcr_emit(NULL, WCR_EVENT_ERROR, "[ERROR] wcr_open: cannot resolve host '%s'", host);
            close(conn->sockfd);
            free(conn);
            return NULL;
        }
        memcpy(&addr.sin_addr, he->h_addr_list[0], (size_t)he->h_length);
    }

    if (connect(conn->sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        wcr_emit(NULL, WCR_EVENT_ERROR, "[ERROR] wcr_open: connect() to %s:%d failed", host, port);
        close(conn->sockfd);
        free(conn);
        return NULL;
    }

    wcr_emit(NULL, WCR_EVENT_DEBUG, "[DEBUG] wcr_open: connected to %s:%d", host, port);
    conn->session_id[0] = '\0';
    return conn;
}

void wcr_close(wcr_conn *conn)
{
    if (!conn)
        return;

    if (conn->sockfd >= 0) {
        wcr_msg *goodbye = wcr_msg_new("{\"action\":\"goodbye\",\"data\":{}}", 0);
        if (goodbye) {
            wcr_send(conn, goodbye);
            wcr_msg_free(goodbye);
        }
        wcr_msg *resp = wcr_recv(conn);
        wcr_msg_free(resp);
        close(conn->sockfd);
    }

    if (conn->client_privkey)
        EVP_PKEY_free(conn->client_privkey);
    if (conn->server_pubkey)
        EVP_PKEY_free(conn->server_pubkey);

    free(conn);
}

static char *build_init_rsa_payload(const char *escaped_pem)
{
    size_t payload_size = strlen(escaped_pem) + WCR_JSON_OVERHEAD;
    char *payload = malloc(payload_size);

    if (!payload)
        return NULL;
    snprintf(payload, payload_size,
        "{\"action\":\"init_rsa\",\"data\":{\"key\":\"%s\"}}",
        escaped_pem);
    return payload;
}

static int check_response_code(const char *response)
{
    cJSON *root = cJSON_Parse(response);
    cJSON *code = root ? cJSON_GetObjectItemCaseSensitive(root, "code") : NULL;
    int ok = code && cJSON_IsNumber(code) && code->valueint == 0;

    cJSON_Delete(root);
    return ok ? 0 : -1;
}

int wcr_handshake(wcr_conn *conn, const char *server_pubkey_pem)
{
    EVP_PKEY *client_key;
    char *escaped_pem;
    char *init_payload;
    wcr_msg *req;
    wcr_msg *resp;

    if (!conn) {
        wcr_emit(NULL, WCR_EVENT_ERROR, "[ERROR] wcr_handshake: NULL argument");
        return -1;
    }

    if (server_pubkey_pem) {
        conn->server_pubkey = parse_pubkey_pem(server_pubkey_pem);
        if (!conn->server_pubkey) {
            wcr_emit(NULL, WCR_EVENT_ERROR, "[ERROR] wcr_handshake: failed to parse server public key");
            return -1;
        }
    }

    client_key = generate_rsa_keypair();
    if (!client_key) {
        wcr_emit(NULL, WCR_EVENT_ERROR, "[ERROR] wcr_handshake: RSA keygen failed");
        return -1;
    }

    escaped_pem = serialize_pubkey_pem(client_key);
    if (!escaped_pem) {
        wcr_emit(NULL, WCR_EVENT_ERROR, "[ERROR] wcr_handshake: failed to serialize client pubkey");
        EVP_PKEY_free(client_key);
        return -1;
    }

    conn->client_privkey = client_key;
    conn->encrypted = 1;

    init_payload = build_init_rsa_payload(escaped_pem);
    free(escaped_pem);
    if (!init_payload)
        return -1;

    req = wcr_msg_new(init_payload, 0);
    free(init_payload);
    if (!req)
        return -1;
    if (wcr_send(conn, req) != 0) {
        wcr_emit(NULL, WCR_EVENT_ERROR, "[ERROR] wcr_handshake: failed to send init_rsa");
        wcr_msg_free(req);
        return -1;
    }
    wcr_msg_free(req);

    resp = wcr_recv(conn);
    if (!resp || !resp->payload) {
        wcr_emit(NULL, WCR_EVENT_ERROR, "[ERROR] wcr_handshake: no response to init_rsa");
        wcr_msg_free(resp);
        return -1;
    }

    if (check_response_code(resp->payload) != 0) {
        wcr_emit(NULL, WCR_EVENT_ERROR, "[ERROR] wcr_handshake: init_rsa rejected: %s", resp->payload);
        wcr_msg_free(resp);
        return -1;
    }

    wcr_msg_free(resp);
    wcr_emit(NULL, WCR_EVENT_DEBUG, "[DEBUG] wcr_handshake: encryption established");
    return 0;
}

char *wcr_send_recv(wcr_conn *conn, const char *json_payload)
{
    wcr_msg *req;
    wcr_msg *resp;
    char *result;

    if (!conn || !json_payload) {
        wcr_emit(NULL, WCR_EVENT_ERROR, "[ERROR] wcr_send_recv: NULL argument");
        return NULL;
    }

    req = wcr_msg_new(json_payload, 0);
    if (!req)
        return NULL;
    if (wcr_send(conn, req) != 0) {
        wcr_msg_free(req);
        return NULL;
    }
    wcr_msg_free(req);

    resp = wcr_recv(conn);
    if (!resp)
        return NULL;
    result = resp->payload;
    resp->payload = NULL;
    wcr_msg_free(resp);
    return result;
}

static int extract_session_id(const char *json, char *out, size_t out_sz)
{
    const char *key = "\"session_id\"";
    const char *p = strstr(json, key);
    size_t i = 0;

    if (!p)
        return -1;
    p += strlen(key);
    while (*p == ' ' || *p == ':' || *p == '\t')
        p++;
    while (*p && *p != ',' && *p != '}' && *p != ' ' && i < out_sz - 1)
        out[i++] = *p++;
    out[i] = '\0';
    return (i > 0) ? 0 : -1;
}

static char *wcr_build_payload_va(const char *action,
                                  const char *session_id,
                                  const char *fmt, va_list args)
{
    int needed;
    int offset;
    char *buf;
    va_list args_copy;

    if (fmt) {
        va_copy(args_copy, args);
        needed = vsnprintf(NULL, 0, fmt, args_copy);
        va_end(args_copy);
        if (needed < 0)
            return NULL;
    } else {
        needed = 0;
    }

    needed += WCR_JSON_OVERHEAD + (int)strlen(action)
            + (session_id ? (int)strlen(session_id) : 0);

    buf = malloc((size_t)needed);
    if (!buf)
        return NULL;

    if (session_id) {
        offset = snprintf(buf, (size_t)needed,
            "{\"action\":\"%s\",\"data\":{\"session_id\":%s", action, session_id);
    } else {
        offset = snprintf(buf, (size_t)needed,
            "{\"action\":\"%s\",\"data\":{", action);
    }
    if (fmt) {
        if (session_id)
            buf[offset++] = ',';
        va_copy(args_copy, args);
        offset += vsnprintf(buf + offset, (size_t)(needed - offset), fmt, args_copy);
        va_end(args_copy);
    }
    buf[offset++] = '}';
    buf[offset++] = '}';
    buf[offset] = '\0';
    return buf;
}

static char *wcr_build_payload(const char *action,
                               const char *session_id, const char *fmt, ...)
{
    char *result;
    va_list args;

    va_start(args, fmt);
    result = wcr_build_payload_va(action, session_id, fmt, args);
    va_end(args);
    return result;
}

static char *wcr_request(wcr_conn *conn, const char *action, const char *fmt, ...)
{
    char *payload;
    char *result;
    va_list args;

    if (!conn || conn->session_id[0] == '\0')
        return NULL;

    if (fmt) {
        va_start(args, fmt);
        payload = wcr_build_payload_va(action, conn->session_id, fmt, args);
        va_end(args);
    } else {
        payload = wcr_build_payload(action, conn->session_id, NULL);
    }
    if (!payload)
        return NULL;
    result = wcr_send_recv(conn, payload);
    free(payload);
    return result;
}

int wcr_auth(wcr_conn *conn, const char *access_key)
{
    char *payload;
    char *response;

    if (!conn) return -1;
    if (!access_key) access_key = "";

    payload = wcr_build_payload("user", NULL,
        "\"password\":\"%s\"", access_key);
    if (!payload) return -1;

    response = wcr_send_recv(conn, payload);
    free(payload);
    if (!response) {
        wcr_emit(NULL, WCR_EVENT_ERROR, "[ERROR] wcr_auth: no response");
        return -1;
    }

    if (extract_session_id(response, conn->session_id, sizeof(conn->session_id)) == 0) {
        free(response);
        wcr_emit(NULL, WCR_EVENT_DEBUG, "[DEBUG] wcr_auth: authenticated, session_id=%s", conn->session_id);
        return 0;
    }

    wcr_emit(NULL, WCR_EVENT_ERROR, "[ERROR] wcr_auth: could not extract session_id from: %s", response);
    free(response);
    return -1;
}

char *wcr_get_listing(wcr_conn *conn)
{
    return wcr_request(conn, "get_listing", NULL);
}

char *wcr_get_hash(wcr_conn *conn)
{
    return wcr_request(conn, "get_hash", NULL);
}

char *wcr_get_package_listing(wcr_conn *conn, const char *package_name)
{
    if (!package_name) return NULL;
    return wcr_request(conn, "get_package_listing",
        "\"package_name\":\"%s\"", package_name);
}

char *wcr_get_package_metadata(wcr_conn *conn, const char *package_name, const char *location_hash)
{
    if (!package_name || !location_hash) return NULL;
    return wcr_request(conn, "get_package_metadata",
        "\"package_name\":\"%s\",\"location_hash\":\"%s\"",
        package_name, location_hash);
}

#endif
