#ifndef FILE_UTILS_H_
    #define FILE_UTILS_H_

    #include "libwconr.h"

    #define READ_BUFFER_SIZE 4096

    char *get_cache_path(const struct wcr_state_s *state, const char *filename);
    char *calculate_sha256_file(const char *filepath);
    int extract_archive(const struct wcr_state_s *state, const char *archive_path, const char *package_name);

#endif /* !FILE_UTILS_H_ */
