#include "libwconr.h"
#include "pkg_downloader.h"
#include "file_utils.h"
#include "wcr_client.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

#ifdef _WIN32
    #include <io.h>
    #define access _access
    #define F_OK 0
#else
    #include <unistd.h>
#endif

static int parse_host_port(const char *source_uri, char *host, size_t host_sz, int *port)
{
    const char *colon;
    size_t hlen;

    colon = strrchr(source_uri, ':');
    if (!colon) {
        hlen = strlen(source_uri);
        if (hlen >= host_sz) hlen = host_sz - 1;
        memcpy(host, source_uri, hlen);
        host[hlen] = '\0';
        *port = WCR_DEFAULT_PORT;
        return 0;
    }

    hlen = (size_t)(colon - source_uri);
    if (hlen >= host_sz) hlen = host_sz - 1;
    memcpy(host, source_uri, hlen);
    host[hlen] = '\0';
    *port = atoi(colon + 1);
    if (*port <= 0) *port = WCR_DEFAULT_PORT;
    return 0;
}

static int convert_listing_to_pkgs_list(const char *listing, const char *filepath)
{
    FILE *fp;
    const char *line_start;
    const char *p;

    fp = fopen(filepath, "w");
    if (!fp) {
        fprintf(stderr, "[ERROR] convert_listing_to_pkgs_list: cannot open %s\n", filepath);
        return -1;
    }

    line_start = listing;
    while (*line_start) {
        char name[256] = {0};
        char version[64] = {0};
        char hash[128] = {0};
        int field = 0;
        size_t i = 0;

        p = line_start;
        while (*p && *p != '\r' && *p != '\n') {
            if (*p == ' ') {
                field++;
                i = 0;
            } else {
                if (field == 0 && i < sizeof(name) - 1) name[i++] = *p;
                else if (field == 1 && i < sizeof(version) - 1) version[i++] = *p;
                else if (field == 2 && i < sizeof(hash) - 1) hash[i++] = *p;
            }
            p++;
        }

        if (name[0] && version[0]) {
            fprintf(fp, "%s,%s,/register/%s.list,%s\n", name, version, name, hash);
        }

        while (*p == '\r' || *p == '\n') p++;
        line_start = p;
    }

    fclose(fp);
    return 0;
}

static int is_pkgs_list_current(const char *pkgs_list_path, const char *remote_hash)
{
    char *local_hash;
    size_t len;
    int current;

    if (!remote_hash || access(pkgs_list_path, F_OK) != 0)
        return 0;

    local_hash = calculate_sha256_file(pkgs_list_path);
    if (!local_hash)
        return 0;

    len = strlen(remote_hash);
    while (len > 0 && (remote_hash[len-1] == '\n' || remote_hash[len-1] == '\r' || remote_hash[len-1] == ' '))
        len--;

    current = (strncmp(remote_hash, local_hash, len) == 0 && local_hash[len] == '\0');
    printf("[DEBUG] sync_wcr: remote_hash=%.*s local_hash=%s\n", (int)len, remote_hash, local_hash);
    free(local_hash);
    return current;
}

static int wcr_download_listing(wcr_conn *conn, const char *pkgs_list_path)
{
    char *listing;

    printf("[INFO] sync_wcr: fetching listing via WCR protocol...\n");

    listing = wcr_get_listing(conn);
    if (!listing) {
        fprintf(stderr, "[ERROR] sync_wcr: get_listing failed\n");
        return -1;
    }

    printf("[DEBUG] sync_wcr: listing received (%zu bytes)\n", strlen(listing));

    if (convert_listing_to_pkgs_list(listing, pkgs_list_path) != 0) {
        free(listing);
        return -1;
    }

    free(listing);
    printf("[INFO] sync_wcr: pkgs.list updated from WCR source\n");
    return 0;
}

static int sync_wcr(const wcr_state *state, const char *source_uri)
{
    char host[256];
    int port;
    wcr_conn *conn;
    char *pkgs_list_path;
    char *remote_hash;
    const char *access_key;
    int rc;

    parse_host_port(source_uri, host, sizeof(host), &port);

    conn = wcr_open(host, port);
    if (!conn) return -1;

    access_key = getenv("WCR_ACCESS");
    if (wcr_auth(conn, access_key ? access_key : "") != 0) {
        wcr_close(conn);
        return -1;
    }

    pkgs_list_path = get_cache_path(state, "pkgs.list");
    if (!pkgs_list_path) {
        wcr_close(conn);
        return -1;
    }

    remote_hash = wcr_get_hash(conn);
    if (!remote_hash)
        fprintf(stderr, "[WARNING] sync_wcr: get_hash failed, forcing download\n");

    if (is_pkgs_list_current(pkgs_list_path, remote_hash)) {
        printf("[INFO] sync_wcr: pkgs.list is up to date\n");
        free(remote_hash);
        free(pkgs_list_path);
        wcr_close(conn);
        return 0;
    }
    free(remote_hash);

    rc = wcr_download_listing(conn, pkgs_list_path);
    free(pkgs_list_path);
    wcr_close(conn);
    return rc;
}

static void trim_whitespace(char *s)
{
    char *start = s;
    char *end;

    while (*start == ' ' || *start == '\n' || *start == '\r' || *start == '\t')
        start++;

    if (start != s)
        memmove(s, start, strlen(start) + 1);

    end = s + strlen(s) - 1;
    while (end > s && (*end == ' ' || *end == '\n' || *end == '\r' || *end == '\t'))
        *end-- = '\0';
}

static int http_needs_download(protocol_type proto, const char *checksum_url, const char *pkgs_list_path)
{
    char *remote_checksum;
    char *local_checksum;
    int result;

    if (access(pkgs_list_path, F_OK) != 0) {
        printf("[INFO] pkgs.list not found locally, downloading...\n");
        return 1;
    }

    printf("[INFO] pkgs.list found locally, checking integrity...\n");

    remote_checksum = download_to_string(proto, checksum_url);
    if (!remote_checksum) {
        fprintf(stderr, "[WARNING] Failed to download checksum.hsh, forcing re-download\n");
        return 1;
    }

    trim_whitespace(remote_checksum);
    printf("[DEBUG] remote_checksum=%s\n", remote_checksum);

    local_checksum = calculate_sha256_file(pkgs_list_path);
    if (!local_checksum) {
        fprintf(stderr, "[ERROR] Failed to calculate local checksum\n");
        free(remote_checksum);
        return 1;
    }

    printf("[DEBUG] local_checksum=%s\n", local_checksum);
    result = (strcmp(local_checksum, remote_checksum) != 0);

    if (result)
        printf("[INFO] Checksums differ, re-downloading pkgs.list...\n");
    else
        printf("[INFO] Checksums match, pkgs.list is up to date\n");

    free(remote_checksum);
    free(local_checksum);
    return result;
}

static int sync_http(const wcr_state *state, protocol_type proto, const char *source_uri)
{
    char *pkgs_list_url;
    char *checksum_url;
    char *pkgs_list_path;
    int rc;

    pkgs_list_url = build_url(source_uri, "pkgs.list");
    if (!pkgs_list_url)
        return -1;

    checksum_url = build_url(source_uri, "checksum.hsh");
    if (!checksum_url) {
        free(pkgs_list_url);
        return -1;
    }

    printf("[DEBUG] pkgs_list_url=%s\n", pkgs_list_url);
    printf("[DEBUG] checksum_url=%s\n", checksum_url);

    pkgs_list_path = get_cache_path(state, "pkgs.list");
    if (!pkgs_list_path) {
        free(pkgs_list_url);
        free(checksum_url);
        return -1;
    }

    if (!http_needs_download(proto, checksum_url, pkgs_list_path)) {
        free(pkgs_list_url);
        free(checksum_url);
        free(pkgs_list_path);
        return 0;
    }
    free(checksum_url);

    printf("[INFO] Downloading pkgs.list...\n");
    rc = download_to_file(proto, pkgs_list_url, pkgs_list_path);
    free(pkgs_list_url);
    free(pkgs_list_path);

    if (rc == 0)
        printf("[INFO] Successfully downloaded pkgs.list\n");
    else
        fprintf(stderr, "[ERROR] Failed to download pkgs.list\n");

    return rc;
}

int sync_package_list(const wcr_state *state, protocol_type proto, const char *source_uri)
{
    int rc;

    printf("[INFO] sync_package_list: source_uri=%s proto=%d\n", source_uri, proto);

    if (!state || !source_uri) {
        fprintf(stderr, "[ERROR] sync_package_list: state or source_uri is NULL\n");
        return -1;
    }

    if (proto == PROT_WCR)
        return sync_wcr(state, source_uri);

    if (curl_global_init(CURL_GLOBAL_DEFAULT) != 0) {
        fprintf(stderr, "[ERROR] sync_package_list: curl_global_init failed\n");
        return -1;
    }

    rc = sync_http(state, proto, source_uri);
    curl_global_cleanup();
    return rc;
}
