#ifndef FILE_UTILS_H_
    #define FILE_UTILS_H_

    #define READ_BUFFER_SIZE 4096

    char *get_cache_path(const char *filename);
    char *calculate_sha256_file(const char *filepath);

#endif /* !FILE_UTILS_H_ */
