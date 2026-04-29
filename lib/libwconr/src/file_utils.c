#include "file_utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
        fprintf(stderr, "[ERROR] get_cache_path: state or state->cache_path is NULL\n");
        return NULL;
    }
    if (!filename) {
        fprintf(stderr, "[ERROR] get_cache_path: filename is NULL\n");
        return NULL;
    }

    cache_base = state->cache_path;

    if (mkdir_p(cache_base) != 0) {
        fprintf(stderr, "[WARNING] get_cache_path: failed to ensure cache dir %s exists\n", cache_base);
    }

    path_len = strlen(cache_base) + 1 + strlen(filename) + 1;
    full_path = malloc(path_len);
    if (!full_path) {
        fprintf(stderr, "[ERROR] get_cache_path: malloc failed\n");
        return NULL;
    }

    snprintf(full_path, path_len, "%s/%s", cache_base, filename);

    printf("[DEBUG] get_cache_path: result=%s\n", full_path);
    return full_path;
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

    printf("[DEBUG] calculate_sha256_file: filepath=%s\n", filepath);

    if (!filepath) {
        fprintf(stderr, "[ERROR] calculate_sha256_file: filepath is NULL\n");
        return NULL;
    }

    fp = fopen(filepath, "rb");
    if (!fp) {
        fprintf(stderr, "[ERROR] calculate_sha256_file: cannot open file %s\n", filepath);
        return NULL;
    }

    if (!SHA256_Init(&sha256_ctx)) {
        fprintf(stderr, "[ERROR] calculate_sha256_file: SHA256_Init failed\n");
        fclose(fp);
        return NULL;
    }

    while ((bytes_read = fread(buffer, 1, READ_BUFFER_SIZE, fp)) > 0) {
        if (!SHA256_Update(&sha256_ctx, buffer, bytes_read)) {
            fprintf(stderr, "[ERROR] calculate_sha256_file: SHA256_Update failed\n");
            fclose(fp);
            return NULL;
        }
    }

    fclose(fp);

    if (!SHA256_Final(hash, &sha256_ctx)) {
        fprintf(stderr, "[ERROR] calculate_sha256_file: SHA256_Final failed\n");
        return NULL;
    }

    hash_string = malloc(SHA256_DIGEST_LENGTH * 2 + 1);
    if (!hash_string) {
        fprintf(stderr, "[ERROR] calculate_sha256_file: malloc failed\n");
        return NULL;
    }

    for (i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        sprintf(hash_string + (i * 2), "%02x", hash[i]);
    }
    hash_string[SHA256_DIGEST_LENGTH * 2] = '\0';

    printf("[DEBUG] calculate_sha256_file: hash=%s\n", hash_string);

    return hash_string;
}
