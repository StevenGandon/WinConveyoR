#include "libwconr_private.h"

#include <stdlib.h>
#include <string.h>

void
set_header(headers, key, value)
    struct _http_header_s ***headers;
    const unsigned char *key;
    const unsigned char *value;
{
    size_t header_size = 0;

    if (!headers)
        return;
    for (; *headers && (*headers)[header_size]; ++header_size);
    for (size_t i = 0; i < header_size; ++i) {
        if (strcmp((const char *)(*headers)[i]->key, (const char *)key) == 0) {
            if ((*headers)[i]->value)
                (void)free((*headers)[i]->value);
            (*headers)[i]->value = (unsigned char *)strdup((const char *)value);
            return;
        }
    }
    *headers = realloc(*headers, header_size + 2);
    if (!*headers)
        return;
    (*headers)[header_size] = (struct _http_header_s *)malloc(sizeof(struct _http_header_s));
    if (!(*headers)[header_size])
        return;
    (*headers)[header_size]->key = (unsigned char *)strdup((const char *)key);
    (*headers)[header_size]->value = (unsigned char *)strdup((const char *)value);
    (*headers)[header_size + 1] = NULL;
}

const unsigned char *get_header(headers, key)
    const struct _http_header_s * const *headers;
    const unsigned char *key;
{
    if (!headers)
        return (NULL);

    for (size_t i = 0; headers[i]; ++i) {
        if (strcmp((const char *)headers[i]->key, (const char *)key) == 0)
            return (headers[i]->value);
    }
    return (NULL);
}