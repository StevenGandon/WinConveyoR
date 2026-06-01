#ifndef PKG_PARSING_H_
    #define PKG_PARSING_H_

    #include <stddef.h>

    int find_package_in_list(const char *pkgs_list_path, const char *package_name,
                              char **out_register_path, char **out_checksum);
    int parse_first_location(const char *register_content, char **out_location);
    int json_extract_string(const char *json, const char *key, char **out_value);
    int json_extract_string_array(const char *json, const char *key,
                                  char ***out_values, size_t *out_count);
    void free_string_array(char **values, size_t count);

#endif /* !PKG_PARSING_H_ */
