#include "libwconr.h"
#include "wcr_event_internal.h"

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
        case PROT_WCR:  return "wcr";
        default:        return NULL;
    }
}

static int proto_from_string(const char *s, protocol_type *out)
{
    if (strcmp(s, "http") == 0) { *out = PROT_HTTP; return 0; }
    if (strcmp(s, "wcr") == 0)  { *out = PROT_WCR;  return 0; }
    return -1;
}

static void wcr_mutex_init(wcr_mutex *m)
{
#ifdef _WIN32
    InitializeCriticalSection(m);
#else
    pthread_mutex_init(m, NULL);
#endif
}

static void wcr_mutex_destroy(wcr_mutex *m)
{
#ifdef _WIN32
    DeleteCriticalSection(m);
#else
    pthread_mutex_destroy(m);
#endif
}

static void wcr_mutex_lock(wcr_mutex *m)
{
#ifdef _WIN32
    EnterCriticalSection(m);
#else
    pthread_mutex_lock(m);
#endif
}

static void wcr_mutex_unlock(wcr_mutex *m)
{
#ifdef _WIN32
    LeaveCriticalSection(m);
#else
    pthread_mutex_unlock(m);
#endif
}

wcr_state *new_state(void)
{
    wcr_state *state = (wcr_state *)calloc(1, sizeof(wcr_state));

    if (!state)
        return NULL;

    wcr_mutex_init(&state->lock);

    state->cache_path = default_cache_path();
    if (!state->cache_path) {
        wcr_emit(NULL, WCR_EVENT_ERROR, "[ERROR] new_state: failed to compute default cache_path");
        wcr_mutex_destroy(&state->lock);
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
            free(state->sources[i]->access_key);
            free(state->sources[i]->server_pubkey_path);
            free(state->sources[i]);
        }
        free(state->sources);
    }

    free(state->cache_path);
    free(state->config_path);
    wcr_mutex_destroy(&state->lock);
    free(state);
}

static void state_auto_save(const wcr_state *state)
{
    if (!state->config_path)
        return;
    if (write_state(state, state->config_path) != 0)
        wcr_emit(state, WCR_EVENT_WARNING, "[WARNING] state_auto_save: failed to write %s", state->config_path);
}

int wcr_state_add_source(wcr_state *state, protocol_type proto, const char *url)
{
    wcr_source **new_sources;
    wcr_source *source;

    if (!state || !url) {
        wcr_emit(state, WCR_EVENT_ERROR, "[ERROR] wcr_state_add_source: state or url is NULL");
        return -1;
    }

    wcr_mutex_lock(&state->lock);

    source = (wcr_source *)calloc(1, sizeof(wcr_source));
    if (!source) {
        wcr_emit(state, WCR_EVENT_ERROR, "[ERROR] wcr_state_add_source: calloc failed");
        wcr_mutex_unlock(&state->lock);
        return -1;
    }

    source->proto = proto;
    source->url = strdup(url);
    if (!source->url) {
        wcr_emit(state, WCR_EVENT_ERROR, "[ERROR] wcr_state_add_source: strdup failed");
        free(source);
        wcr_mutex_unlock(&state->lock);
        return -1;
    }

    new_sources = realloc(state->sources, sizeof(wcr_source *) * (state->sources_count + 1));
    if (!new_sources) {
        wcr_emit(state, WCR_EVENT_ERROR, "[ERROR] wcr_state_add_source: realloc failed");
        free(source->url);
        free(source);
        wcr_mutex_unlock(&state->lock);
        return -1;
    }

    state->sources = new_sources;
    state->sources[state->sources_count] = source;
    state->sources_count++;

    state_auto_save(state);
    wcr_mutex_unlock(&state->lock);
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
        wcr_emit(state, WCR_EVENT_ERROR, "[ERROR] write_state: cannot open %s for writing", filepath);
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
            wcr_emit(state, WCR_EVENT_WARNING, "[WARNING] write_state: skipping source with unknown proto %d", src->proto);
            continue;
        }

        fprintf(file, "source=%s %s\n", proto_name, src->url);
        if (src->access_key)
            fprintf(file, "access_key=%s\n", src->access_key);
        if (src->server_pubkey_path)
            fprintf(file, "server_pubkey=%s\n", src->server_pubkey_path);
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
        wcr_emit(NULL, WCR_EVENT_ERROR, "[ERROR] load_state: cannot open %s for reading", filepath);
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
                wcr_emit(state, WCR_EVENT_ERROR, "[ERROR] load_state: strdup failed for cache_path");
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
                wcr_emit(state, WCR_EVENT_WARNING, "[WARNING] load_state: malformed source line (no space): %s", value);
                continue;
            }

            *space = '\0';
            if (proto_from_string(value, &proto) != 0) {
                wcr_emit(state, WCR_EVENT_WARNING, "[WARNING] load_state: unknown proto '%s', skipping", value);
                continue;
            }

            if (wcr_state_add_source(state, proto, space + 1) != 0) {
                close_state(state);
                fclose(file);
                return NULL;
            }
            continue;
        }

        #define ACCESS_KEY_PREFIX "access_key="
        if (strncmp(line, ACCESS_KEY_PREFIX, sizeof(ACCESS_KEY_PREFIX) - 1) == 0
            && state->sources_count > 0) {
            wcr_source *last = state->sources[state->sources_count - 1];
            free(last->access_key);
            last->access_key = strdup(line + sizeof(ACCESS_KEY_PREFIX) - 1);
            continue;
        }

        #define SERVER_PUBKEY_PREFIX "server_pubkey="
        if (strncmp(line, SERVER_PUBKEY_PREFIX, sizeof(SERVER_PUBKEY_PREFIX) - 1) == 0
            && state->sources_count > 0) {
            wcr_source *last = state->sources[state->sources_count - 1];
            free(last->server_pubkey_path);
            last->server_pubkey_path = strdup(line + sizeof(SERVER_PUBKEY_PREFIX) - 1);
            continue;
        }
    }

    state->config_path = strdup(filepath);

    fclose(file);
    return state;
}

const wcr_source *wcr_state_find_source(const wcr_state *state, const char *url)
{
    size_t i;

    if (!state || !url)
        return NULL;
    for (i = 0; i < state->sources_count; i++) {
        if (state->sources[i] && state->sources[i]->url
            && strcmp(state->sources[i]->url, url) == 0)
            return state->sources[i];
    }
    return NULL;
}

int wcr_source_set_auth(wcr_state *state, size_t index,
                        const char *access_key,
                        const char *server_pubkey_path)
{
    wcr_source *src;

    if (!state || index >= state->sources_count)
        return -1;
    src = state->sources[index];
    if (!src)
        return -1;

    free(src->access_key);
    src->access_key = access_key ? strdup(access_key) : NULL;

    free(src->server_pubkey_path);
    src->server_pubkey_path = server_pubkey_path ? strdup(server_pubkey_path) : NULL;

    state_auto_save(state);
    return 0;
}
