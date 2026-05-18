#include "pkg_parsing.h"
#include "wcr_event_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cJSON.h>

int find_package_in_list(const char *pkgs_list_path, const char *package_name,
                          char **out_register_path, char **out_checksum)
{
    FILE *fp;
    char line[1024];
    size_t name_len;

    *out_register_path = NULL;
    *out_checksum = NULL;

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

        wcr_emit(NULL, WCR_EVENT_DEBUG, "[DEBUG] find_package_in_list: %s version=%s register=%s checksum=%s",
               package_name, version, *out_register_path, *out_checksum);

        fclose(fp);

        if (!*out_register_path || !*out_checksum) {
            if (*out_register_path) free(*out_register_path);
            if (*out_checksum) free(*out_checksum);
            *out_register_path = NULL;
            *out_checksum = NULL;
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
