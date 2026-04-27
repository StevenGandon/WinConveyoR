#ifndef PKG_DOWNLOADER_H_
    #define PKG_DOWNLOADER_H_

    #include <stddef.h>

    struct memory_buffer_s {
        unsigned char *data;
        size_t size;
    };

    char *download_to_string(const char *url);
    int download_to_file(const char *url, const char *filepath);

#endif /* !PKG_DOWNLOADER_H_ */
