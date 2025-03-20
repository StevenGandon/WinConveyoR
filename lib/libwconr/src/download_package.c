#include "libwconr.h"
#include "libwconr_private.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <curl/curl.h>

#ifdef _WIN32
  #include <windows.h>
#else
  #include <unistd.h>
#endif

int download_package(const unsigned char *http_address, const unsigned char *location)
{
    if (!http_address || !location)
        return -1;

    const char *url = (const char *)http_address;
    const char *filename = strrchr(url, '/');
    if (filename && *(filename + 1) != '\0') {
        filename++;
    } else {
        filename = "downloaded_file";
    }

    const char *sep = "/";
#ifdef _WIN32
    sep = "\\";
#endif

    size_t dir_len = strlen((const char *)location);
    int ends_with_sep = (dir_len > 0) &&
        (((const char *)location)[dir_len - 1] == '/' || ((const char *)location)[dir_len - 1] == '\\');

    size_t full_path_size = dir_len + (ends_with_sep ? 0 : strlen(sep)) + strlen(filename) + 1;
    char *full_path = (char *)malloc(full_path_size);
    if (!full_path)
        return -1;

    strcpy(full_path, (const char *)location);
    if (!ends_with_sep) {
        strcat(full_path, sep);
    }
    strcat(full_path, filename);

    printf("Downloading to: %s\n", full_path);

    CURL *curl;
    CURLcode res;
    FILE *file;

    if (curl_global_init(CURL_GLOBAL_DEFAULT) != 0)
    {
        fprintf(stderr, "curl_global_init() failed\n");
        free(full_path);
        return -1;
    }

    curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "curl_easy_init() failed\n");
        free(full_path);
        curl_global_cleanup();
        return -1;
    }

    file = fopen(full_path, "wb");
    if (!file) {
        fprintf(stderr, "fopen() failed for file: %s\n", full_path);
        free(full_path);
        curl_easy_cleanup(curl);
        curl_global_cleanup();
        return -1;
    }

    curl_easy_setopt(curl, CURLOPT_URL, (const char *)http_address);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    res = curl_easy_perform(curl);

    fclose(file);
    curl_easy_cleanup(curl);
    curl_global_cleanup();

    free(full_path);

    if (res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        return -1;
    }

    return 0;
}
