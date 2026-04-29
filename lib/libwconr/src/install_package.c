#include "libwconr.h"
#include "pkg_downloader.h"
#include "file_utils.h"
#include "pkg_parsing.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

static int fetch_register(const char *source_uri, const char *register_path, char **out_content)
{
    char *url = NULL;

    *out_content = NULL;

    url = build_url(source_uri, register_path);
    if (!url) {
        return -1;
    }

    printf("[DEBUG] fetch_register: url=%s\n", url);

    *out_content = download_to_string(url);
    free(url);

    if (!*out_content) {
        fprintf(stderr, "[ERROR] fetch_register: download failed\n");
        return -1;
    }

    return 0;
}

static int fetch_metadata(const char *source_uri, const char *variant_location,
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

    content = download_to_string(url);
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

static int fetch_and_verify_archive(const wcr_state *state, const char *source_uri,
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

    rc = download_to_file(url, path);
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

int install_package(const wcr_state *state, const char *source_uri, const char *package_name)
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

    printf("[INFO] install_package: source_uri=%s package=%s\n", source_uri, package_name);

    if (!state || !source_uri || !package_name) {
        fprintf(stderr, "[ERROR] install_package: state, source_uri or package_name is NULL\n");
        return -1;
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

    if (fetch_register(source_uri, register_path, &register_content) != 0) {
        goto cleanup;
    }

    if (parse_first_location(register_content, &variant_location) != 0) {
        goto cleanup;
    }

    if (fetch_metadata(source_uri, variant_location, &archive_address, &archive_sha256) != 0) {
        goto cleanup;
    }

    if (fetch_and_verify_archive(state, source_uri, archive_address, archive_sha256, &archive_path) != 0) {
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
