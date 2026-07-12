#include "libwconr.h"
#include "wcr_event_internal.h"
#include "pkg_downloader.h"
#include "file_utils.h"
#include "pkg_parsing.h"
#include "pkg_registry.h"
#include "wcr_client.h"
#include "pkg_specifier.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

static int wcr_connect_source(const wcr_state *state, const wcr_source *src,
                              const char *host, int port, wcr_conn **out)
{
    wcr_conn *conn;
    const char *access_key;

    conn = wcr_open(host, port);
    if (!conn)
        return -1;

    {
        const char *pubkey_path = (src && src->server_pubkey_path)
            ? src->server_pubkey_path : getenv("WCR_PUBKEY");

        if (pubkey_path) {
            char *pem = read_file_text(pubkey_path);

            if (!pem) {
                wcr_emit(state, WCR_EVENT_ERROR,
                         "[ERROR] wcr_connect_source: cannot read pubkey %s",
                         pubkey_path);
                wcr_close(conn);
                return -1;
            }
            if (wcr_handshake(conn, pem) != 0) {
                wcr_emit(state, WCR_EVENT_ERROR,
                         "[ERROR] wcr_connect_source: handshake failed");
                free(pem);
                wcr_close(conn);
                return -1;
            }
            free(pem);
            wcr_emit(state, WCR_EVENT_INFO,
                     "[INFO] wcr_connect_source: RSA encryption established");
        }
    }

    access_key = (src && src->access_key) ? src->access_key : getenv("WCR_ACCESS");
    if (wcr_auth(conn, access_key ? access_key : "") != 0) {
        wcr_close(conn);
        return -1;
    }

    *out = conn;
    return 0;
}

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

static int extract_line_field(const char *line, int field_idx, char *out, size_t out_sz)
{
    const char *p = line;
    int field = 0;
    size_t i = 0;

    while (*p && *p != '\r' && *p != '\n') {
        if (*p == ' ') {
            field++;
            i = 0;
        } else if (field == field_idx && i < out_sz - 1) {
            out[i++] = *p;
        }
        p++;
    }
    out[i] = '\0';

    return (i > 0) ? 0 : -1;
}

static int strcasematch(const char *filter, const char *value)
{
    if (!filter)
        return 1;
#ifdef _WIN32
    return (_stricmp(filter, value) == 0);
#else
    return (strcasecmp(filter, value) == 0);
#endif
}

static int select_variant_wcr(const char *listing,
                              const struct pkg_specifier *spec,
                              char *out_hash, size_t hash_sz)
{
    const char *line = listing;
    char ver[128], arch[128], mach[128], hash[256];
    const char *best_line = NULL;

    out_hash[0] = '\0';

    while (line && *line) {
        const char *next = strstr(line, "\r\n");

        if (!next)
            next = strchr(line, '\n');

        if (*line == '\r' || *line == '\n' || *line == '\0') {
            line = next ? next + (next[0] == '\r' ? 2 : 1) : NULL;
            continue;
        }

        if (extract_line_field(line, 0, ver, sizeof(ver)) != 0 ||
            extract_line_field(line, 1, arch, sizeof(arch)) != 0 ||
            extract_line_field(line, 2, mach, sizeof(mach)) != 0 ||
            extract_line_field(line, 3, hash, sizeof(hash)) != 0) {
            line = next ? next + (next[0] == '\r' ? 2 : 1) : NULL;
            continue;
        }

        if (strcasematch(spec->version, ver) &&
            strcasematch(spec->arch, arch) &&
            strcasematch(spec->machine, mach)) {
            size_t hlen = strlen(hash);

            if (hlen >= hash_sz) hlen = hash_sz - 1;
            memcpy(out_hash, hash, hlen);
            out_hash[hlen] = '\0';
            return 0;
        }

        if (!best_line)
            best_line = line;

        line = next ? next + (next[0] == '\r' ? 2 : 1) : NULL;
    }

    if (!spec->version && !spec->arch && !spec->machine && best_line) {
        extract_line_field(best_line, 3, out_hash, hash_sz);
        return (out_hash[0] != '\0') ? 0 : -1;
    }

    return -1;
}

static int wcr_fetch_metadata(const wcr_state *state, wcr_conn *conn,
                              const struct pkg_specifier *spec,
                              char **out_address, char **out_sha256,
                              char *out_location_hash, size_t location_hash_sz,
                              char ***out_depends, size_t *out_depends_count,
                              char **out_version)
{
    char *listing;
    char *metadata;

    *out_address = NULL;
    *out_sha256 = NULL;
    *out_depends = NULL;
    *out_depends_count = 0;
    *out_version = NULL;
    out_location_hash[0] = '\0';

    listing = wcr_get_package_listing(conn, spec->name);
    if (!listing) {
        wcr_emit(state, WCR_EVENT_ERROR, "[ERROR] wcr_fetch_metadata: get_package_listing failed for '%s'", spec->name);
        return -1;
    }

    wcr_emit(state, WCR_EVENT_DEBUG, "[DEBUG] wcr_fetch_metadata: package_listing=\n%s", listing);

    if (select_variant_wcr(listing, spec, out_location_hash, location_hash_sz) != 0) {
        wcr_emit(state, WCR_EVENT_ERROR, "[ERROR] wcr_fetch_metadata: no matching variant for '%s'", spec->name);
        free(listing);
        return -1;
    }
    free(listing);

    wcr_emit(state, WCR_EVENT_DEBUG, "[DEBUG] wcr_fetch_metadata: using location_hash=%s", out_location_hash);

    metadata = wcr_get_package_metadata(conn, spec->name, out_location_hash);
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

    json_extract_string(metadata, "version", out_version);

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

static int install_wcr_single(const wcr_state *state, const char *source_uri,
                              const char *package_name, wcr_install_ctx *ctx,
                              int is_dep)
{
    char host[256];
    int port;
    wcr_conn *conn;
    char *address = NULL;
    char *sha256_expected = NULL;
    char *sha256_local = NULL;
    char *local_path = NULL;
    char *version = NULL;
    char **depends = NULL;
    char location_hash[128] = {0};
    size_t depends_count = 0;
    size_t i;
    const wcr_source *src;
    const char *basename;
    struct pkg_specifier spec;

    if (pkg_specifier_parse(package_name, &spec) != 0) {
        wcr_emit(state, WCR_EVENT_ERROR, "[ERROR] install_wcr: invalid package specifier '%s'", package_name);
        return -1;
    }

    if (install_ctx_has(ctx, spec.name)) {
        wcr_emit(state, WCR_EVENT_DEBUG, "[DEBUG] install_wcr: skipping already-visited '%s'", spec.name);
        pkg_specifier_free(&spec);
        return 0;
    }

    if (install_ctx_add(state, ctx, spec.name) != 0) {
        pkg_specifier_free(&spec);
        return -1;
    }

    wcr_emit(state, WCR_EVENT_INFO, "[INFO] install_wcr: resolving '%s'", package_name);

    parse_host_port(source_uri, host, sizeof(host), &port);
    src = wcr_state_find_source(state, source_uri);

    if (wcr_connect_source(state, src, host, port, &conn) != 0) {
        pkg_specifier_free(&spec);
        return -1;
    }

    if (wcr_fetch_metadata(state, conn, &spec, &address, &sha256_expected,
                           location_hash, sizeof(location_hash),
                           &depends, &depends_count, &version) != 0) {
        wcr_close(conn);
        pkg_specifier_free(&spec);
        return -1;
    }

    for (i = 0; i < depends_count; i++) {
        wcr_emit(state, WCR_EVENT_INFO, "[INFO] install_wcr: dependency '%s' required by '%s'",
                 depends[i], spec.name);
        wcr_close(conn);
        if (install_wcr_single(state, source_uri, depends[i], ctx, 1) != 0) {
            wcr_emit(state, WCR_EVENT_ERROR, "[ERROR] install_wcr: failed to install dependency '%s'", depends[i]);
            free(address);
            free(sha256_expected);
            free_string_array(depends, depends_count);
            pkg_specifier_free(&spec);
            return -1;
        }
        if (wcr_connect_source(state, src, host, port, &conn) != 0) {
            free(address);
            free(sha256_expected);
            free_string_array(depends, depends_count);
            pkg_specifier_free(&spec);
            return -1;
        }
    }
    free_string_array(depends, depends_count);

    basename = strrchr(address, '/');
    basename = basename ? basename + 1 : address;

    local_path = get_cache_path(state, basename);
    if (!local_path) {
        wcr_close(conn);
        free(address);
        free(sha256_expected);
        pkg_specifier_free(&spec);
        return -1;
    }

    wcr_emit(state, WCR_EVENT_INFO, "[INFO] install_wcr: downloading %s", spec.name);

    if (wcr_download_file(conn, spec.name, location_hash, local_path) != 0) {
        wcr_close(conn);
        free(address);
        free(sha256_expected);
        free(local_path);
        pkg_specifier_free(&spec);
        return -1;
    }
    wcr_close(conn);
    free(address);

    sha256_local = calculate_sha256_file(local_path);
    if (!sha256_local || strcmp(sha256_local, sha256_expected) != 0) {
        wcr_emit(state, WCR_EVENT_ERROR, "[ERROR] install_wcr: SHA256 mismatch (expected=%s got=%s)",
                 sha256_expected, sha256_local ? sha256_local : "null");
        free(sha256_expected);
        free(sha256_local);
        free(local_path);
        pkg_specifier_free(&spec);
        return -1;
    }
    free(sha256_expected);
    free(sha256_local);

    wcr_emit(state, WCR_EVENT_INFO, "[INFO] install_wcr: SHA256 verified ok");

    if (extract_archive(state, local_path, spec.name) != 0) {
        free(local_path);
        pkg_specifier_free(&spec);
        return -1;
    }
    free(local_path);

    record_installed(state, spec.name, version ? version : "unknown", is_dep);
    free(version);

    wcr_emit(state, WCR_EVENT_INFO, "[INFO] install_wcr: '%s' installed successfully", spec.name);
    pkg_specifier_free(&spec);
    return 0;
}

static int install_wcr(const wcr_state *state, const char *source_uri, const char *package_name)
{
    wcr_install_ctx ctx = {0};
    int rc;

    rc = install_wcr_single(state, source_uri, package_name, &ctx, 0);
    install_ctx_cleanup(&ctx);
    return rc;
}

static int install_http_with_deps(const wcr_state *state, protocol_type proto, const char *source_uri,
                                  const char *package_name, wcr_install_ctx *ctx,
                                  int is_dep)
{
    char *pkgs_list_path;
    char *register_path = NULL;
    char *checksum = NULL;
    char *version = NULL;
    char *register_content = NULL;
    char *variant_location = NULL;
    char *archive_address = NULL;
    char *archive_sha256 = NULL;
    char *archive_path = NULL;
    char **depends = NULL;
    size_t depends_count = 0;
    size_t i;
    struct pkg_specifier spec;

    if (pkg_specifier_parse(package_name, &spec) != 0) {
        wcr_emit(state, WCR_EVENT_ERROR, "[ERROR] install: invalid package specifier '%s'", package_name);
        return -1;
    }

    if (install_ctx_has(ctx, spec.name)) {
        wcr_emit(state, WCR_EVENT_DEBUG, "[DEBUG] install: skipping already-visited '%s'", spec.name);
        pkg_specifier_free(&spec);
        return 0;
    }

    if (install_ctx_add(state, ctx, spec.name) != 0) {
        pkg_specifier_free(&spec);
        return -1;
    }

    wcr_emit(state, WCR_EVENT_INFO, "[INFO] install: resolving '%s'", package_name);

    pkgs_list_path = get_cache_path(state, "pkgs.list");
    if (!pkgs_list_path) {
        pkg_specifier_free(&spec);
        return -1;
    }

    if (find_package_in_list(pkgs_list_path, spec.name, &register_path, &checksum, &version) != 0) {
        free(pkgs_list_path);
        pkg_specifier_free(&spec);
        return -1;
    }
    free(pkgs_list_path);
    free(checksum);

    if (fetch_register(state, proto, source_uri, register_path, &register_content) != 0) {
        free(register_path);
        free(version);
        pkg_specifier_free(&spec);
        return -1;
    }
    free(register_path);

    if (select_variant_register(register_content, &spec, &variant_location) != 0) {
        wcr_emit(state, WCR_EVENT_ERROR, "[ERROR] install: no matching variant for '%s'", package_name);
        free(register_content);
        free(version);
        pkg_specifier_free(&spec);
        return -1;
    }
    free(register_content);

    if (fetch_metadata(state, proto, source_uri, variant_location,
                       &archive_address, &archive_sha256, &depends, &depends_count) != 0) {
        free(variant_location);
        free(version);
        pkg_specifier_free(&spec);
        return -1;
    }
    free(variant_location);

    for (i = 0; i < depends_count; i++) {
        wcr_emit(state, WCR_EVENT_INFO, "[INFO] install: dependency '%s' required by '%s'",
                 depends[i], spec.name);
        if (install_http_with_deps(state, proto, source_uri, depends[i], ctx, 1) != 0) {
            wcr_emit(state, WCR_EVENT_ERROR, "[ERROR] install: failed to install dependency '%s'",
                     depends[i]);
            free_string_array(depends, depends_count);
            free(archive_address);
            free(archive_sha256);
            free(version);
            pkg_specifier_free(&spec);
            return -1;
        }
    }
    free_string_array(depends, depends_count);

    if (fetch_and_verify_archive(state, proto, source_uri, archive_address, archive_sha256, &archive_path) != 0) {
        free(archive_address);
        free(archive_sha256);
        free(version);
        pkg_specifier_free(&spec);
        return -1;
    }
    free(archive_address);
    free(archive_sha256);

    wcr_emit(state, WCR_EVENT_INFO, "[INFO] install: archive downloaded and SHA256 verified ok (%s)", archive_path);

    if (extract_archive(state, archive_path, spec.name) != 0) {
        free(archive_path);
        free(version);
        pkg_specifier_free(&spec);
        return -1;
    }

    free(archive_path);
    record_installed(state, spec.name, version ? version : "unknown", is_dep);
    free(version);
    wcr_emit(state, WCR_EVENT_INFO, "[INFO] install: '%s' installed successfully", spec.name);
    pkg_specifier_free(&spec);
    return 0;
}

static int install_http(const wcr_state *state, protocol_type proto, const char *source_uri, const char *package_name)
{
    wcr_install_ctx ctx = {0};
    int rc;

    rc = install_http_with_deps(state, proto, source_uri, package_name, &ctx, 0);
    install_ctx_cleanup(&ctx);
    return rc;
}

static int variants_wcr(const wcr_state *state, const char *source_uri,
                        const char *package_name,
                        wcr_pkg_variant **out, size_t *out_count)
{
    char host[256];
    int port;
    wcr_conn *conn;
    char *listing;
    const wcr_source *src;
    int rc;

    parse_host_port(source_uri, host, sizeof(host), &port);
    src = wcr_state_find_source(state, source_uri);

    if (wcr_connect_source(state, src, host, port, &conn) != 0)
        return -1;

    listing = wcr_get_package_listing(conn, package_name);
    wcr_close(conn);
    if (!listing)
        return -1;

    rc = parse_wcr_listing_variants(listing, out, out_count);
    free(listing);
    return rc;
}

static int variants_http(const wcr_state *state, protocol_type proto,
                         const char *source_uri, const char *package_name,
                         wcr_pkg_variant **out, size_t *out_count)
{
    char *pkgs_list_path;
    char *register_path = NULL;
    char *checksum = NULL;
    char *version = NULL;
    char *register_content = NULL;
    int rc;

    pkgs_list_path = get_cache_path(state, "pkgs.list");
    if (!pkgs_list_path)
        return -1;

    if (find_package_in_list(pkgs_list_path, package_name, &register_path, &checksum, &version) != 0) {
        free(pkgs_list_path);
        return -1;
    }
    free(pkgs_list_path);
    free(checksum);
    free(version);

    if (fetch_register(state, proto, source_uri, register_path, &register_content) != 0) {
        free(register_path);
        return -1;
    }
    free(register_path);

    rc = parse_register_variants(register_content, out, out_count);
    free(register_content);
    return rc;
}

int list_package_variants(const wcr_state *state, protocol_type proto,
                          const char *source_uri, const char *package_name,
                          wcr_pkg_variant **out, size_t *out_count)
{
    *out = NULL;
    *out_count = 0;

    if (!state || !source_uri || !package_name)
        return -1;

    if (proto == PROT_WCR)
        return variants_wcr(state, source_uri, package_name, out, out_count);

    if (curl_global_init(CURL_GLOBAL_DEFAULT) != 0)
        return -1;

    {
        int rc = variants_http(state, proto, source_uri, package_name, out, out_count);
        curl_global_cleanup();
        return rc;
    }
}

static int metadata_wcr(const wcr_state *state, const char *source_uri,
                        const struct pkg_specifier *spec, wcr_pkg_metadata *out)
{
    char host[256];
    int port;
    wcr_conn *conn;
    char *listing;
    char *metadata_json;
    char location_hash[256] = {0};
    const wcr_source *src;

    parse_host_port(source_uri, host, sizeof(host), &port);
    src = wcr_state_find_source(state, source_uri);

    if (wcr_connect_source(state, src, host, port, &conn) != 0)
        return -1;

    listing = wcr_get_package_listing(conn, spec->name);
    if (!listing) {
        wcr_close(conn);
        return -1;
    }

    if (select_variant_wcr(listing, spec, location_hash, sizeof(location_hash)) != 0) {
        free(listing);
        wcr_close(conn);
        return -1;
    }
    free(listing);

    metadata_json = wcr_get_package_metadata(conn, spec->name, location_hash);
    wcr_close(conn);
    if (!metadata_json)
        return -1;

    {
        int rc = json_parse_metadata(metadata_json, out);
        free(metadata_json);
        return rc;
    }
}

static int metadata_http(const wcr_state *state, protocol_type proto,
                         const char *source_uri, const struct pkg_specifier *spec,
                         wcr_pkg_metadata *out)
{
    char *pkgs_list_path;
    char *register_path = NULL;
    char *checksum = NULL;
    char *version = NULL;
    char *register_content = NULL;
    char *variant_location = NULL;
    char *url = NULL;
    char *metadata_json = NULL;

    pkgs_list_path = get_cache_path(state, "pkgs.list");
    if (!pkgs_list_path)
        return -1;

    if (find_package_in_list(pkgs_list_path, spec->name, &register_path, &checksum, &version) != 0) {
        free(pkgs_list_path);
        return -1;
    }
    free(pkgs_list_path);
    free(checksum);
    free(version);

    if (fetch_register(state, proto, source_uri, register_path, &register_content) != 0) {
        free(register_path);
        return -1;
    }
    free(register_path);

    if (select_variant_register(register_content, spec, &variant_location) != 0) {
        free(register_content);
        return -1;
    }
    free(register_content);

    url = build_url(source_uri, variant_location);
    free(variant_location);
    if (!url)
        return -1;

    metadata_json = download_to_string(proto, url);
    free(url);
    if (!metadata_json)
        return -1;

    {
        int rc = json_parse_metadata(metadata_json, out);
        free(metadata_json);
        return rc;
    }
}

int get_package_metadata(const wcr_state *state, protocol_type proto,
                         const char *source_uri, const char *package_spec,
                         wcr_pkg_metadata *out)
{
    struct pkg_specifier spec;
    int rc;

    memset(out, 0, sizeof(*out));
    if (!state || !source_uri || !package_spec)
        return -1;

    if (pkg_specifier_parse(package_spec, &spec) != 0)
        return -1;

    if (proto == PROT_WCR) {
        rc = metadata_wcr(state, source_uri, &spec, out);
        pkg_specifier_free(&spec);
        return rc;
    }

    if (curl_global_init(CURL_GLOBAL_DEFAULT) != 0) {
        pkg_specifier_free(&spec);
        return -1;
    }

    rc = metadata_http(state, proto, source_uri, &spec, out);
    pkg_specifier_free(&spec);
    curl_global_cleanup();
    return rc;
}

void free_package_metadata(wcr_pkg_metadata *meta)
{
    if (!meta)
        return;
    free(meta->name);
    free(meta->version);
    free(meta->arch);
    free(meta->machine);
    free(meta->description);
    free(meta->address);
    free(meta->sha256);
    free(meta->md5);
    free_string_array(meta->depends, meta->depends_count);
    memset(meta, 0, sizeof(*meta));
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
