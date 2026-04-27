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

static int find_package_in_list(const char *pkgs_list_path, const char *package_name,
                                 char **out_register_path, char **out_checksum)
{
    FILE *fp;
    char line[1024];
    size_t name_len;

    *out_register_path = NULL;
    *out_checksum = NULL;

    fp = fopen(pkgs_list_path, "r");
    if (!fp) {
        fprintf(stderr, "[ERROR] find_package_in_list: cannot open %s\n", pkgs_list_path);
        return -1;
    }

    name_len = strlen(package_name);

    while (fgets(line, sizeof(line), fp)) {
        char *end = line + strlen(line);
        while (end > line && (*(end - 1) == '\n' || *(end - 1) == '\r')) {
            *(--end) = '\0';
        }

        if (strncmp(line, package_name, name_len) != 0 || line[name_len] != ',') {
            continue;
        }

        char *p = line + name_len + 1;
        char *version = p;
        char *comma1 = strchr(version, ',');
        if (!comma1) continue;
        *comma1 = '\0';

        char *register_path = comma1 + 1;
        char *comma2 = strchr(register_path, ',');
        if (!comma2) continue;
        *comma2 = '\0';

        char *checksum = comma2 + 1;

        *out_register_path = strdup(register_path);
        *out_checksum = strdup(checksum);

        printf("[DEBUG] find_package_in_list: %s version=%s register=%s checksum=%s\n",
               package_name, version, *out_register_path, *out_checksum);

        fclose(fp);

        if (!*out_register_path || !*out_checksum) {
            if (*out_register_path) free(*out_register_path);
            if (*out_checksum) free(*out_checksum);
            *out_register_path = NULL;
            *out_checksum = NULL;
            return -1;
        }

        return 0;
    }

    fclose(fp);
    fprintf(stderr, "[ERROR] find_package_in_list: package '%s' not found in %s\n", package_name, pkgs_list_path);
    return -1;
}

static int json_extract_string(const char *json, const char *key, char **out_value)
{
    char needle[128];
    const char *p;
    const char *value_start;
    const char *value_end;
    size_t value_len;

    *out_value = NULL;

    if (snprintf(needle, sizeof(needle), "\"%s\"", key) >= (int)sizeof(needle)) {
        fprintf(stderr, "[ERROR] json_extract_string: key too long: %s\n", key);
        return -1;
    }

    p = strstr(json, needle);
    if (!p) {
        fprintf(stderr, "[ERROR] json_extract_string: key '%s' not found\n", key);
        return -1;
    }

    p += strlen(needle);

    while (*p && *p != ':') p++;
    if (*p != ':') {
        fprintf(stderr, "[ERROR] json_extract_string: missing ':' after key '%s'\n", key);
        return -1;
    }
    p++;

    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;

    if (*p != '"') {
        fprintf(stderr, "[ERROR] json_extract_string: value for key '%s' is not a string\n", key);
        return -1;
    }
    p++;
    value_start = p;

    value_end = strchr(value_start, '"');
    if (!value_end) {
        fprintf(stderr, "[ERROR] json_extract_string: unterminated string for key '%s'\n", key);
        return -1;
    }

    value_len = (size_t)(value_end - value_start);

    *out_value = malloc(value_len + 1);
    if (!*out_value) {
        fprintf(stderr, "[ERROR] json_extract_string: malloc failed\n");
        return -1;
    }
    memcpy(*out_value, value_start, value_len);
    (*out_value)[value_len] = '\0';

    return 0;
}

static int parse_first_location(const char *register_content, char **out_location)
{
    const char *p = register_content;
    const char *key = "Location:";
    size_t key_len = strlen(key);

    *out_location = NULL;

    while (*p) {
        const char *line_end = strchr(p, '\n');
        size_t line_len = line_end ? (size_t)(line_end - p) : strlen(p);

        if (line_len >= key_len && strncmp(p, key, key_len) == 0) {
            const char *value = p + key_len;
            const char *value_end = p + line_len;

            while (value < value_end && (*value == ' ' || *value == '\t')) value++;
            while (value_end > value && (*(value_end - 1) == ' ' || *(value_end - 1) == '\t' || *(value_end - 1) == '\r')) value_end--;

            size_t value_len = (size_t)(value_end - value);
            if (value_len == 0) {
                fprintf(stderr, "[ERROR] parse_first_location: empty Location value\n");
                return -1;
            }

            *out_location = malloc(value_len + 1);
            if (!*out_location) {
                fprintf(stderr, "[ERROR] parse_first_location: malloc failed\n");
                return -1;
            }
            memcpy(*out_location, value, value_len);
            (*out_location)[value_len] = '\0';

            printf("[DEBUG] parse_first_location: found Location=%s\n", *out_location);
            return 0;
        }

        if (!line_end) break;
        p = line_end + 1;
    }

    fprintf(stderr, "[ERROR] parse_first_location: no Location field found in register\n");
    return -1;
}

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
