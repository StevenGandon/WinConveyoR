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

static int sync_wcr(const wcr_state *state, const char *source_uri)
{
    char host[256];
    int port;
    wcr_conn *conn = NULL;
    char *pkgs_list_path = NULL;
    char *remote_hash = NULL;
    char *local_hash = NULL;
    char *listing = NULL;
    const char *access_key;
    int rc = -1;

    parse_host_port(source_uri, host, sizeof(host), &port);

    conn = wcr_open(host, port);
    if (!conn) return -1;

    access_key = getenv("WCR_ACCESS");
    if (wcr_auth(conn, access_key ? access_key : "") != 0) {
        goto cleanup;
    }

    pkgs_list_path = get_cache_path(state, "pkgs.list");
    if (!pkgs_list_path) goto cleanup;

    remote_hash = wcr_get_hash(conn);
    if (!remote_hash) {
        fprintf(stderr, "[WARNING] sync_wcr: get_hash failed, forcing download\n");
    }

    if (remote_hash && access(pkgs_list_path, F_OK) == 0) {
        local_hash = calculate_sha256_file(pkgs_list_path);
        if (local_hash) {
            char *trimmed = remote_hash;
            size_t len = strlen(trimmed);
            while (len > 0 && (trimmed[len-1] == '\n' || trimmed[len-1] == '\r' || trimmed[len-1] == ' '))
                trimmed[--len] = '\0';

            printf("[DEBUG] sync_wcr: remote_hash=%s local_hash=%s\n", trimmed, local_hash);
            if (strcmp(trimmed, local_hash) == 0) {
                printf("[INFO] sync_wcr: pkgs.list is up to date\n");
                rc = 0;
                goto cleanup;
            }
        }
    }

    printf("[INFO] sync_wcr: fetching listing via WCR protocol...\n");
    listing = wcr_get_listing(conn);
    if (!listing) {
        fprintf(stderr, "[ERROR] sync_wcr: get_listing failed\n");
        goto cleanup;
    }

    printf("[DEBUG] sync_wcr: listing received (%zu bytes)\n", strlen(listing));

    if (convert_listing_to_pkgs_list(listing, pkgs_list_path) != 0) {
        goto cleanup;
    }

    printf("[INFO] sync_wcr: pkgs.list updated from WCR source\n");
    rc = 0;

cleanup:
    free(pkgs_list_path);
    free(remote_hash);
    free(local_hash);
    free(listing);
    wcr_close(conn);
    return rc;
}

struct sync_resources_s {
    char *pkgs_list_url;
    char *checksum_url;
    char *pkgs_list_path;
    char *remote_checksum;
    char *local_checksum;
};

static int cleanup_sync_resources(struct sync_resources_s *res, int result)
{
    if (res->pkgs_list_url) free(res->pkgs_list_url);
    if (res->checksum_url) free(res->checksum_url);
    if (res->pkgs_list_path) free(res->pkgs_list_path);
    if (res->remote_checksum) free(res->remote_checksum);
    if (res->local_checksum) free(res->local_checksum);

    curl_global_cleanup();

    return result;
}

int sync_package_list(const wcr_state *state, protocol_type proto, const char *source_uri)
{
    struct sync_resources_s res = {NULL, NULL, NULL, NULL, NULL};
    size_t uri_len;
    int needs_download = 0;

    printf("[INFO] sync_package_list: source_uri=%s proto=%d\n", source_uri, proto);

    if (!state || !source_uri) {
        fprintf(stderr, "[ERROR] sync_package_list: state or source_uri is NULL\n");
        return -1;
    }

    if (proto == PROT_WCR) {
        return sync_wcr(state, source_uri);
    }

    if (curl_global_init(CURL_GLOBAL_DEFAULT) != 0) {
        fprintf(stderr, "[ERROR] sync_package_list: curl_global_init failed\n");
        return -1;
    }

    uri_len = strlen(source_uri);

    res.pkgs_list_url = malloc(uri_len + strlen("/pkgs.list") + 1);
    if (!res.pkgs_list_url) {
        fprintf(stderr, "[ERROR] sync_package_list: malloc failed for pkgs_list_url\n");
        return cleanup_sync_resources(&res, -1);
    }

    if (source_uri[uri_len - 1] == '/') {
        sprintf(res.pkgs_list_url, "%spkgs.list", source_uri);
    } else {
        sprintf(res.pkgs_list_url, "%s/pkgs.list", source_uri);
    }

    res.checksum_url = malloc(uri_len + strlen("/checksum.hsh") + 1);
    if (!res.checksum_url) {
        fprintf(stderr, "[ERROR] sync_package_list: malloc failed for checksum_url\n");
        return cleanup_sync_resources(&res, -1);
    }

    if (source_uri[uri_len - 1] == '/') {
        sprintf(res.checksum_url, "%schecksum.hsh", source_uri);
    } else {
        sprintf(res.checksum_url, "%s/checksum.hsh", source_uri);
    }

    printf("[DEBUG] pkgs_list_url=%s\n", res.pkgs_list_url);
    printf("[DEBUG] checksum_url=%s\n", res.checksum_url);

    res.pkgs_list_path = get_cache_path(state, "pkgs.list");
    if (!res.pkgs_list_path) {
        fprintf(stderr, "[ERROR] sync_package_list: failed to get cache path\n");
        return cleanup_sync_resources(&res, -1);
    }

    if (access(res.pkgs_list_path, F_OK) != 0) {
        printf("[INFO] pkgs.list not found locally, downloading...\n");
        needs_download = 1;
    } else {
        printf("[INFO] pkgs.list found locally, checking integrity...\n");

        res.remote_checksum = download_to_string(proto, res.checksum_url);
        if (!res.remote_checksum) {
            fprintf(stderr, "[WARNING] Failed to download checksum.hsh, forcing re-download\n");
            needs_download = 1;
        } else {
            char *p = res.remote_checksum;
            while (*p && (*p == ' ' || *p == '\n' || *p == '\r' || *p == '\t')) {
                p++;
            }
            char *end = p + strlen(p) - 1;
            while (end > p && (*end == ' ' || *end == '\n' || *end == '\r' || *end == '\t')) {
                *end = '\0';
                end--;
            }

            if (p != res.remote_checksum) {
                memmove(res.remote_checksum, p, strlen(p) + 1);
            }

            printf("[DEBUG] remote_checksum=%s\n", res.remote_checksum);

            res.local_checksum = calculate_sha256_file(res.pkgs_list_path);
            if (!res.local_checksum) {
                fprintf(stderr, "[ERROR] Failed to calculate local checksum\n");
                needs_download = 1;
            } else {
                printf("[DEBUG] local_checksum=%s\n", res.local_checksum);

                if (strcmp(res.local_checksum, res.remote_checksum) != 0) {
                    printf("[INFO] Checksums differ, re-downloading pkgs.list...\n");
                    needs_download = 1;
                } else {
                    printf("[INFO] Checksums match, pkgs.list is up to date\n");
                    return cleanup_sync_resources(&res, 0);
                }
            }
        }
    }

    if (needs_download) {
        printf("[INFO] Downloading pkgs.list...\n");
        if (download_to_file(proto, res.pkgs_list_url, res.pkgs_list_path) == 0) {
            printf("[INFO] Successfully downloaded pkgs.list\n");
            return cleanup_sync_resources(&res, 0);
        } else {
            fprintf(stderr, "[ERROR] Failed to download pkgs.list\n");
            return cleanup_sync_resources(&res, -1);
        }
    }

    return cleanup_sync_resources(&res, 0);
}
