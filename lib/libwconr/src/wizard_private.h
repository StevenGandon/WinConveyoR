#ifndef WIZARD_PRIVATE_H_
    #define WIZARD_PRIVATE_H_

    #include <stddef.h>
    #include <stdint.h>
    #include <stdlib.h>
    #include <string.h>
    #include <sys/mman.h>
    #include <sys/stat.h>
    #include <fcntl.h>
    #include <unistd.h>

    #define WIZARD_MAGIC_0 0x42
    #define WIZARD_MAGIC_1 0xa4
    #define WIZARD_MAGIC_2 0x09
    #define WIZARD_MAGIC_3 0x67

    #define WIZARD_HEADER_SIZE              27
    #define WIZARD_SECTION_HEADER_PREFIX    16
    #define WIZARD_SECTION_ENTRY_SIZE       28
    #define WIZARD_STRING_LENGTH_SIZE       4

    static inline uint16_t wizard_be16(const unsigned char *p)
    {
        return (uint16_t)(((uint16_t)p[0] << 8) | (uint16_t)p[1]);
    }

    static inline uint32_t wizard_be32(const unsigned char *p)
    {
        return ((uint32_t)p[0] << 24) |
               ((uint32_t)p[1] << 16) |
               ((uint32_t)p[2] << 8)  |
               ((uint32_t)p[3]);
    }

    static inline uint64_t wizard_be64(const unsigned char *p)
    {
        return ((uint64_t)p[0] << 56) |
               ((uint64_t)p[1] << 48) |
               ((uint64_t)p[2] << 40) |
               ((uint64_t)p[3] << 32) |
               ((uint64_t)p[4] << 24) |
               ((uint64_t)p[5] << 16) |
               ((uint64_t)p[6] << 8)  |
               ((uint64_t)p[7]);
    }

    struct _wizard_file_header_raw_s {
        unsigned char magic[4];
        unsigned char endianness[1];
        unsigned char version[2];
        unsigned char flags[4];
        unsigned char section_header_offset[8];
        unsigned char strndx_offset[8];
    } __attribute__((packed));

    struct _wizard_section_header_raw_s {
        unsigned char section_header_size[8];
        unsigned char section_count[8];
    } __attribute__((packed));

    struct _wizard_section_entry_raw_s {
        unsigned char name_strndx[4];
        unsigned char section_type[4];
        unsigned char section_flags[4];
        unsigned char section_size[8];
        unsigned char section_offset[8];
    } __attribute__((packed));

    struct _wizard_strndx_entry_raw_s {
        unsigned char length[4];
    } __attribute__((packed));

    struct _wizard_ctx_s {
        unsigned char *base;
        size_t file_size;

        struct _wizard_file_header_raw_s *header;
        struct _wizard_section_header_raw_s *section_header;
        struct _wizard_section_entry_raw_s *sections;
        unsigned char *strndx_base;

        uint16_t version;
        uint32_t flags;
        uint64_t section_count;

        int fd;
    };

    struct _wizard_ctx_s *wizard_open(const char *path);
    void wizard_close(struct _wizard_ctx_s *ctx);

    uint16_t wizard_get_version(const struct _wizard_ctx_s *ctx);
    uint32_t wizard_get_flags(const struct _wizard_ctx_s *ctx);
    uint64_t wizard_get_section_count(const struct _wizard_ctx_s *ctx);

    const struct _wizard_section_entry_raw_s *wizard_get_section(
        const struct _wizard_ctx_s *ctx,
        uint64_t index);

    uint32_t wizard_get_section_type(
        const struct _wizard_section_entry_raw_s *section);

    uint32_t wizard_get_section_flags(
        const struct _wizard_section_entry_raw_s *section);

    const struct _wizard_section_entry_raw_s *wizard_get_section_by_name(
        const struct _wizard_ctx_s *ctx,
        const char *name);

    int wizard_get_section_data(
        const struct _wizard_ctx_s *ctx,
        const struct _wizard_section_entry_raw_s *section,
        const unsigned char **data_out,
        uint64_t *size_out);

    int wizard_get_string(
        const struct _wizard_ctx_s *ctx,
        uint32_t strndx_offset,
        const char **str_out,
        uint32_t *len_out);

    int wizard_get_section_name(
        const struct _wizard_ctx_s *ctx,
        const struct _wizard_section_entry_raw_s *section,
        const char **name_out,
        uint32_t *len_out);

    struct _wizard_vm_s;

    struct _wizard_vm_s *wizard_vm_init(const char *instructions_dir);
    void wizard_vm_close(struct _wizard_vm_s *vm);

    int wizard_vm_exec(
        struct _wizard_vm_s *vm,
        struct _wizard_ctx_s *ctx,
        const struct _wizard_section_entry_raw_s *section);

    void wizard_vm_dump_instructions(const struct _wizard_vm_s *vm);

    int wizard_vm_disasm(
        struct _wizard_vm_s *vm,
        struct _wizard_ctx_s *ctx,
        const struct _wizard_section_entry_raw_s *section);

#endif
