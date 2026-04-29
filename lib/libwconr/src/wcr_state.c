#include "libwconr.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <unistd.h>
#endif

static char *default_cache_path(void)
{
    const char *home;
    char buf[1024];

#ifdef _WIN32
    home = getenv("LOCALAPPDATA");
    if (!home) {
        return NULL;
    }
    snprintf(buf, sizeof(buf), "%s/cache/wcr", home);
#else
    home = getenv("HOME");
    if (!home) {
        return NULL;
    }
    snprintf(buf, sizeof(buf), "%s/.cache/wcr", home);
#endif

    return strdup(buf);
}

static const char *proto_to_string(protocol_type proto)
{
    switch (proto) {
        case PROT_HTTP: return "http";
        default:        return NULL;
    }
}

static int proto_from_string(const char *s, protocol_type *out)
{
    if (strcmp(s, "http") == 0) { *out = PROT_HTTP; return 0; }
    return -1;
}

wcr_state *new_state(void)
{
    wcr_state *state = (wcr_state *)calloc(1, sizeof(wcr_state));

    if (!state)
        return NULL;

    state->cache_path = default_cache_path();
    if (!state->cache_path) {
        fprintf(stderr, "[ERROR] new_state: failed to compute default cache_path\n");
        free(state);
        return NULL;
    }

    state->sources = NULL;
    state->sources_count = 0;

    return state;
}

void close_state(wcr_state *state)
{
    size_t i;

    if (!state)
        return;

    if (state->sources) {
        for (i = 0; i < state->sources_count; i++) {
            if (!state->sources[i])
                continue;
            free(state->sources[i]->url);
            free(state->sources[i]);
        }
        free(state->sources);
    }

    free(state->cache_path);
    free(state);
}

int wcr_state_add_source(wcr_state *state, protocol_type proto, const char *url)
{
    wcr_source **new_sources;
    wcr_source *source;

    if (!state || !url) {
        fprintf(stderr, "[ERROR] wcr_state_add_source: state or url is NULL\n");
        return -1;
    }

    source = (wcr_source *)calloc(1, sizeof(wcr_source));
    if (!source) {
        fprintf(stderr, "[ERROR] wcr_state_add_source: calloc failed\n");
        return -1;
    }

    source->proto = proto;
    source->url = strdup(url);
    if (!source->url) {
        fprintf(stderr, "[ERROR] wcr_state_add_source: strdup failed\n");
        free(source);
        return -1;
    }

    new_sources = realloc(state->sources, sizeof(wcr_source *) * (state->sources_count + 1));
    if (!new_sources) {
        fprintf(stderr, "[ERROR] wcr_state_add_source: realloc failed\n");
        free(source->url);
        free(source);
        return -1;
    }

    state->sources = new_sources;
    state->sources[state->sources_count] = source;
    state->sources_count++;

    return 0;
}

int write_state(const wcr_state *state, const char *filepath)
{
    FILE *file;
    size_t i;

    if (!state || !filepath)
        return -1;

    file = fopen(filepath, "w");
    if (!file) {
        fprintf(stderr, "[ERROR] write_state: cannot open %s for writing\n", filepath);
        return -1;
    }

    if (state->cache_path) {
        fprintf(file, "cache_path=%s\n", state->cache_path);
    }

    for (i = 0; i < state->sources_count; i++) {
        const wcr_source *src = state->sources[i];
        const char *proto_name;

        if (!src || !src->url)
            continue;

        proto_name = proto_to_string(src->proto);
        if (!proto_name) {
            fprintf(stderr, "[WARNING] write_state: skipping source with unknown proto %d\n", src->proto);
            continue;
        }

        fprintf(file, "source=%s %s\n", proto_name, src->url);
    }

    fclose(file);
    return 0;
}

wcr_state *load_state(const char *filepath)
{
    FILE *file;
    wcr_state *state;
    char line[1024];

    if (!filepath)
        return NULL;

    file = fopen(filepath, "r");
    if (!file) {
        fprintf(stderr, "[ERROR] load_state: cannot open %s for reading\n", filepath);
        return NULL;
    }

    state = new_state();
    if (!state) {
        fclose(file);
        return NULL;
    }

    while (fgets(line, sizeof(line), file)) {
        char *end = line + strlen(line);
        while (end > line && (*(end - 1) == '\n' || *(end - 1) == '\r')) {
            *(--end) = '\0';
        }

        if (line[0] == '\0' || line[0] == '#') {
            continue;
        }

        if (strncmp(line, "cache_path=", 11) == 0) {
            free(state->cache_path);
            state->cache_path = strdup(line + 11);
            if (!state->cache_path) {
                fprintf(stderr, "[ERROR] load_state: strdup failed for cache_path\n");
                close_state(state);
                fclose(file);
                return NULL;
            }
            continue;
        }

        if (strncmp(line, "source=", 7) == 0) {
            char *value = line + 7;
            char *space = strchr(value, ' ');
            protocol_type proto;

            if (!space) {
                fprintf(stderr, "[WARNING] load_state: malformed source line (no space): %s\n", value);
                continue;
            }

            *space = '\0';
            if (proto_from_string(value, &proto) != 0) {
                fprintf(stderr, "[WARNING] load_state: unknown proto '%s', skipping\n", value);
                continue;
            }

            if (wcr_state_add_source(state, proto, space + 1) != 0) {
                close_state(state);
                fclose(file);
                return NULL;
            }
            continue;
        }
    }

    fclose(file);
    return state;
}
