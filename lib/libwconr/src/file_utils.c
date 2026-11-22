#include "file_utils.h"
#include "wcr_event_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <openssl/sha.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifdef _WIN32
    #include <windows.h>
    #include <direct.h>
    #define MKDIR(p) _mkdir(p)
#else
    #include <unistd.h>
    #define MKDIR(p) mkdir(p, 0755)
#endif

static int mkdir_p(const char *path)
{
    char tmp[1024];
    char *p;
    size_t len;

    len = strlen(path);
    if (len >= sizeof(tmp)) {
        return -1;
    }
    memcpy(tmp, path, len + 1);

    for (p = tmp + 1; *p; p++) {
        if (*p == '/' || *p == '\\') {
            char saved = *p;
            *p = '\0';
            (void)MKDIR(tmp);
            *p = saved;
        }
    }
    (void)MKDIR(tmp);
    return 0;
}

char *get_cache_path(const struct wcr_state_s *state, const char *filename)
{
    char *full_path = NULL;
    const char *cache_base;
    size_t path_len;

    if (!state || !state->cache_path) {
        wcr_emit(state, WCR_EVENT_ERROR, "[ERROR] get_cache_path: state or state->cache_path is NULL");
        return NULL;
    }
    if (!filename) {
        wcr_emit(state, WCR_EVENT_ERROR, "[ERROR] get_cache_path: filename is NULL");
        return NULL;
    }

    cache_base = state->cache_path;

    if (mkdir_p(cache_base) != 0) {
        wcr_emit(state, WCR_EVENT_WARNING, "[WARNING] get_cache_path: failed to ensure cache dir %s exists", cache_base);
    }

    path_len = strlen(cache_base) + 1 + strlen(filename) + 1;
    full_path = malloc(path_len);
    if (!full_path) {
        wcr_emit(state, WCR_EVENT_ERROR, "[ERROR] get_cache_path: malloc failed");
        return NULL;
    }

    snprintf(full_path, path_len, "%s/%s", cache_base, filename);

    wcr_emit(state, WCR_EVENT_DEBUG, "[DEBUG] get_cache_path: result=%s", full_path);
    return full_path;
}

#define EXTRACT_CMD_MAX 2048
#define PKG_SUBDIR_MAX 256

int extract_archive(const struct wcr_state_s *state, const char *archive_path, const char *package_name)
{
    char pkg_subdir[PKG_SUBDIR_MAX];
    char cmd[EXTRACT_CMD_MAX];
    char *install_dir;
    int rc;

    snprintf(pkg_subdir, sizeof(pkg_subdir), "packages/%s", package_name);
    install_dir = get_cache_path(state, pkg_subdir);
    if (!install_dir)
        return -1;

    if (mkdir_p(install_dir) != 0) {
        wcr_emit(state, WCR_EVENT_ERROR, "[ERROR] extract_archive: cannot create %s", install_dir);
        free(install_dir);
        return -1;
    }

    snprintf(cmd, sizeof(cmd), "tar -xzf \"%s\" -C \"%s\"", archive_path, install_dir);
    wcr_emit(state, WCR_EVENT_INFO, "[INFO] extract_archive: extracting to %s", install_dir);

    rc = system(cmd);
    free(install_dir);

    if (rc != 0) {
        wcr_emit(state, WCR_EVENT_ERROR, "[ERROR] extract_archive: tar failed (rc=%d)", rc);
        return -1;
    }

    wcr_emit(state, WCR_EVENT_INFO, "[INFO] extract_archive: ok");
    return 0;
}

char *read_file_text(const char *path)
{
    FILE *fp;
    long sz;
    char *buf;

    fp = fopen(path, "r");
    if (!fp)
        return NULL;
    fseek(fp, 0, SEEK_END);
    sz = ftell(fp);
    if (sz <= 0) {
        fclose(fp);
        return NULL;
    }
    rewind(fp);
    buf = malloc((size_t)sz + 1);
    if (!buf) {
        fclose(fp);
        return NULL;
    }
    if (fread(buf, 1, (size_t)sz, fp) != (size_t)sz) {
        free(buf);
        fclose(fp);
        return NULL;
    }
    buf[sz] = '\0';
    fclose(fp);
    return buf;
}

char *calculate_sha256_file(const char *filepath)
{
    FILE *fp;
    unsigned char buffer[READ_BUFFER_SIZE];
    unsigned char hash[SHA256_DIGEST_LENGTH];
    char *hash_string;
    SHA256_CTX sha256_ctx;
    size_t bytes_read;
    unsigned char i;
    memset(buffer, 0, READ_BUFFER_SIZE);

    wcr_emit(NULL, WCR_EVENT_DEBUG, "[DEBUG] calculate_sha256_file: filepath=%s", filepath);

    if (!filepath) {
        wcr_emit(NULL, WCR_EVENT_ERROR, "[ERROR] calculate_sha256_file: filepath is NULL");
        return NULL;
    }

    fp = fopen(filepath, "rb");
    if (!fp) {
        wcr_emit(NULL, WCR_EVENT_ERROR, "[ERROR] calculate_sha256_file: cannot open file %s", filepath);
        return NULL;
    }

    if (!SHA256_Init(&sha256_ctx)) {
        wcr_emit(NULL, WCR_EVENT_ERROR, "[ERROR] calculate_sha256_file: SHA256_Init failed");
        fclose(fp);
        return NULL;
    }

    while ((bytes_read = fread(buffer, 1, READ_BUFFER_SIZE, fp)) > 0) {
        if (!SHA256_Update(&sha256_ctx, buffer, bytes_read)) {
            wcr_emit(NULL, WCR_EVENT_ERROR, "[ERROR] calculate_sha256_file: SHA256_Update failed");
            fclose(fp);
            return NULL;
        }
    }

    fclose(fp);

    if (!SHA256_Final(hash, &sha256_ctx)) {
        wcr_emit(NULL, WCR_EVENT_ERROR, "[ERROR] calculate_sha256_file: SHA256_Final failed");
        return NULL;
    }

    hash_string = malloc(SHA256_DIGEST_LENGTH * 2 + 1);
    if (!hash_string) {
        wcr_emit(NULL, WCR_EVENT_ERROR, "[ERROR] calculate_sha256_file: malloc failed");
        return NULL;
    }

    for (i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        sprintf(hash_string + (i * 2), "%02x", hash[i]);
    }
    hash_string[SHA256_DIGEST_LENGTH * 2] = '\0';

    wcr_emit(NULL, WCR_EVENT_DEBUG, "[DEBUG] calculate_sha256_file: hash=%s", hash_string);

    return hash_string;
}
