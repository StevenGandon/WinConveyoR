#ifndef LIBWCONR_H_
    #define LIBWCONR_H_

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

    /* ==== structs definition ==== */

    /* current system infos */
    struct wcr_system_s {
        short arch;     // system architecture
        short platform; // system operating system
    };

    /* current state datas of the program */
    struct wcr_state_s {
        struct wcr_system_s system_informations; // system information about current machine / target machine
    };

    /* ==== types definition ==== */

    typedef struct wcr_state_s wcr_state;
    typedef struct wcr_system_s wcr_system;

    /* ==== high level interfaces ====  */

    struct wcr_state_s *new_state(void);
    void close_state(struct wcr_state_s *__s);

    /* ==== low level interfaces ==== */

#endif /* !LIBWCONR_H_ */
