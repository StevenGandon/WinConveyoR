#include "pkg_registry.h"
#include "file_utils.h"
#include "wcr_event_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int is_already_installed(const char *path, const char *name)
{
    FILE *fp;
    char line[REGISTRY_LINE_MAX];
    size_t name_len;

    fp = fopen(path, "r");
    if (!fp)
        return 0;

    name_len = strlen(name);

    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, name, name_len) == 0 && line[name_len] == ',') {
            fclose(fp);
            return 1;
        }
    }

    fclose(fp);
    return 0;
}

int record_installed(const wcr_state *state, const char *name, const char *version)
{
    char *path;
    FILE *fp;

    path = get_cache_path(state, INSTALLED_LIST_FILE);
    if (!path)
        return -1;

    if (is_already_installed(path, name)) {
        wcr_emit(state, WCR_EVENT_DEBUG, "[DEBUG] record_installed: '%s' already registered, skipping", name);
        free(path);
        return 0;
    }

    fp = fopen(path, "a");
    if (!fp) {
        wcr_emit(state, WCR_EVENT_ERROR, "[ERROR] record_installed: cannot open %s", path);
        free(path);
        return -1;
    }

    fprintf(fp, "%s,%s\n", name, version);
    fclose(fp);
    free(path);

    wcr_emit(state, WCR_EVENT_INFO, "[INFO] record_installed: registered '%s' v%s", name, version);
    return 0;
}

int remove_installed(const wcr_state *state, const char *name)
{
    char *path;
    char *tmp_path;
    FILE *fp;
    FILE *tmp;
    char line[REGISTRY_LINE_MAX];
    size_t name_len;
    size_t path_len;
    int found;

    path = get_cache_path(state, INSTALLED_LIST_FILE);
    if (!path)
        return -1;

    path_len = strlen(path) + 5;
    tmp_path = malloc(path_len);
    if (!tmp_path) {
        free(path);
        return -1;
    }
    snprintf(tmp_path, path_len, "%s.tmp", path);

    fp = fopen(path, "r");
    if (!fp) {
        wcr_emit(state, WCR_EVENT_ERROR, "[ERROR] remove_installed: cannot open %s", path);
        free(path);
        free(tmp_path);
        return -1;
    }

    tmp = fopen(tmp_path, "w");
    if (!tmp) {
        fclose(fp);
        free(path);
        free(tmp_path);
        return -1;
    }

    name_len = strlen(name);
    found = 0;

    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, name, name_len) == 0 && line[name_len] == ',') {
            found = 1;
            continue;
        }
        fputs(line, tmp);
    }

    fclose(fp);
    fclose(tmp);

    if (found) {
        remove(path);
        rename(tmp_path, path);
        wcr_emit(state, WCR_EVENT_INFO, "[INFO] remove_installed: removed '%s'", name);
    } else {
        remove(tmp_path);
        wcr_emit(state, WCR_EVENT_WARNING, "[WARNING] remove_installed: '%s' not found in registry", name);
    }

    free(path);
    free(tmp_path);
    return found ? 0 : -1;
}

int list_installed(const wcr_state *state, wcr_installed_pkg **out, size_t *out_count)
{
    char *path;
    FILE *fp;
    char line[REGISTRY_LINE_MAX];
    wcr_installed_pkg *list = NULL;
    size_t count = 0;
    size_t capacity = 0;

    *out = NULL;
    *out_count = 0;

    path = get_cache_path(state, INSTALLED_LIST_FILE);
    if (!path)
        return -1;

    fp = fopen(path, "r");
    free(path);
    if (!fp)
        return 0;

    while (fgets(line, sizeof(line), fp)) {
        char *end = line + strlen(line);
        char *comma;
        wcr_installed_pkg *tmp;

        while (end > line && (*(end - 1) == '\n' || *(end - 1) == '\r'))
            *(--end) = '\0';

        comma = strchr(line, ',');
        if (!comma)
            continue;
        *comma = '\0';

        if (count >= capacity) {
            capacity = capacity ? capacity * 2 : 8;
            tmp = realloc(list, capacity * sizeof(wcr_installed_pkg));
            if (!tmp) {
                free_installed_list(list, count);
                fclose(fp);
                return -1;
            }
            list = tmp;
        }

        list[count].name = strdup(line);
        list[count].version = strdup(comma + 1);
        count++;
    }

    fclose(fp);
    *out = list;
    *out_count = count;
    return 0;
}

void free_installed_list(wcr_installed_pkg *list, size_t count)
{
    size_t i;

    if (!list)
        return;
    for (i = 0; i < count; i++) {
        free(list[i].name);
        free(list[i].version);
    }
    free(list);
}
