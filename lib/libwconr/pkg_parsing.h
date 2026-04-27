#ifndef PKG_PARSING_H_
    #define PKG_PARSING_H_

    int find_package_in_list(const char *pkgs_list_path, const char *package_name,
                              char **out_register_path, char **out_checksum);
    int parse_first_location(const char *register_content, char **out_location);
    int json_extract_string(const char *json, const char *key, char **out_value);

#endif /* !PKG_PARSING_H_ */
