#include "libwconr.h"
#include "pkg_downloader.h"
#include "file_utils.h"
#include "pkg_parsing.h"
#include "wcr_client.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

static int fetch_register(protocol_type proto, const char *source_uri, const char *register_path, char **out_content)
{
    char *url = NULL;

    *out_content = NULL;

    url = build_url(source_uri, register_path);
    if (!url) {
        return -1;
    }

    printf("[DEBUG] fetch_register: url=%s\n", url);

    *out_content = download_to_string(proto, url);
    free(url);

    if (!*out_content) {
        fprintf(stderr, "[ERROR] fetch_register: download failed\n");
        return -1;
    }

    return 0;
}

static int fetch_metadata(protocol_type proto, const char *source_uri, const char *variant_location,
                          char **out_address, char **out_sha256)
{
    char *url = NULL;
    char *content = NULL;
    int rc;

    *out_address = NULL;
    *out_sha256 = NULL;

    url = build_url(source_uri, variant_location);
    if (!url) {
        return -1;
    }

    printf("[DEBUG] fetch_metadata: url=%s\n", url);

    content = download_to_string(proto, url);
    free(url);

    if (!content) {
        fprintf(stderr, "[ERROR] fetch_metadata: download failed\n");
        return -1;
    }

    rc = json_extract_string(content, "address", out_address);
    if (rc != 0) {
        free(content);
        return -1;
    }

    rc = json_extract_string(content, "SHA256", out_sha256);
    free(content);
    if (rc != 0) {
        free(*out_address);
        *out_address = NULL;
        return -1;
    }

    printf("[DEBUG] fetch_metadata: address=%s sha256=%s\n", *out_address, *out_sha256);
    return 0;
}

static int fetch_and_verify_archive(const wcr_state *state, protocol_type proto, const char *source_uri,
                                     const char *archive_address, const char *expected_sha256,
                                     char **out_path)
{
    char *url = NULL;
    char *path = NULL;
    char *local_sha256 = NULL;
    const char *basename;
    int rc;

    *out_path = NULL;

    url = build_url(source_uri, archive_address);
    if (!url) {
        return -1;
    }

    basename = strrchr(archive_address, '/');
    basename = basename ? basename + 1 : archive_address;

    path = get_cache_path(state, basename);
    if (!path) {
        free(url);
        return -1;
    }

    printf("[DEBUG] fetch_and_verify_archive: url=%s path=%s\n", url, path);

    rc = download_to_file(proto, url, path);
    free(url);
    if (rc != 0) {
        fprintf(stderr, "[ERROR] fetch_and_verify_archive: download failed\n");
        free(path);
        return -1;
    }

    local_sha256 = calculate_sha256_file(path);
    if (!local_sha256) {
        fprintf(stderr, "[ERROR] fetch_and_verify_archive: SHA256 computation failed\n");
        free(path);
        return -1;
    }

    if (strcmp(local_sha256, expected_sha256) != 0) {
        fprintf(stderr, "[ERROR] fetch_and_verify_archive: SHA256 mismatch\n");
        fprintf(stderr, "  expected: %s\n", expected_sha256);
        fprintf(stderr, "  got:      %s\n", local_sha256);
        free(path);
        free(local_sha256);
        return -1;
    }

    free(local_sha256);
    *out_path = path;
    return 0;
}

static int parse_host_port(const char *source_uri, char *host, size_t host_sz, int *port)
{
    const char *colon = strrchr(source_uri, ':');
    size_t hlen;

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

static int install_wcr(const wcr_state *state, const char *source_uri, const char *package_name)
{
    char host[256];
    int port;
    wcr_conn *conn = NULL;
    char *listing = NULL;
    char *metadata = NULL;
    char *address = NULL;
    char *sha256 = NULL;
    char *archive_path = NULL;
    char location_hash[128] = {0};
    const char *access_key;
    int rc = -1;

    (void)state;

    parse_host_port(source_uri, host, sizeof(host), &port);

    conn = wcr_open(host, port);
    if (!conn) return -1;

    access_key = getenv("WCR_ACCESS");
    if (wcr_auth(conn, access_key ? access_key : "") != 0) {
        goto cleanup;
    }

    listing = wcr_get_package_listing(conn, package_name);
    if (!listing) {
        fprintf(stderr, "[ERROR] install_wcr: get_package_listing failed for '%s'\n", package_name);
        goto cleanup;
    }

    printf("[DEBUG] install_wcr: package_listing=\n%s\n", listing);

    {
        const char *p = listing;
        int field = 0;
        size_t i = 0;

        while (*p && *p != '\r' && *p != '\n') {
            if (*p == ' ') {
                field++;
                i = 0;
            } else if (field == 3 && i < sizeof(location_hash) - 1) {
                location_hash[i++] = *p;
            }
            p++;
        }
    }

    if (location_hash[0] == '\0') {
        fprintf(stderr, "[ERROR] install_wcr: could not extract location_hash from listing\n");
        goto cleanup;
    }

    printf("[DEBUG] install_wcr: using location_hash=%s\n", location_hash);

    metadata = wcr_get_package_metadata(conn, package_name, location_hash);
    if (!metadata) {
        fprintf(stderr, "[ERROR] install_wcr: get_package_metadata failed\n");
        goto cleanup;
    }

    printf("[DEBUG] install_wcr: metadata=%s\n", metadata);

    if (json_extract_string(metadata, "address", &address) != 0) {
        fprintf(stderr, "[ERROR] install_wcr: cannot extract 'address' from metadata\n");
        goto cleanup;
    }

    if (json_extract_string(metadata, "SHA256", &sha256) != 0) {
        fprintf(stderr, "[ERROR] install_wcr: cannot extract 'SHA256' from metadata\n");
        goto cleanup;
    }

    printf("[INFO] install_wcr: package=%s address=%s SHA256=%s\n", package_name, address, sha256);
    printf("[INFO] install_wcr: metadata retrieved successfully via WCR protocol\n");
    printf("[INFO] install_wcr: archive download via WCR not supported yet (needs protocol extension)\n");

    rc = 0;

cleanup:
    free(listing);
    free(metadata);
    free(address);
    free(sha256);
    free(archive_path);
    wcr_close(conn);
    return rc;
}

int install_package(const wcr_state *state, protocol_type proto, const char *source_uri, const char *package_name)
{
    char *pkgs_list_path = NULL;
    char *register_path = NULL;
    char *checksum = NULL;
    char *register_content = NULL;
    char *variant_location = NULL;
    char *archive_address = NULL;
    char *archive_sha256 = NULL;
    char *archive_path = NULL;
    int rc = -1;

    printf("[INFO] install_package: source_uri=%s package=%s proto=%d\n", source_uri, package_name, proto);

    if (!state || !source_uri || !package_name) {
        fprintf(stderr, "[ERROR] install_package: state, source_uri or package_name is NULL\n");
        return -1;
    }

    if (proto == PROT_WCR) {
        return install_wcr(state, source_uri, package_name);
    }

    if (curl_global_init(CURL_GLOBAL_DEFAULT) != 0) {
        fprintf(stderr, "[ERROR] install_package: curl_global_init failed\n");
        return -1;
    }

    pkgs_list_path = get_cache_path(state, "pkgs.list");
    if (!pkgs_list_path) {
        goto cleanup;
    }

    if (find_package_in_list(pkgs_list_path, package_name, &register_path, &checksum) != 0) {
        goto cleanup;
    }

    if (fetch_register(proto, source_uri, register_path, &register_content) != 0) {
        goto cleanup;
    }

    if (parse_first_location(register_content, &variant_location) != 0) {
        goto cleanup;
    }

    if (fetch_metadata(proto, source_uri, variant_location, &archive_address, &archive_sha256) != 0) {
        goto cleanup;
    }

    if (fetch_and_verify_archive(state, proto, source_uri, archive_address, archive_sha256, &archive_path) != 0) {
        goto cleanup;
    }

    printf("[INFO] install_package: archive downloaded and SHA256 verified ok (%s)\n", archive_path);
    printf("[INFO] install_package: stub - would now extract and run install wizard\n");

    rc = 0;

cleanup:
    free(pkgs_list_path);
    free(register_path);
    free(checksum);
    free(register_content);
    free(variant_location);
    free(archive_address);
    free(archive_sha256);
    free(archive_path);
    curl_global_cleanup();
    return rc;
}
