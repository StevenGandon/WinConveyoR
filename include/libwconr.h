#ifndef LIBWCONR_H_
    #define LIBWCONR_H_

    #include <stddef.h>
    #include "wcr_event.h"

    #ifdef _WIN32
        #ifndef WIN32_LEAN_AND_MEAN
            #define WIN32_LEAN_AND_MEAN
        #endif
        #include <windows.h>
        typedef CRITICAL_SECTION wcr_mutex;
    #else
        #include <pthread.h>
        typedef pthread_mutex_t wcr_mutex;
    #endif

    /* ==== enums ==== */

    /* architectures */
    enum SUPPORTED_ARCHITECTURES {
        UNSUPPORTED_ARCH = -(1 << 0),
        X86_64_ARCH = (1 << 0),
        ARM64_ARCH = (1 << 1),
        I686_ARCH = (1 << 2)
    };

    /* os */
    enum SUPPORTED_PLATFORMS {
        UNSUPPORTED_PLTF = -(1 << 0),
        NT_PLTF = (1 << 0),
        DARWIN_PLTF = (1 << 1),
        GEN_LINUX_PLTF = (1 << 2)
    };

    /* transport protocols */
    typedef enum {
        PROT_HTTP = 0,
        PROT_WCR = 1
    } protocol_type;

    /* ==== structs definition ==== */

    /* current system infos */
    struct wcr_system_s {
        short arch;     // system architecture
        short platform; // system operating system
    };

    /* a single configured source (mirror) */
    struct wcr_source_s {
        char *url;            // mirror URL
        protocol_type proto;  // transport protocol
    };

    /* current state datas of the program */
    struct wcr_state_s {
        struct wcr_system_s system_informations; // system information about current machine / target machine
        char *cache_path;                        // user-configured cache directory
        char *config_path;                       // path to config file for auto-save
        struct wcr_source_s **sources;           // configured mirrors
        size_t sources_count;                    // number of configured mirrors
        wcr_mutex lock;                          // mutex for thread-safe access
        wcr_event_callback_t event_callback;     // user-registered event callback (or NULL)
        void *event_user_data;                   // opaque pointer passed to callback
    };

    /* ==== types definition ==== */

    /* installed package entry */
    struct wcr_installed_pkg_s {
        char *name;
        char *version;
    };

    typedef struct wcr_state_s wcr_state;
    typedef struct wcr_system_s wcr_system;
    typedef struct wcr_source_s wcr_source;
    typedef struct wcr_installed_pkg_s wcr_installed_pkg;

    /* ==== high level interfaces ====  */

    struct wcr_state_s *new_state(void);
    void close_state(struct wcr_state_s *__s);
    int download_package(const unsigned char *http_address, const unsigned char *location);
    int write_state(const struct wcr_state_s *state, const char *filepath);
    struct wcr_state_s *load_state(const char *filepath);
    int wcr_state_add_source(struct wcr_state_s *state, protocol_type proto, const char *url);
    int sync_package_list(const struct wcr_state_s *state, protocol_type proto, const char *source_uri);
    int install_package(const struct wcr_state_s *state, protocol_type proto, const char *source_uri, const char *package_name);
    int uninstall_package(const struct wcr_state_s *state, const char *package_name);
    int record_installed(const struct wcr_state_s *state, const char *name, const char *version);
    int remove_installed(const struct wcr_state_s *state, const char *name);
    int list_installed(const struct wcr_state_s *state, struct wcr_installed_pkg_s **out, size_t *out_count);
    void free_installed_list(struct wcr_installed_pkg_s *list, size_t count);
    void wcr_set_event_callback(struct wcr_state_s *state, wcr_event_callback_t callback, void *user_data);

    /* ==== low level interfaces ==== */

#endif /* !LIBWCONR_H_ */
