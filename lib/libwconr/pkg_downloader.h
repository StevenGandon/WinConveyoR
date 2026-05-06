#ifndef PKG_DOWNLOADER_H_
    #define PKG_DOWNLOADER_H_

    #include <stddef.h>
    #include "libwconr.h"

    struct memory_buffer_s {
        unsigned char *data;
        size_t size;
    };

    char *download_to_string(protocol_type proto, const char *url);
    int download_to_file(protocol_type proto, const char *url, const char *filepath);
    char *build_url(const char *source_uri, const char *path);

#endif /* !PKG_DOWNLOADER_H_ */
