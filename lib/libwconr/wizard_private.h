#ifndef WIZARD_PRIVATE_H_
    #define WIZARD_PRIVATE_H_

    #include <stddef.h>
    #include <stdint.h>
    #include <stdlib.h>
    #include <string.h>
    #include <stdio.h>

    #ifdef _WIN32
    #    include <windows.h>
    #else
    #    include <fcntl.h>
    #    include <sys/mman.h>
    #    include <sys/stat.h>
    #    include <unistd.h>
    #endif

    #ifdef _MSC_VER
    #    define WIZARD_PACK_BEGIN __pragma(pack(push, 1))
    #    define WIZARD_PACK_END   __pragma(pack(pop))
    #    define WIZARD_PACKED
    #else
    #    define WIZARD_PACK_BEGIN
    #    define WIZARD_PACK_END
    #    define WIZARD_PACKED     __attribute__((packed))
    #endif

    #define WIZARD_MAGIC "\x42\xa4\x09\x67"
    #define WIZARD_MAGIC_SIZE 4

    #define WIZARD_HEADER_SIZE              27
    #define WIZARD_SECTION_HEADER_PREFIX    16
    #define WIZARD_SECTION_ENTRY_SIZE       28
    #define WIZARD_STRING_LENGTH_SIZE       4
    #define WIZARD_MAX_SECTIONS             1000

    uint16_t wizard_be16(const unsigned char *p);
    uint32_t wizard_be32(const unsigned char *p);
    uint64_t wizard_be64(const unsigned char *p);

    WIZARD_PACK_BEGIN

    struct _wizard_file_header_raw_s {
        unsigned char magic[4];
        unsigned char endianness[1];
        unsigned char version[2];
        unsigned char flags[4];
        unsigned char section_header_offset[8];
        unsigned char strndx_offset[8];
    } WIZARD_PACKED;

    struct _wizard_section_header_raw_s {
        unsigned char section_header_size[8];
        unsigned char section_count[8];
    } WIZARD_PACKED;

    struct _wizard_section_entry_raw_s {
        unsigned char name_strndx[4];
        unsigned char section_type[4];
        unsigned char section_flags[4];
        unsigned char section_size[8];
        unsigned char section_offset[8];
    } WIZARD_PACKED;

    struct _wizard_strndx_entry_raw_s {
        unsigned char length[4];
    } WIZARD_PACKED;

    WIZARD_PACK_END

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

#ifndef _WIN32
        int fd;
#endif
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
