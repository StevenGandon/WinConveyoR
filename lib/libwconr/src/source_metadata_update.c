#include "source_metadata_update.h"

static char* get_cache_path(const char *filename)
{
    char *full_path = NULL;
    size_t path_len;
    
    if (!filename) {
        fprintf(stderr, "[ERROR] get_cache_path: filename is NULL\n");
        return NULL;
    }
    
#ifdef _WIN32
    const char *appdata = getenv("LOCALAPPDATA");
    if (!appdata) {
        fprintf(stderr, "[ERROR] get_cache_path: LOCALAPPDATA not found\n");
        return NULL;
    }
    
    printf("[DEBUG] get_cache_path: LOCALAPPDATA=%s\n", appdata);
    
    path_len = strlen(appdata) + strlen("/cache/wcr/") + strlen(filename) + 1;
    full_path = malloc(path_len);
    if (!full_path) {
        fprintf(stderr, "[ERROR] get_cache_path: malloc failed\n");
        return NULL;
    }
    
    snprintf(full_path, path_len, "%s/cache/wcr/%s", appdata, filename);
    
#else
    const char *cache_base = "/mnt/c/Users/thoma/Documents/GitHub/WinConveyoR/lib/libwconr/src/test";
    
    printf("[DEBUG] get_cache_path: Using cache_base=%s\n", cache_base);
    
    path_len = strlen(cache_base) + 1 + strlen(filename) + 1;
    full_path = malloc(path_len);
    if (!full_path) {
        fprintf(stderr, "[ERROR] get_cache_path: malloc failed\n");
        return NULL;
    }
    
    snprintf(full_path, path_len, "%s/%s", cache_base, filename);
#endif

    printf("[DEBUG] get_cache_path: result=%s\n", full_path);
    return full_path;
}

static char* calculate_sha256_file(const char *filepath)
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

static size_t write_memory_callback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    struct memory_buffer_s *mem = (struct memory_buffer_s *)userp;
    
    unsigned char *ptr = realloc(mem->data, mem->size + realsize + 1);
    if (!ptr) {
        fprintf(stderr, "[ERROR] write_memory_callback: realloc failed\n");
        return 0;
    }
    
    mem->data = ptr;
    memcpy(&(mem->data[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->data[mem->size] = '\0';
    
    return realsize;
}

static char* download_to_string(const char *url)
{
    CURL *curl;
    CURLcode res;
    struct memory_buffer_s buffer;
    char *result = NULL;
    
    printf("[DEBUG] download_to_string: url=%s\n", url);
    
    if (!url) {
        fprintf(stderr, "[ERROR] download_to_string: url is NULL\n");
        return NULL;
    }
    
    buffer.data = malloc(1);
    buffer.size = 0;
    
    if (!buffer.data) {
        fprintf(stderr, "[ERROR] download_to_string: initial malloc failed\n");
        return NULL;
    }
    
    curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "[ERROR] download_to_string: curl_easy_init failed\n");
        free(buffer.data);
        return NULL;
    }
    
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_memory_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&buffer);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "WinConveyoR/1.0");
    
    res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    
    if (res != CURLE_OK) {
        fprintf(stderr, "[ERROR] download_to_string: curl failed: %s\n", curl_easy_strerror(res));
        free(buffer.data);
        return NULL;
    }
    
    printf("[DEBUG] download_to_string: downloaded %zu bytes\n", buffer.size);
    
    result = (char *)buffer.data;
    return result;
}

static int download_to_file(const char *url, const char *filepath)
{
    CURL *curl;
    CURLcode res;
    FILE *fp;
    
    printf("[DEBUG] download_to_file: url=%s, filepath=%s\n", url, filepath);
    
    if (!url || !filepath) {
        fprintf(stderr, "[ERROR] download_to_file: url or filepath is NULL\n");
        return -1;
    }
    
    fp = fopen(filepath, "wb");
    if (!fp) {
        fprintf(stderr, "[ERROR] download_to_file: cannot open file %s for writing\n", filepath);
        return -1;
    }
    
    curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "[ERROR] download_to_file: curl_easy_init failed\n");
        fclose(fp);
        return -1;
    }
    
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "WinConveyoR/1.0");
    
    res = curl_easy_perform(curl);
    
    curl_easy_cleanup(curl);
    fclose(fp);
    
    if (res != CURLE_OK) {
        fprintf(stderr, "[ERROR] download_to_file: curl failed: %s\n", curl_easy_strerror(res));
        remove(filepath);
        return -1;
    }
    
    printf("[DEBUG] download_to_file: success\n");
    
    return 0;
}

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
