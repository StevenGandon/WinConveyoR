#ifndef SOURCE_METADATA_UPDATE_H
    #define SOURCE_METADATA_UPDATE_H
    
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <curl/curl.h>
    #include <openssl/sha.h>
    
    #ifdef _WIN32
        #include <windows.h>
        #include <sys/stat.h>
        #define stat _stat
        #define access _access
        #define F_OK 0
    #else
        #include <unistd.h>
        #include <sys/stat.h>
    #endif
    
    #define READ_BUFFER_SIZE 4096
    
    struct memory_buffer_s {
        unsigned char *data;
        size_t size;
    };
    
    struct sync_resources_s {
        char *pkgs_list_url;
        char *checksum_url;
        char *pkgs_list_path;
        char *remote_checksum;
        char *local_checksum;
    };
    
    static char* get_cache_path(const char *filename);
    static char* calculate_sha256_file(const char *filepath);
    static size_t write_memory_callback(void *contents, size_t size, size_t nmemb, void *userp);
    static char* download_to_string(const char *url);
    static int download_to_file(const char *url, const char *filepath);
    static int cleanup_sync_resources(struct sync_resources_s *res, int result);
    int sync_package_list(const char *source_uri);

#endif /* SOURCE_METADATA_UPDATE_H */
