#include "pkg_downloader.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

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

char *download_to_string(const char *url)
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

int download_to_file(const char *url, const char *filepath)
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
