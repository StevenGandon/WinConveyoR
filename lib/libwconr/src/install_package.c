#include "libwconr.h"
#include "wcr_event_internal.h"
#include "pkg_downloader.h"
#include "file_utils.h"
#include "pkg_parsing.h"
#include "wcr_client.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

static int fetch_register(const wcr_state *state, protocol_type proto, const char *source_uri, const char *register_path, char **out_content)
{
    char *url = NULL;

    *out_content = NULL;

    url = build_url(source_uri, register_path);
    if (!url) {
        return -1;
    }

    wcr_emit(state, WCR_EVENT_DEBUG, "[DEBUG] fetch_register: url=%s", url);

    *out_content = download_to_string(proto, url);
    free(url);

    if (!*out_content) {
        wcr_emit(state, WCR_EVENT_ERROR, "[ERROR] fetch_register: download failed");
        return -1;
    }

    return 0;
}

static int fetch_metadata(const wcr_state *state, protocol_type proto, const char *source_uri, const char *variant_location,
                          char **out_address, char **out_sha256,
                          char ***out_depends, size_t *out_depends_count)
{
    char *url = NULL;
    char *content = NULL;
    int rc;

    *out_address = NULL;
    *out_sha256 = NULL;
    *out_depends = NULL;
    *out_depends_count = 0;

    url = build_url(source_uri, variant_location);
    if (!url) {
        return -1;
    }

    wcr_emit(state, WCR_EVENT_DEBUG, "[DEBUG] fetch_metadata: url=%s", url);

    content = download_to_string(proto, url);
    free(url);

    if (!content) {
        wcr_emit(state, WCR_EVENT_ERROR, "[ERROR] fetch_metadata: download failed");
        return -1;
    }

    rc = json_extract_string(content, "address", out_address);
    if (rc != 0) {
        free(content);
        return -1;
    }

    rc = json_extract_string(content, "SHA256", out_sha256);
    if (rc != 0) {
        free(*out_address);
        *out_address = NULL;
        free(content);
        return -1;
    }

    rc = json_extract_string_array(content, "depends", out_depends, out_depends_count);
    free(content);
    if (rc != 0) {
        free(*out_address);
        *out_address = NULL;
        free(*out_sha256);
        *out_sha256 = NULL;
        return -1;
    }

    wcr_emit(state, WCR_EVENT_DEBUG, "[DEBUG] fetch_metadata: address=%s sha256=%s depends_count=%zu",
             *out_address, *out_sha256, *out_depends_count);
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

    wcr_emit(state, WCR_EVENT_DEBUG, "[DEBUG] fetch_and_verify_archive: url=%s path=%s", url, path);

    rc = download_to_file(proto, url, path);
    free(url);
    if (rc != 0) {
        wcr_emit(state, WCR_EVENT_ERROR, "[ERROR] fetch_and_verify_archive: download failed");
        free(path);
        return -1;
    }

    local_sha256 = calculate_sha256_file(path);
    if (!local_sha256) {
        wcr_emit(state, WCR_EVENT_ERROR, "[ERROR] fetch_and_verify_archive: SHA256 computation failed");
        free(path);
        return -1;
    }

    if (strcmp(local_sha256, expected_sha256) != 0) {
        wcr_emit(state, WCR_EVENT_ERROR, "[ERROR] fetch_and_verify_archive: SHA256 mismatch (expected: %s, got: %s)", expected_sha256, local_sha256);
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

static int extract_location_hash(const char *listing, char *out, size_t out_sz)
{
    const char *p = listing;
    int field = 0;
    size_t i = 0;

    while (*p && *p != '\r' && *p != '\n') {
        if (*p == ' ') {
            field++;
            i = 0;
        } else if (field == 3 && i < out_sz - 1) {
            out[i++] = *p;
        }
        p++;
    }
    out[i] = '\0';

    return (i > 0) ? 0 : -1;
}

static int wcr_fetch_metadata(const wcr_state *state, wcr_conn *conn, const char *package_name,
                              char **out_address, char **out_sha256,
                              char ***out_depends, size_t *out_depends_count)
{
    char *listing;
    char *metadata;
    char location_hash[128] = {0};

    *out_address = NULL;
    *out_sha256 = NULL;
    *out_depends = NULL;
    *out_depends_count = 0;

    listing = wcr_get_package_listing(conn, package_name);
    if (!listing) {
        wcr_emit(state, WCR_EVENT_ERROR, "[ERROR] wcr_fetch_metadata: get_package_listing failed for '%s'", package_name);
        return -1;
    }

    wcr_emit(state, WCR_EVENT_DEBUG, "[DEBUG] wcr_fetch_metadata: package_listing=\n%s", listing);

    if (extract_location_hash(listing, location_hash, sizeof(location_hash)) != 0) {
        wcr_emit(state, WCR_EVENT_ERROR, "[ERROR] wcr_fetch_metadata: could not extract location_hash");
        free(listing);
        return -1;
    }
    free(listing);

    wcr_emit(state, WCR_EVENT_DEBUG, "[DEBUG] wcr_fetch_metadata: using location_hash=%s", location_hash);

    metadata = wcr_get_package_metadata(conn, package_name, location_hash);
    if (!metadata) {
        wcr_emit(state, WCR_EVENT_ERROR, "[ERROR] wcr_fetch_metadata: get_package_metadata failed");
        return -1;
    }

    wcr_emit(state, WCR_EVENT_DEBUG, "[DEBUG] wcr_fetch_metadata: metadata=%s", metadata);

    if (json_extract_string(metadata, "address", out_address) != 0) {
        wcr_emit(state, WCR_EVENT_ERROR, "[ERROR] wcr_fetch_metadata: cannot extract 'address'");
        free(metadata);
        return -1;
    }

    if (json_extract_string(metadata, "SHA256", out_sha256) != 0) {
        wcr_emit(state, WCR_EVENT_ERROR, "[ERROR] wcr_fetch_metadata: cannot extract 'SHA256'");
        free(*out_address);
        *out_address = NULL;
        free(metadata);
        return -1;
    }

    if (json_extract_string_array(metadata, "depends", out_depends, out_depends_count) != 0) {
        free(*out_address);
        *out_address = NULL;
        free(*out_sha256);
        *out_sha256 = NULL;
        free(metadata);
        return -1;
    }

    free(metadata);
    return 0;
}

#define WCR_MAX_DEP_DEPTH 32

typedef struct {
    char *names[WCR_MAX_DEP_DEPTH];
    size_t count;
} wcr_install_ctx;

static void install_ctx_cleanup(wcr_install_ctx *ctx)
{
    size_t i;

    for (i = 0; i < ctx->count; i++)
        free(ctx->names[i]);
}

static int install_ctx_has(const wcr_install_ctx *ctx, const char *name)
{
    size_t i;

    for (i = 0; i < ctx->count; i++) {
        if (strcmp(ctx->names[i], name) == 0)
            return 1;
    }
    return 0;
}

static int install_ctx_add(const wcr_state *state, wcr_install_ctx *ctx, const char *name)
{
    if (ctx->count >= WCR_MAX_DEP_DEPTH) {
        wcr_emit(state, WCR_EVENT_ERROR,
                 "[ERROR] install: dependency depth limit (%d) reached", WCR_MAX_DEP_DEPTH);
        return -1;
    }
    ctx->names[ctx->count] = strdup(name);
    if (!ctx->names[ctx->count])
        return -1;
    ctx->count++;
    return 0;
}

static int install_wcr(const wcr_state *state, const char *source_uri, const char *package_name)
{
    char host[256];
    int port;
    wcr_conn *conn;
    char *address = NULL;
    char *sha256 = NULL;
    char **depends = NULL;
    size_t depends_count = 0;
    size_t i;
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

    rc = wcr_fetch_metadata(state, conn, package_name, &address, &sha256, &depends, &depends_count);
    wcr_close(conn);

    if (rc != 0)
        return -1;

    wcr_emit(state, WCR_EVENT_INFO, "[INFO] install_wcr: package=%s address=%s SHA256=%s", package_name, address, sha256);

    if (depends_count > 0) {
        wcr_emit(state, WCR_EVENT_INFO, "[INFO] install_wcr: package has %zu dependencies:", depends_count);
        for (i = 0; i < depends_count; i++)
            wcr_emit(state, WCR_EVENT_INFO, "[INFO] install_wcr:   - %s", depends[i]);
    }

    wcr_emit(state, WCR_EVENT_INFO, "[INFO] install_wcr: metadata retrieved successfully via WCR protocol");
    wcr_emit(state, WCR_EVENT_INFO, "[INFO] install_wcr: archive download via WCR not supported yet (needs protocol extension)");

    free(address);
    free(sha256);
    free_string_array(depends, depends_count);
    return 0;
}

static int install_http_with_deps(const wcr_state *state, protocol_type proto, const char *source_uri,
                                  const char *package_name, wcr_install_ctx *ctx)
{
    char *pkgs_list_path;
    char *register_path = NULL;
    char *checksum = NULL;
    char *register_content = NULL;
    char *variant_location = NULL;
    char *archive_address = NULL;
    char *archive_sha256 = NULL;
    char *archive_path = NULL;
    char **depends = NULL;
    size_t depends_count = 0;
    size_t i;

    if (install_ctx_has(ctx, package_name)) {
        wcr_emit(state, WCR_EVENT_DEBUG, "[DEBUG] install: skipping already-visited '%s'", package_name);
        return 0;
    }

    if (install_ctx_add(state, ctx, package_name) != 0)
        return -1;

    wcr_emit(state, WCR_EVENT_INFO, "[INFO] install: resolving '%s'", package_name);

    pkgs_list_path = get_cache_path(state, "pkgs.list");
    if (!pkgs_list_path)
        return -1;

    if (find_package_in_list(pkgs_list_path, package_name, &register_path, &checksum) != 0) {
        free(pkgs_list_path);
        return -1;
    }
    free(pkgs_list_path);
    free(checksum);

    if (fetch_register(state, proto, source_uri, register_path, &register_content) != 0) {
        free(register_path);
        return -1;
    }
    free(register_path);

    if (parse_first_location(register_content, &variant_location) != 0) {
        free(register_content);
        return -1;
    }
    free(register_content);

    if (fetch_metadata(state, proto, source_uri, variant_location,
                       &archive_address, &archive_sha256, &depends, &depends_count) != 0) {
        free(variant_location);
        return -1;
    }
    free(variant_location);

    for (i = 0; i < depends_count; i++) {
        wcr_emit(state, WCR_EVENT_INFO, "[INFO] install: dependency '%s' required by '%s'",
                 depends[i], package_name);
        if (install_http_with_deps(state, proto, source_uri, depends[i], ctx) != 0) {
            wcr_emit(state, WCR_EVENT_ERROR, "[ERROR] install: failed to install dependency '%s'",
                     depends[i]);
            free_string_array(depends, depends_count);
            free(archive_address);
            free(archive_sha256);
            return -1;
        }
    }
    free_string_array(depends, depends_count);

    if (fetch_and_verify_archive(state, proto, source_uri, archive_address, archive_sha256, &archive_path) != 0) {
        free(archive_address);
        free(archive_sha256);
        return -1;
    }
    free(archive_address);
    free(archive_sha256);

    wcr_emit(state, WCR_EVENT_INFO, "[INFO] install: archive downloaded and SHA256 verified ok (%s)", archive_path);
    wcr_emit(state, WCR_EVENT_INFO, "[INFO] install: stub - would now extract and run install wizard");

    free(archive_path);
    return 0;
}

static int install_http(const wcr_state *state, protocol_type proto, const char *source_uri, const char *package_name)
{
    wcr_install_ctx ctx = {0};
    int rc;

    rc = install_http_with_deps(state, proto, source_uri, package_name, &ctx);
    install_ctx_cleanup(&ctx);
    return rc;
}

int install_package(const wcr_state *state, protocol_type proto, const char *source_uri, const char *package_name)
{
    int rc;

    wcr_emit(state, WCR_EVENT_INFO, "[INFO] install_package: source_uri=%s package=%s proto=%d", source_uri, package_name, proto);

    if (!state || !source_uri || !package_name) {
        wcr_emit(NULL, WCR_EVENT_ERROR, "[ERROR] install_package: state, source_uri or package_name is NULL");
        return -1;
    }

    if (proto == PROT_WCR)
        return install_wcr(state, source_uri, package_name);

    if (curl_global_init(CURL_GLOBAL_DEFAULT) != 0) {
        wcr_emit(state, WCR_EVENT_ERROR, "[ERROR] install_package: curl_global_init failed");
        return -1;
    }

    rc = install_http(state, proto, source_uri, package_name);
    curl_global_cleanup();
    return rc;
}
