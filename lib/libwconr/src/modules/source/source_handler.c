#include "libwconr_private_source.h"
#include <string.h>

struct source_mirror_s *create_mirror(const char *address)
{
    struct source_mirror_s *mirror;

    if (!address)
        return (NULL);
    mirror = (struct source_mirror_s *)malloc(sizeof(struct source_mirror_s));
    if (!mirror)
        return (NULL);
    mirror->mirror_address = (char *)strdup(address);
    if (!mirror->mirror_address) {
        (void)free(mirror);
        return (NULL);
    }
    return (mirror);
}

void destroy_mirror(struct source_mirror_s *mirror)
{
    if (!mirror)
        return;
    if (mirror->mirror_address)
        (void)free(mirror->mirror_address);
    (void)free(mirror);
}

struct source_handler_s *create_source_handler(void)
{
    struct source_handler_s *source_handler = (struct source_handler_s *)malloc(sizeof(struct source_handler_s));

    if (!source_handler)
        return (NULL);
    source_handler->mirrors = NULL;
    source_handler->mirrors_size = 0;
    source_handler->cached_sources = NULL;
    source_handler->cached_sources_size = 0;
    return (source_handler);
}

void destroy_source_handler(struct source_handler_s *source_handler)
{
    if (!source_handler)
        return;
    if (source_handler->mirrors) {
        for (size_t i = 0; i < source_handler->mirrors_size; ++i) {
            if (source_handler->mirrors[i])
                continue;
            (void)destroy_mirror(source_handler->mirrors[i]);
            source_handler->mirrors[i] = NULL;
        }
        (void)free(source_handler->mirrors);
        source_handler->mirrors = NULL;
    }
    if (source_handler->cached_sources) {
        for (size_t i = 0; i < source_handler->cached_sources_size; ++i) {
            if (source_handler->cached_sources[i])
                continue;
            // (void)destroy_mirror(source_handler->cached_sources[i]);
            source_handler->cached_sources[i] = NULL;
        }
        (void)free(source_handler->cached_sources);
        source_handler->cached_sources = NULL;
    }
    (void)free(source_handler);
}