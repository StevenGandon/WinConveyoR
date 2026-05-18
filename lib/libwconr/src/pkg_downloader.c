#include "pkg_downloader.h"
#include "wcr_event_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

char *build_url(const char *source_uri, const char *path)
{
    size_t uri_len;
    size_t path_len;
    char *url;

    if (!source_uri || !path) {
        wcr_emit(NULL, WCR_EVENT_ERROR, "[ERROR] build_url: source_uri or path is NULL");
        return NULL;
    }

    uri_len = strlen(source_uri);
    path_len = strlen(path);

    url = malloc(uri_len + path_len + 2);
    if (!url) {
        wcr_emit(NULL, WCR_EVENT_ERROR, "[ERROR] build_url: malloc failed");
        return NULL;
    }

    if (uri_len > 0 && source_uri[uri_len - 1] == '/' && path[0] == '/') {
        sprintf(url, "%s%s", source_uri, path + 1);
    } else if ((uri_len == 0 || source_uri[uri_len - 1] != '/') && path[0] != '/') {
        sprintf(url, "%s/%s", source_uri, path);
    } else {
        sprintf(url, "%s%s", source_uri, path);
    }

    return url;
}

static size_t write_memory_callback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    struct memory_buffer_s *mem = (struct memory_buffer_s *)userp;

    unsigned char *ptr = realloc(mem->data, mem->size + realsize + 1);
    if (!ptr) {
        wcr_emit(NULL, WCR_EVENT_ERROR, "[ERROR] write_memory_callback: realloc failed");
        return 0;
    }

    mem->data = ptr;
    memcpy(&(mem->data[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->data[mem->size] = '\0';

    return realsize;
}

char *download_to_string(protocol_type proto, const char *url)
{
    CURL *curl;
    CURLcode res;
    struct memory_buffer_s buffer;
    char *result = NULL;

    wcr_emit(NULL, WCR_EVENT_DEBUG, "[DEBUG] download_to_string: url=%s", url);

    if (!url) {
        wcr_emit(NULL, WCR_EVENT_ERROR, "[ERROR] download_to_string: url is NULL");
        return NULL;
    }

    if (proto == PROT_WCR) {
        wcr_emit(NULL, WCR_EVENT_ERROR, "[ERROR] download_to_string: PROT_WCR not implemented yet");
        return NULL;
    }

    buffer.data = malloc(1);
    buffer.size = 0;

    if (!buffer.data) {
        wcr_emit(NULL, WCR_EVENT_ERROR, "[ERROR] download_to_string: initial malloc failed");
        return NULL;
    }

    curl = curl_easy_init();
    if (!curl) {
        wcr_emit(NULL, WCR_EVENT_ERROR, "[ERROR] download_to_string: curl_easy_init failed");
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
        wcr_emit(NULL, WCR_EVENT_ERROR, "[ERROR] download_to_string: curl failed: %s", curl_easy_strerror(res));
        free(buffer.data);
        return NULL;
    }

    wcr_emit(NULL, WCR_EVENT_DEBUG, "[DEBUG] download_to_string: downloaded %zu bytes", buffer.size);

    result = (char *)buffer.data;
    return result;
}

int download_to_file(protocol_type proto, const char *url, const char *filepath)
{
    CURL *curl;
    CURLcode res;
    FILE *fp;

    wcr_emit(NULL, WCR_EVENT_DEBUG, "[DEBUG] download_to_file: url=%s, filepath=%s", url, filepath);

    if (!url || !filepath) {
        wcr_emit(NULL, WCR_EVENT_ERROR, "[ERROR] download_to_file: url or filepath is NULL");
        return -1;
    }

    if (proto == PROT_WCR) {
        wcr_emit(NULL, WCR_EVENT_ERROR, "[ERROR] download_to_file: PROT_WCR not implemented yet");
        return -1;
    }

    fp = fopen(filepath, "wb");
    if (!fp) {
        wcr_emit(NULL, WCR_EVENT_ERROR, "[ERROR] download_to_file: cannot open file %s for writing", filepath);
        return -1;
    }

    curl = curl_easy_init();
    if (!curl) {
        wcr_emit(NULL, WCR_EVENT_ERROR, "[ERROR] download_to_file: curl_easy_init failed");
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
        wcr_emit(NULL, WCR_EVENT_ERROR, "[ERROR] download_to_file: curl failed: %s", curl_easy_strerror(res));
        remove(filepath);
        return -1;
    }

    wcr_emit(NULL, WCR_EVENT_DEBUG, "[DEBUG] download_to_file: success");

    return 0;
}
