#ifndef WIZARD_PRIVATE_H_
#define WIZARD_PRIVATE_H_

/**
 * @file wizard_private.h
 * @brief Internal structures and API for parsing and executing .WIZARD binary files.
 *
 * A .WIZARD file is a structured binary package script. It contains a file
 * header, a section header table, a variable number of sections (metadata,
 * build, install, uninstall, purge), and a string table (strndx).
 *
 * All multi-byte integer fields in the on-disk structs are big-endian.
 * Use the @c wizard_be* helpers to read them portably.
 */

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

/* ── Packing macros ──────────────────────────────────────────────────────── */

#ifdef _MSC_VER
#    define WIZARD_PACK_BEGIN __pragma(pack(push, 1))
#    define WIZARD_PACK_END   __pragma(pack(pop))
#    define WIZARD_PACKED
#else
#    define WIZARD_PACK_BEGIN
#    define WIZARD_PACK_END
#    define WIZARD_PACKED     __attribute__((packed))
#endif

/* ── Magic & size constants ──────────────────────────────────────────────── */

/** @brief Expected magic bytes at offset 0 of every .WIZARD file. */
#define WIZARD_MAGIC            "\x42\xa4\x09\x67"

/** @brief Length of the magic field in bytes. */
#define WIZARD_MAGIC_SIZE       4

/** @brief Total size of the on-disk file header in bytes. */
#define WIZARD_HEADER_SIZE      27

/** @brief Size of the fixed prefix of the section header (size + count fields). */
#define WIZARD_SECTION_HEADER_PREFIX    16

/** @brief Size of a single section entry in the section header table. */
#define WIZARD_SECTION_ENTRY_SIZE       28

/** @brief Size of the length field that precedes each string in the strndx. */
#define WIZARD_STRING_LENGTH_SIZE       4

/** @brief Maximum number of sections accepted during bounds validation. */
#define WIZARD_MAX_SECTIONS     1000

/* ── Endian helpers ──────────────────────────────────────────────────────── */

/**
 * @brief Reads a big-endian 16-bit unsigned integer from an unaligned buffer.
 * @param[in] p  Pointer to at least 2 bytes of data.
 * @return Host-endian value, or 0 if @p p is NULL.
 */
uint16_t wizard_be16(const unsigned char *p);

/**
 * @brief Reads a big-endian 32-bit unsigned integer from an unaligned buffer.
 * @param[in] p  Pointer to at least 4 bytes of data.
 * @return Host-endian value, or 0 if @p p is NULL.
 */
uint32_t wizard_be32(const unsigned char *p);

/**
 * @brief Reads a big-endian 64-bit unsigned integer from an unaligned buffer.
 * @param[in] p  Pointer to at least 8 bytes of data.
 * @return Host-endian value, or 0 if @p p is NULL.
 */
uint64_t wizard_be64(const unsigned char *p);

/* ── On-disk packed structs ──────────────────────────────────────────────── */

WIZARD_PACK_BEGIN

/**
 * @brief On-disk layout of the .WIZARD file header (27 bytes, big-endian fields).
 *
 * Always located at offset 0 of the file. All integer fields must be read
 * through the @c wizard_be* helpers to handle endianness correctly.
 */
struct _wizard_file_header_raw_s {
    unsigned char magic[4];                 /**< Magic number: must equal @ref WIZARD_MAGIC. */
    unsigned char endianness[1];            /**< 0 = big-endian file, 1 = little-endian file. */
    unsigned char version[2];              /**< Format version: high byte = major, low byte = minor. */
    unsigned char flags[4];               /**< Format flags: bit 0 = 8-byte alignment, bit 1 = 16-byte alignment. */
    unsigned char section_header_offset[8];/**< Absolute file offset to @ref _wizard_section_header_raw_s. */
    unsigned char strndx_offset[8];        /**< Absolute file offset to the string table. */
} WIZARD_PACKED;

/**
 * @brief On-disk layout of the section header prefix (16 bytes, big-endian fields).
 *
 * Located at the offset given by @c section_header_offset in the file header.
 * Immediately followed by @c section_count entries of @ref _wizard_section_entry_raw_s.
 */
struct _wizard_section_header_raw_s {
    unsigned char section_header_size[8];  /**< Total size of the section header block in bytes. */
    unsigned char section_count[8];        /**< Number of section entries that follow. */
} WIZARD_PACKED;

/**
 * @brief On-disk layout of a single section table entry (28 bytes, big-endian fields).
 *
 * Each entry describes one section: its name (via strndx), its type and
 * platform flags, its byte size, and its absolute offset in the file.
 */
struct _wizard_section_entry_raw_s {
    unsigned char name_strndx[4];   /**< Offset into the string table for this section's name. */
    unsigned char section_type[4];  /**< Section type (see TYPE_SECTION_* constants). */
    unsigned char section_flags[4]; /**< Platform flags: bit 0 = Windows, bit 1 = POSIX. */
    unsigned char section_size[8];  /**< Size of the section content in bytes. */
    unsigned char section_offset[8];/**< Absolute file offset to the section content. */
} WIZARD_PACKED;

/**
 * @brief On-disk layout of a string table entry header (4 bytes).
 *
 * Each entry in the string table begins with this 4-byte length field,
 * immediately followed by @c length raw bytes of string data (no NUL terminator
 * on disk; callers must copy and terminate as needed).
 */
struct _wizard_strndx_entry_raw_s {
    unsigned char length[4];    /**< Length in bytes of the string data that follows. */
} WIZARD_PACKED;

WIZARD_PACK_END

/* ── Runtime context ─────────────────────────────────────────────────────── */

/**
 * @brief Runtime context for an open .WIZARD file.
 *
 * Created by @ref wizard_open and destroyed by @ref wizard_close. The file is
 * memory-mapped for the lifetime of the context; all raw struct pointers are
 * views into the mapped region and must not be freed individually.
 */
struct _wizard_ctx_s {
    unsigned char *base;        /**< Base address of the memory-mapped file. */
    size_t file_size;           /**< Total size of the mapped file in bytes. */

    /** @brief Pointer into @p base at offset 0 (file header view). */
    struct _wizard_file_header_raw_s *header;

    /** @brief Pointer into @p base at @c section_header_offset (section header view). */
    struct _wizard_section_header_raw_s *section_header;

    /** @brief Pointer to the first section entry, immediately after @p section_header. */
    struct _wizard_section_entry_raw_s *sections;

    /** @brief Pointer into @p base at @c strndx_offset (string table base). */
    unsigned char *strndx_base;

    uint16_t version;       /**< Decoded format version (from file header). */
    uint32_t flags;         /**< Decoded format flags (from file header). */
    uint64_t section_count; /**< Decoded number of sections (from section header). */

#ifndef _WIN32
    int fd;                 /**< File descriptor kept open for the duration of the mmap (POSIX only). */
#endif
};

/* ── File lifecycle ──────────────────────────────────────────────────────── */

/**
 * @brief Opens and memory-maps a .WIZARD file, returning a validated context.
 *
 * Validates the magic number, version range, and section table bounds before
 * returning. The file remains mapped until @ref wizard_close is called.
 *
 * @param[in] path  Path to the .WIZARD file on disk.
 * @return Pointer to a new context, or NULL on any failure (file not found,
 *         invalid magic, bounds violation, allocation error, etc.).
 */
struct _wizard_ctx_s *wizard_open(const char *path);

/**
 * @brief Closes a .WIZARD context and releases all associated resources.
 *
 * Unmaps the file, closes the file descriptor (POSIX), and frees the context.
 *
 * @param[in] ctx  Context to close. No-op if NULL.
 */
void wizard_close(struct _wizard_ctx_s *ctx);

/* ── Context accessors ───────────────────────────────────────────────────── */

/**
 * @brief Returns the format version stored in the file header.
 * @param[in] ctx  Open wizard context.
 * @return Format version, or 0 if @p ctx is NULL.
 */
uint16_t wizard_get_version(const struct _wizard_ctx_s *ctx);

/**
 * @brief Returns the flags field stored in the file header.
 * @param[in] ctx  Open wizard context.
 * @return Flags bitmask, or 0 if @p ctx is NULL.
 */
uint32_t wizard_get_flags(const struct _wizard_ctx_s *ctx);

/**
 * @brief Returns the total number of sections in the file.
 * @param[in] ctx  Open wizard context.
 * @return Section count, or 0 if @p ctx is NULL.
 */
uint64_t wizard_get_section_count(const struct _wizard_ctx_s *ctx);

/* ── Section accessors ───────────────────────────────────────────────────── */

/**
 * @brief Returns a pointer to the Nth section entry.
 *
 * The returned pointer is a view into the memory-mapped file; do not free it.
 *
 * @param[in] ctx    Open wizard context.
 * @param[in] index  Zero-based section index.
 * @return Pointer to the section entry, or NULL if out of range or @p ctx is NULL.
 */
const struct _wizard_section_entry_raw_s *wizard_get_section(
    const struct _wizard_ctx_s *ctx, uint64_t index);

/**
 * @brief Decodes the type field of a section entry.
 * @param[in] section  Section entry to read.
 * @return Section type value, or 0 if @p section is NULL.
 */
uint32_t wizard_get_section_type(const struct _wizard_section_entry_raw_s *section);

/**
 * @brief Decodes the flags field of a section entry.
 * @param[in] section  Section entry to read.
 * @return Section flags bitmask, or 0 if @p section is NULL.
 */
uint32_t wizard_get_section_flags(const struct _wizard_section_entry_raw_s *section);

/**
 * @brief Finds the first section whose name matches @p name.
 *
 * Comparison is by byte-exact match of the raw string data; no NUL terminator
 * is assumed in the string table.
 *
 * @param[in] ctx   Open wizard context.
 * @param[in] name  NUL-terminated name to search for.
 * @return Pointer to the matching section entry, or NULL if not found.
 */
const struct _wizard_section_entry_raw_s *wizard_get_section_by_name(
    const struct _wizard_ctx_s *ctx, const char *name);

/**
 * @brief Returns a pointer to the raw content bytes of a section.
 *
 * The pointer is a view into the memory-mapped file; do not free it.
 * Validates that the section data lies within the mapped file bounds.
 *
 * @param[in]  ctx       Open wizard context.
 * @param[in]  section   Section entry to query.
 * @param[out] data_out  Set to the start of the section content on success.
 * @param[out] size_out  Set to the section size in bytes on success.
 * @return 0 on success, -1 if any argument is NULL or bounds are violated.
 */
int wizard_get_section_data(
    const struct _wizard_ctx_s *ctx,
    const struct _wizard_section_entry_raw_s *section,
    const unsigned char **data_out,
    uint64_t *size_out);

/* ── String table accessors ──────────────────────────────────────────────── */

/**
 * @brief Resolves a string table entry by its byte offset.
 *
 * The returned pointer is a non-owning view into the memory-mapped file.
 * The string is not NUL-terminated on disk; use @p len_out to bound reads.
 *
 * @param[in]  ctx            Open wizard context.
 * @param[in]  strndx_offset  Byte offset from @c strndx_base to the desired entry.
 * @param[out] str_out        Set to the start of the string bytes on success.
 * @param[out] len_out        Set to the string length in bytes on success.
 * @return 0 on success, -1 if any argument is NULL or bounds are violated.
 */
int wizard_get_string(
    const struct _wizard_ctx_s *ctx,
    uint32_t strndx_offset,
    const char **str_out,
    uint32_t *len_out);

/**
 * @brief Resolves the name of a section through the string table.
 *
 * Reads the @c name_strndx field of @p section and calls @ref wizard_get_string.
 *
 * @param[in]  ctx      Open wizard context.
 * @param[in]  section  Section entry whose name to resolve.
 * @param[out] name_out Set to the start of the name bytes on success.
 * @param[out] len_out  Set to the name length in bytes on success.
 * @return 0 on success, -1 on any failure.
 */
int wizard_get_section_name(
    const struct _wizard_ctx_s *ctx,
    const struct _wizard_section_entry_raw_s *section,
    const char **name_out,
    uint32_t *len_out);

/* ── VM forward declarations ─────────────────────────────────────────────── */

/** @brief Opaque handle to the wizard virtual machine. Defined in vm_internal.h. */
struct _wizard_vm_s;

/**
 * @brief Creates and initialises a wizard VM by loading instruction XML files
 *        from @p instructions_dir.
 *
 * Loads every XML file listed in the internal @c vm_xml_files table. At least
 * one instruction set must load successfully; otherwise NULL is returned.
 *
 * @param[in] instructions_dir  Directory containing instruction XML files.
 * @return Pointer to a new VM, or NULL on failure.
 */
struct _wizard_vm_s *wizard_vm_init(const char *instructions_dir);

/**
 * @brief Destroys a wizard VM and frees all associated instruction data.
 * @param[in] vm  VM to destroy. No-op if NULL.
 */
void wizard_vm_close(struct _wizard_vm_s *vm);

/**
 * @brief Executes the bytecode contained in a section using the wizard VM.
 *
 * Selects the instruction set whose version matches the file's format version,
 * then interprets each instruction sequentially. Execution stops on the first
 * unrecognised opcode or handler error.
 *
 * @param[in] vm       Initialised wizard VM.
 * @param[in] ctx      Open wizard context providing the string table and file data.
 * @param[in] section  Section entry whose content will be executed.
 * @return 0 on success, -1 on any failure.
 */
int wizard_vm_exec(
    struct _wizard_vm_s *vm,
    struct _wizard_ctx_s *ctx,
    const struct _wizard_section_entry_raw_s *section);

/**
 * @brief Prints all loaded instruction sets and their opcodes to stdout.
 *
 * Intended for debugging. Prints each version's opcode list with name and
 * argument types.
 *
 * @param[in] vm  Initialised wizard VM.
 */
void wizard_vm_dump_instructions(const struct _wizard_vm_s *vm);

/**
 * @brief Disassembles a section's bytecode and prints it to stdout.
 *
 * Decodes each instruction in order, printing its file offset, index,
 * mnemonic name, and decoded argument values.
 *
 * @param[in] vm       Initialised wizard VM.
 * @param[in] ctx      Open wizard context.
 * @param[in] section  Section to disassemble.
 * @return 0 on success, -1 on any decode failure.
 */
int wizard_vm_disasm(
    struct _wizard_vm_s *vm,
    struct _wizard_ctx_s *ctx,
    const struct _wizard_section_entry_raw_s *section);

#endif /* !WIZARD_PRIVATE_H_ */
