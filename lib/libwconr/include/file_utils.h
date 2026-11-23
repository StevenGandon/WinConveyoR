#ifndef FILE_UTILS_H_
    #define FILE_UTILS_H_

    #include "libwconr.h"

    #define READ_BUFFER_SIZE 4096

    char *get_cache_path(const struct wcr_state_s *state, const char *filename);
    char *calculate_sha256_file(const char *filepath);
    char *read_file_text(const char *path);
    int extract_archive(const struct wcr_state_s *state, const char *archive_path, const char *package_name);
    int mkdir_p(const char *path);
    int extract_archive_to(const struct wcr_state_s *state, const char *archive_path, const char *dest_dir);

#endif /* !FILE_UTILS_H_ */
