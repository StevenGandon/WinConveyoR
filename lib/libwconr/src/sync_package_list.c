#include "libwconr.h"
#include "pkg_downloader.h"
#include "file_utils.h"

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

int sync_package_list(const char *source_uri)
{
    struct sync_resources_s res = {NULL, NULL, NULL, NULL, NULL};
    size_t uri_len;
    int needs_download = 0;

    printf("[INFO] sync_package_list: source_uri=%s\n", source_uri);

    if (!source_uri) {
        fprintf(stderr, "[ERROR] sync_package_list: source_uri is NULL\n");
        return -1;
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

    res.pkgs_list_path = get_cache_path("pkgs.list");
    if (!res.pkgs_list_path) {
        fprintf(stderr, "[ERROR] sync_package_list: failed to get cache path\n");
        return cleanup_sync_resources(&res, -1);
    }

    if (access(res.pkgs_list_path, F_OK) != 0) {
        printf("[INFO] pkgs.list not found locally, downloading...\n");
        needs_download = 1;
    } else {
        printf("[INFO] pkgs.list found locally, checking integrity...\n");

        res.remote_checksum = download_to_string(res.checksum_url);
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
        if (download_to_file(res.pkgs_list_url, res.pkgs_list_path) == 0) {
            printf("[INFO] Successfully downloaded pkgs.list\n");
            return cleanup_sync_resources(&res, 0);
        } else {
            fprintf(stderr, "[ERROR] Failed to download pkgs.list\n");
            return cleanup_sync_resources(&res, -1);
        }
    }

    return cleanup_sync_resources(&res, 0);
}
