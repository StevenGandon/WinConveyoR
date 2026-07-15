#include "pkg_parsing.h"
#include "pkg_specifier.h"
#include "wcr_event_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
    #define strcasecmp _stricmp
#else
    #include <strings.h>
#endif
#include <cJSON.h>

int find_package_in_list(const char *pkgs_list_path, const char *package_name,
                          char **out_register_path, char **out_checksum,
                          char **out_version)
{
    FILE *fp;
    char line[1024];
    size_t name_len;

    *out_register_path = NULL;
    *out_checksum = NULL;
    if (out_version)
        *out_version = NULL;

    fp = fopen(pkgs_list_path, "r");
    if (!fp) {
        wcr_emit(NULL, WCR_EVENT_ERROR, "[ERROR] find_package_in_list: cannot open %s", pkgs_list_path);
        return -1;
    }

    name_len = strlen(package_name);

    while (fgets(line, sizeof(line), fp)) {
        char *end = line + strlen(line);
        while (end > line && (*(end - 1) == '\n' || *(end - 1) == '\r')) {
            *(--end) = '\0';
        }

        if (strncmp(line, package_name, name_len) != 0 || line[name_len] != ',') {
            continue;
        }

        char *p = line + name_len + 1;
        char *version = p;
        char *comma1 = strchr(version, ',');
        if (!comma1) continue;
        *comma1 = '\0';

        char *register_path = comma1 + 1;
        char *comma2 = strchr(register_path, ',');
        if (!comma2) continue;
        *comma2 = '\0';

        char *checksum = comma2 + 1;

        *out_register_path = strdup(register_path);
        *out_checksum = strdup(checksum);
        if (out_version)
            *out_version = strdup(version);

        wcr_emit(NULL, WCR_EVENT_DEBUG, "[DEBUG] find_package_in_list: %s version=%s register=%s checksum=%s",
               package_name, version, *out_register_path, *out_checksum);

        fclose(fp);

        if (!*out_register_path || !*out_checksum) {
            if (*out_register_path) free(*out_register_path);
            if (*out_checksum) free(*out_checksum);
            *out_register_path = NULL;
            *out_checksum = NULL;
            if (out_version) { free(*out_version); *out_version = NULL; }
            return -1;
        }

        return 0;
    }

    fclose(fp);
    wcr_emit(NULL, WCR_EVENT_ERROR, "[ERROR] find_package_in_list: package '%s' not found in %s", package_name, pkgs_list_path);
    return -1;
}

int parse_first_location(const char *register_content, char **out_location)
{
    const char *p = register_content;
    const char *key = "Location:";
    size_t key_len = strlen(key);

    *out_location = NULL;

    while (*p) {
        const char *line_end = strchr(p, '\n');
        size_t line_len = line_end ? (size_t)(line_end - p) : strlen(p);

        if (line_len >= key_len && strncmp(p, key, key_len) == 0) {
            const char *value = p + key_len;
            const char *value_end = p + line_len;

            while (value < value_end && (*value == ' ' || *value == '\t')) value++;
            while (value_end > value && (*(value_end - 1) == ' ' || *(value_end - 1) == '\t' || *(value_end - 1) == '\r')) value_end--;

            size_t value_len = (size_t)(value_end - value);
            if (value_len == 0) {
                wcr_emit(NULL, WCR_EVENT_ERROR, "[ERROR] parse_first_location: empty Location value");
                return -1;
            }

            *out_location = malloc(value_len + 1);
            if (!*out_location) {
                wcr_emit(NULL, WCR_EVENT_ERROR, "[ERROR] parse_first_location: malloc failed");
                return -1;
            }
            memcpy(*out_location, value, value_len);
            (*out_location)[value_len] = '\0';

            wcr_emit(NULL, WCR_EVENT_DEBUG, "[DEBUG] parse_first_location: found Location=%s", *out_location);
            return 0;
        }

        if (!line_end) break;
        p = line_end + 1;
    }

    wcr_emit(NULL, WCR_EVENT_ERROR, "[ERROR] parse_first_location: no Location field found in register");
    return -1;
}

static char *cjson_strdup(cJSON *root, const char *key)
{
    cJSON *item = cJSON_GetObjectItemCaseSensitive(root, key);

    if (cJSON_IsString(item) && item->valuestring)
        return strdup(item->valuestring);
    return NULL;
}

static long cjson_long(cJSON *root, const char *key)
{
    cJSON *item = cJSON_GetObjectItemCaseSensitive(root, key);

    if (cJSON_IsNumber(item))
        return (long)item->valuedouble;
    return 0;
}

int json_parse_metadata(const char *json, struct wcr_pkg_metadata_s *out)
{
    cJSON *root;
    cJSON *deps;
    cJSON *elem;
    size_t i;

    memset(out, 0, sizeof(*out));

    root = cJSON_Parse(json);
    if (!root)
        return -1;

    out->name = cjson_strdup(root, "package");
    out->version = cjson_strdup(root, "version");
    out->arch = cjson_strdup(root, "architecture");
    out->machine = cjson_strdup(root, "machine");
    out->description = cjson_strdup(root, "description");
    out->address = cjson_strdup(root, "address");
    out->sha256 = cjson_strdup(root, "SHA256");
    out->md5 = cjson_strdup(root, "MD5sum");
    out->size = cjson_long(root, "size");
    out->added_at = cjson_long(root, "added_at");

    deps = cJSON_GetObjectItemCaseSensitive(root, "depends");
    if (cJSON_IsArray(deps)) {
        out->depends_count = (size_t)cJSON_GetArraySize(deps);
        if (out->depends_count > 0) {
            out->depends = calloc(out->depends_count, sizeof(char *));
            if (out->depends) {
                i = 0;
                cJSON_ArrayForEach(elem, deps) {
                    if (cJSON_IsString(elem) && elem->valuestring)
                        out->depends[i] = strdup(elem->valuestring);
                    i++;
                }
            }
        }
    }

    cJSON_Delete(root);
    return 0;
}

int json_extract_string(const char *json, const char *key, char **out_value)
{
    cJSON *root = cJSON_Parse(json);
    cJSON *item;

    *out_value = NULL;
    if (!root) {
        wcr_emit(NULL, WCR_EVENT_ERROR, "[ERROR] json_extract_string: invalid JSON");
        return -1;
    }
    item = cJSON_GetObjectItemCaseSensitive(root, key);
    if (!cJSON_IsString(item) || !item->valuestring) {
        wcr_emit(NULL, WCR_EVENT_ERROR, "[ERROR] json_extract_string: key '%s' not found or not a string", key);
        cJSON_Delete(root);
        return -1;
    }
    *out_value = strdup(item->valuestring);
    cJSON_Delete(root);
    if (!*out_value) {
        wcr_emit(NULL, WCR_EVENT_ERROR, "[ERROR] json_extract_string: malloc failed");
        return -1;
    }
    return 0;
}

void free_string_array(char **values, size_t count)
{
    size_t i;

    if (!values)
        return;
    for (i = 0; i < count; i++)
        free(values[i]);
    free(values);
}

int json_extract_string_array(const char *json, const char *key,
                              char ***out_values, size_t *out_count)
{
    cJSON *root;
    cJSON *item;
    cJSON *elem;
    char **values;
    size_t count;
    size_t i;

    *out_values = NULL;
    *out_count = 0;

    root = cJSON_Parse(json);
    if (!root) {
        wcr_emit(NULL, WCR_EVENT_ERROR, "[ERROR] json_extract_string_array: invalid JSON");
        return -1;
    }

    item = cJSON_GetObjectItemCaseSensitive(root, key);
    if (!cJSON_IsArray(item)) {
        cJSON_Delete(root);
        return 0;
    }

    count = (size_t)cJSON_GetArraySize(item);
    if (count == 0) {
        cJSON_Delete(root);
        return 0;
    }

    values = calloc(count, sizeof(char *));
    if (!values) {
        cJSON_Delete(root);
        return -1;
    }

    i = 0;
    cJSON_ArrayForEach(elem, item) {
        if (!cJSON_IsString(elem) || !elem->valuestring) {
            free_string_array(values, i);
            cJSON_Delete(root);
            return -1;
        }
        values[i] = strdup(elem->valuestring);
        if (!values[i]) {
            free_string_array(values, i);
            cJSON_Delete(root);
            return -1;
        }
        i++;
    }

    cJSON_Delete(root);
    *out_values = values;
    *out_count = count;
    return 0;
}

static char *extract_register_field(const char *block_start, const char *block_end, const char *key)
{
    const char *p = block_start;
    size_t key_len = strlen(key);

    while (p < block_end) {
        const char *line_end = memchr(p, '\n', (size_t)(block_end - p));

        if (!line_end)
            line_end = block_end;

        if ((size_t)(line_end - p) >= key_len && strncmp(p, key, key_len) == 0) {
            const char *val = p + key_len;
            const char *val_end = line_end;

            while (val < val_end && (*val == ' ' || *val == '\t')) val++;
            while (val_end > val && (*(val_end - 1) == '\r' || *(val_end - 1) == ' ')) val_end--;

            if (val < val_end)
                return strndup(val, (size_t)(val_end - val));
        }

        p = line_end + 1;
    }

    return NULL;
}

int select_variant_register(const char *register_content,
                            const struct pkg_specifier *spec,
                            char **out_location)
{
    const char *p = register_content;
    char *first_location = NULL;

    *out_location = NULL;

    while (*p) {
        const char *block_start = p;
        const char *block_end;
        char *ver, *arch, *mach, *loc;
        int match;

        block_end = strstr(p, "\n\n");
        if (!block_end) {
            block_end = p + strlen(p);
            p = block_end;
        } else {
            p = block_end + 2;
        }

        loc = extract_register_field(block_start, block_end, "Location:");
        if (!loc)
            continue;

        if (!first_location)
            first_location = strdup(loc);

        ver = extract_register_field(block_start, block_end, "Version:");
        arch = extract_register_field(block_start, block_end, "Architecture:");
        mach = extract_register_field(block_start, block_end, "Machine:");

        match = 1;
        if (spec->version && ver && strcasecmp(spec->version, ver) != 0) match = 0;
        if (spec->arch && arch && strcasecmp(spec->arch, arch) != 0) match = 0;
        if (spec->machine && mach && strcasecmp(spec->machine, mach) != 0) match = 0;

        free(ver);
        free(arch);
        free(mach);

        if (match) {
            *out_location = loc;
            free(first_location);
            return 0;
        }

        free(loc);
    }

    if (!spec->version && !spec->arch && !spec->machine && first_location) {
        *out_location = first_location;
        return 0;
    }

    free(first_location);
    return -1;
}

int parse_register_variants(const char *register_content,
                            struct wcr_pkg_variant_s **out, size_t *out_count)
{
    const char *p = register_content;
    struct wcr_pkg_variant_s *list = NULL;
    size_t count = 0;
    size_t capacity = 0;

    *out = NULL;
    *out_count = 0;

    while (*p) {
        const char *block_start = p;
        const char *block_end;
        struct wcr_pkg_variant_s *tmp;

        block_end = strstr(p, "\n\n");
        if (!block_end) {
            block_end = p + strlen(p);
            p = block_end;
        } else {
            p = block_end + 2;
        }

        {
            char *ver = extract_register_field(block_start, block_end, "Version:");
            char *arch = extract_register_field(block_start, block_end, "Architecture:");
            char *mach = extract_register_field(block_start, block_end, "Machine:");

            if (!ver && !arch && !mach)
                continue;

            if (count >= capacity) {
                capacity = capacity ? capacity * 2 : 4;
                tmp = realloc(list, capacity * sizeof(struct wcr_pkg_variant_s));
                if (!tmp) {
                    free(ver); free(arch); free(mach);
                    free_variant_list(list, count);
                    return -1;
                }
                list = tmp;
            }

            list[count].version = ver;
            list[count].arch = arch;
            list[count].machine = mach;
            count++;
        }
    }

    *out = list;
    *out_count = count;
    return 0;
}

int parse_wcr_listing_variants(const char *listing,
                               struct wcr_pkg_variant_s **out, size_t *out_count)
{
    const char *line = listing;
    struct wcr_pkg_variant_s *list = NULL;
    size_t count = 0;
    size_t capacity = 0;

    *out = NULL;
    *out_count = 0;

    while (line && *line) {
        const char *next = strstr(line, "\r\n");
        char ver[128] = {0}, arch[128] = {0}, mach[128] = {0};
        const char *p;
        int field;
        size_t i;
        struct wcr_pkg_variant_s *tmp;

        if (!next)
            next = strchr(line, '\n');

        if (*line == '\r' || *line == '\n') {
            line = next ? next + (next[0] == '\r' ? 2 : 1) : NULL;
            continue;
        }

        p = line;
        field = 0;
        i = 0;
        while (*p && *p != '\r' && *p != '\n') {
            if (*p == ' ') {
                field++;
                i = 0;
            } else {
                if (field == 0 && i < sizeof(ver) - 1) ver[i++] = *p;
                else if (field == 1 && i < sizeof(arch) - 1) arch[i++] = *p;
                else if (field == 2 && i < sizeof(mach) - 1) mach[i++] = *p;
            }
            p++;
        }

        if (ver[0]) {
            if (count >= capacity) {
                capacity = capacity ? capacity * 2 : 4;
                tmp = realloc(list, capacity * sizeof(struct wcr_pkg_variant_s));
                if (!tmp) {
                    free_variant_list(list, count);
                    return -1;
                }
                list = tmp;
            }

            list[count].version = strdup(ver);
            list[count].arch = arch[0] ? strdup(arch) : NULL;
            list[count].machine = mach[0] ? strdup(mach) : NULL;
            count++;
        }

        line = next ? next + (next[0] == '\r' ? 2 : 1) : NULL;
    }

    *out = list;
    *out_count = count;
    return 0;
}

void free_variant_list(struct wcr_pkg_variant_s *list, size_t count)
{
    size_t i;

    if (!list)
        return;
    for (i = 0; i < count; i++) {
        free(list[i].version);
        free(list[i].arch);
        free(list[i].machine);
    }
    free(list);
}
