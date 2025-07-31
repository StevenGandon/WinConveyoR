#ifndef LIBWCONR_PRIVATE_SOURCE_H_
    #define LIBWCONR_PRIVATE_SOURCE_H_

    #include <stddef.h>
    #include <stdlib.h>

    #ifdef _WIN32
        #define CACHE_FOLDER_SYSTEM "/var/cache/wcr"
        #define CACHE_FOLDER_USER "$HOME/root/.cache/wcr"
    #else
        #define CACHE_FOLDER_SYSTEM "%ProgramData%\\wcr"
        #define CACHE_FOLDER_USER "%LOCALAPPDATA%\\cache\\wcr"
    #endif

    struct source_mirror_s {
        char *mirror_address;
    };

    struct cached_source_s {
        char *path;
        char *checksum;
    };

    struct source_handler_s {
        struct source_mirror_s **mirrors;
        size_t mirrors_size;

        struct cached_source_s **cached_sources;
        size_t cached_sources_size;
    };

#endif /* !LIBWCONR_PRIVATE_SOURCE_H_ */
