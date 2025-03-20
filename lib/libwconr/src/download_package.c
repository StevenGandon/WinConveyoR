#include "libwconr.h"
#include "libwconr_private.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <curl/curl.h>

int download_package(const unsigned char *http_address, const unsigned char *location)
{
    if (!http_address || !location)
        return -1;

    CURL *curl;
    CURLcode res;
    FILE *file;

    if (curl_global_init(CURL_GLOBAL_DEFAULT) != 0)
        return -1;

    curl = curl_easy_init();
    if (!curl) {
        curl_global_cleanup();
        return -1;
    }

    file = fopen((const char *)location, "wb");
    if (!file) {
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

    if (res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n",
                curl_easy_strerror(res));
        return -1;
    }

    return 0;
}
