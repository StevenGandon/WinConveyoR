#include "libwconr.h"
#include "pkg_downloader.h"
#include "file_utils.h"
#include "pkg_parsing.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

int install_package(const char *source_uri, const char *package_name)
{
    char *pkgs_list_path = NULL;
    char *register_path = NULL;
    char *checksum = NULL;
    char *register_url = NULL;
    char *register_content = NULL;
    char *variant_location = NULL;
    size_t uri_len, register_path_len;
    int rc;

    printf("[INFO] install_package: source_uri=%s package=%s\n", source_uri, package_name);

    if (!source_uri || !package_name) {
        fprintf(stderr, "[ERROR] install_package: source_uri or package_name is NULL\n");
        return -1;
    }

    if (curl_global_init(CURL_GLOBAL_DEFAULT) != 0) {
        fprintf(stderr, "[ERROR] install_package: curl_global_init failed\n");
        return -1;
    }

    pkgs_list_path = get_cache_path("pkgs.list");
    if (!pkgs_list_path) {
        fprintf(stderr, "[ERROR] install_package: failed to get cache path\n");
        curl_global_cleanup();
        return -1;
    }

    rc = find_package_in_list(pkgs_list_path, package_name, &register_path, &checksum);
    if (rc != 0) {
        free(pkgs_list_path);
        curl_global_cleanup();
        return -1;
    }

    uri_len = strlen(source_uri);
    register_path_len = strlen(register_path);
    register_url = malloc(uri_len + register_path_len + 1);
    if (!register_url) {
        fprintf(stderr, "[ERROR] install_package: malloc failed for register_url\n");
        free(pkgs_list_path); free(register_path); free(checksum);
        curl_global_cleanup();
        return -1;
    }

    if (source_uri[uri_len - 1] == '/' && register_path[0] == '/') {
        sprintf(register_url, "%s%s", source_uri, register_path + 1);
    } else if (source_uri[uri_len - 1] != '/' && register_path[0] != '/') {
        sprintf(register_url, "%s/%s", source_uri, register_path);
    } else {
        sprintf(register_url, "%s%s", source_uri, register_path);
    }

    printf("[DEBUG] install_package: register_url=%s\n", register_url);

    register_content = download_to_string(register_url);
    if (!register_content) {
        fprintf(stderr, "[ERROR] install_package: failed to download register file\n");
        free(pkgs_list_path); free(register_path); free(checksum); free(register_url);
        curl_global_cleanup();
        return -1;
    }

    rc = parse_first_location(register_content, &variant_location);
    if (rc != 0) {
        free(pkgs_list_path); free(register_path); free(checksum); free(register_url); free(register_content);
        curl_global_cleanup();
        return -1;
    }

    char *json_url = NULL;
    char *json_content = NULL;
    char *archive_address = NULL;
    char *archive_sha256 = NULL;
    size_t variant_location_len = strlen(variant_location);

    json_url = malloc(uri_len + variant_location_len + 1);
    if (!json_url) {
        fprintf(stderr, "[ERROR] install_package: malloc failed for json_url\n");
        free(pkgs_list_path); free(register_path); free(checksum); free(register_url); free(register_content); free(variant_location);
        curl_global_cleanup();
        return -1;
    }

    if (source_uri[uri_len - 1] == '/' && variant_location[0] == '/') {
        sprintf(json_url, "%s%s", source_uri, variant_location + 1);
    } else if (source_uri[uri_len - 1] != '/' && variant_location[0] != '/') {
        sprintf(json_url, "%s/%s", source_uri, variant_location);
    } else {
        sprintf(json_url, "%s%s", source_uri, variant_location);
    }

    printf("[DEBUG] install_package: json_url=%s\n", json_url);

    json_content = download_to_string(json_url);
    if (!json_content) {
        fprintf(stderr, "[ERROR] install_package: failed to download metadata JSON\n");
        free(pkgs_list_path); free(register_path); free(checksum); free(register_url); free(register_content); free(variant_location); free(json_url);
        curl_global_cleanup();
        return -1;
    }

    rc = json_extract_string(json_content, "address", &archive_address);
    if (rc != 0) {
        free(pkgs_list_path); free(register_path); free(checksum); free(register_url); free(register_content); free(variant_location); free(json_url); free(json_content);
        curl_global_cleanup();
        return -1;
    }

    rc = json_extract_string(json_content, "SHA256", &archive_sha256);
    if (rc != 0) {
        free(pkgs_list_path); free(register_path); free(checksum); free(register_url); free(register_content); free(variant_location); free(json_url); free(json_content); free(archive_address);
        curl_global_cleanup();
        return -1;
    }

    printf("[DEBUG] install_package: archive_address=%s\n", archive_address);
    printf("[DEBUG] install_package: archive_sha256=%s\n", archive_sha256);

    char *archive_url = NULL;
    char *archive_basename = NULL;
    char *archive_path = NULL;
    char *local_archive_sha256 = NULL;
    size_t archive_address_len = strlen(archive_address);

    archive_url = malloc(uri_len + archive_address_len + 1);
    if (!archive_url) {
        fprintf(stderr, "[ERROR] install_package: malloc failed for archive_url\n");
        free(pkgs_list_path); free(register_path); free(checksum); free(register_url); free(register_content); free(variant_location); free(json_url); free(json_content); free(archive_address); free(archive_sha256);
        curl_global_cleanup();
        return -1;
    }

    if (source_uri[uri_len - 1] == '/' && archive_address[0] == '/') {
        sprintf(archive_url, "%s%s", source_uri, archive_address + 1);
    } else if (source_uri[uri_len - 1] != '/' && archive_address[0] != '/') {
        sprintf(archive_url, "%s/%s", source_uri, archive_address);
    } else {
        sprintf(archive_url, "%s%s", source_uri, archive_address);
    }

    archive_basename = strrchr(archive_address, '/');
    archive_basename = archive_basename ? archive_basename + 1 : archive_address;

    archive_path = get_cache_path(archive_basename);
    if (!archive_path) {
        fprintf(stderr, "[ERROR] install_package: failed to get archive cache path\n");
        free(pkgs_list_path); free(register_path); free(checksum); free(register_url); free(register_content); free(variant_location); free(json_url); free(json_content); free(archive_address); free(archive_sha256); free(archive_url);
        curl_global_cleanup();
        return -1;
    }

    printf("[DEBUG] install_package: archive_url=%s\n", archive_url);
    printf("[DEBUG] install_package: archive_path=%s\n", archive_path);

    rc = download_to_file(archive_url, archive_path);
    if (rc != 0) {
        fprintf(stderr, "[ERROR] install_package: failed to download archive\n");
        free(pkgs_list_path); free(register_path); free(checksum); free(register_url); free(register_content); free(variant_location); free(json_url); free(json_content); free(archive_address); free(archive_sha256); free(archive_url); free(archive_path);
        curl_global_cleanup();
        return -1;
    }

    local_archive_sha256 = calculate_sha256_file(archive_path);
    if (!local_archive_sha256) {
        fprintf(stderr, "[ERROR] install_package: failed to compute SHA256 of downloaded archive\n");
        free(pkgs_list_path); free(register_path); free(checksum); free(register_url); free(register_content); free(variant_location); free(json_url); free(json_content); free(archive_address); free(archive_sha256); free(archive_url); free(archive_path);
        curl_global_cleanup();
        return -1;
    }

    if (strcmp(local_archive_sha256, archive_sha256) != 0) {
        fprintf(stderr, "[ERROR] install_package: SHA256 mismatch\n");
        fprintf(stderr, "  expected: %s\n", archive_sha256);
        fprintf(stderr, "  got:      %s\n", local_archive_sha256);
        free(pkgs_list_path); free(register_path); free(checksum); free(register_url); free(register_content); free(variant_location); free(json_url); free(json_content); free(archive_address); free(archive_sha256); free(archive_url); free(archive_path); free(local_archive_sha256);
        curl_global_cleanup();
        return -1;
    }

    printf("[INFO] install_package: archive downloaded and SHA256 verified ok (%s)\n", archive_path);
    printf("[INFO] install_package: stub - would now extract and run install wizard\n");

    free(pkgs_list_path);
    free(register_path);
    free(checksum);
    free(register_url);
    free(register_content);
    free(variant_location);
    free(json_url);
    free(json_content);
    free(archive_address);
    free(archive_sha256);
    free(archive_url);
    free(archive_path);
    free(local_archive_sha256);
    curl_global_cleanup();
    return 0;
}
