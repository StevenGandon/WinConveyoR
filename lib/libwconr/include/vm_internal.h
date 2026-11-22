#ifndef VM_INTERNAL_H_
#define VM_INTERNAL_H_

/**
 * @file vm_internal.h
 * @brief Internal types and API for the wizard bytecode virtual machine.
 *
 * The wizard VM interprets the bytecode stored in the code sections of a
 * .WIZARD file (build, install, uninstall, purge). An instruction set is
 * loaded from an XML file at VM initialisation time; at runtime each opcode
 * is decoded, its arguments resolved through the string table, and a
 * registered C handler is called to perform the actual operation.
 *
 * This header is private to libwconr and must not be included by consumers
 * of the public API.
 */

#include "wizard_private.h"

/* ── Argument type system ────────────────────────────────────────────────── */

/**
 * @brief Base data types supported by VM instruction arguments.
 *
 * String arguments are resolved through the .WIZARD string table at decode
 * time. Integer types are read directly from the bytecode stream.
 */
enum vm_base_type {
    VM_BASE_STR,    /**< String — decoded as a strndx offset, resolved to a char*. */
    VM_BASE_U8,     /**< Unsigned 8-bit integer. */
    VM_BASE_U16,    /**< Unsigned 16-bit integer (used for chmod mode fields). */
    VM_BASE_U32,    /**< Unsigned 32-bit integer. */
    VM_BASE_U64,    /**< Unsigned 64-bit integer. */
};

/**
 * @brief Full type descriptor for a VM argument.
 *
 * Combines a base type with an array flag. When @p is_array is non-zero the
 * argument is prefixed in the bytecode stream by a 4-byte element count
 * (see @ref WizardArgumentArray).
 */
struct vm_arg_type {
    enum vm_base_type base;     /**< Scalar base type. */
    int               is_array; /**< Non-zero if the argument is an array of @p base. */
};

/* ── Decoded argument value ──────────────────────────────────────────────── */

/**
 * @brief A decoded argument value produced by @ref vm_decode_arg.
 *
 * The active union member is determined by @p type.base. When @p type.is_array
 * is non-zero, @p count gives the number of elements and each pointer addresses
 * a heap-allocated array of that many values. When not an array, @p count is 1
 * and the pointer addresses a single-element heap-allocated array.
 *
 * Call @ref vm_free_args to release all memory owned by decoded argument arrays.
 */
struct vm_arg_value {
    struct vm_arg_type type;    /**< Type descriptor that governs which union member is valid. */
    uint32_t           count;   /**< Number of elements (1 for scalars, N for arrays). */

    union {
        /** @brief String data: parallel arrays of C-string pointers and their lengths. */
        struct {
            char     **data;    /**< Heap-allocated array of @p count NUL-terminated strings. */
            uint32_t  *lens;    /**< Heap-allocated array of @p count string byte lengths. */
        } str;
        uint8_t  *u8;   /**< Heap-allocated array of @p count uint8_t values. */
        uint16_t *u16;  /**< Heap-allocated array of @p count uint16_t values (big-endian decoded). */
        uint32_t *u32;  /**< Heap-allocated array of @p count uint32_t values (big-endian decoded). */
        uint64_t *u64;  /**< Heap-allocated array of @p count uint64_t values (big-endian decoded). */
    } v;
};

/* ── Handler function type ───────────────────────────────────────────────── */

/**
 * @brief Prototype for a VM instruction handler function.
 *
 * Each opcode is mapped to a handler of this type. The handler receives the
 * fully decoded argument array and must not retain any pointer after returning.
 *
 * @param[in] args   Array of @p count decoded argument values.
 * @param[in] count  Number of elements in @p args.
 * @return 0 on success, -1 on any failure. A non-zero return aborts execution
 *         of the current section.
 */
typedef int (*vm_handler_t)(struct vm_arg_value *args, uint32_t count);

/* ── Handler registry ────────────────────────────────────────────────────── */

/**
 * @brief Maps an instruction name to its handler function.
 *
 * The table is terminated by an entry whose @p name field is NULL.
 */
struct vm_handler_entry {
    const char   *name;     /**< NUL-terminated instruction mnemonic (e.g. "mkdir"). */
    vm_handler_t  handler;  /**< Corresponding handler function. */
};

/**
 * @brief Associates a handler table with a format version.
 *
 * The global handler registry is a NULL-table-terminated array of these structs,
 * sorted by ascending version. The VM picks the entry with the highest version
 * that does not exceed the file's format version.
 */
struct vm_handler_version {
    uint16_t                         version;   /**< Format version this table applies to. */
    const struct vm_handler_entry   *table;     /**< Handler table, or NULL to mark end of registry. */
};

/**
 * @brief Looks up the handler for the given instruction name and format version.
 *
 * Selects the most recent handler table whose version does not exceed @p version,
 * then searches linearly for a matching name.
 *
 * @param[in] version  Format version of the .WIZARD file being executed.
 * @param[in] name     NUL-terminated instruction mnemonic to look up.
 * @return Pointer to the handler function, or NULL if not found.
 */
vm_handler_t vm_find_handler(uint16_t version, const char *name);

/* ── Instruction set definition ──────────────────────────────────────────── */

/** @brief Maximum size for path and command-line buffers used by VM handlers. */
#define VM_BUF_SIZE 4096

/**
 * @brief Defines a single formal argument of an instruction (from XML).
 *
 * Used at decode time to select the correct @ref vm_base_type and at
 * disassembly time to label printed arguments.
 */
struct vm_arg_def {
    struct vm_arg_type type;    /**< Type of this argument (base type + array flag). */
    char              *label;   /**< Heap-allocated human-readable argument label from XML. */
};

/**
 * @brief Defines a single opcode within an instruction set (from XML).
 */
struct vm_op_def {
    char             *name;         /**< Heap-allocated instruction mnemonic (e.g. "copy"). */
    uint16_t          code;         /**< Numeric opcode as it appears in the bytecode stream. */
    struct vm_arg_def *args;        /**< Heap-allocated array of @p arg_count argument definitions. */
    uint32_t          arg_count;    /**< Number of formal arguments for this instruction. */
};

/**
 * @brief A complete instruction set loaded from a single XML file.
 *
 * An instruction set is versioned and contains all opcodes valid for that
 * format version.
 */
struct vm_iset_s {
    uint16_t          version;      /**< Format version this instruction set targets. */
    struct vm_op_def *ops;          /**< Heap-allocated array of @p op_count opcode definitions. */
    uint32_t          op_count;     /**< Number of opcodes in @p ops. */
};

/**
 * @brief Top-level wizard VM instance.
 *
 * Holds one or more instruction sets loaded from the XML files found in the
 * instructions directory supplied to @ref wizard_vm_init.
 */
struct _wizard_vm_s {
    struct vm_iset_s *isets;        /**< Heap-allocated array of @p iset_count instruction sets. */
    uint32_t          iset_count;   /**< Number of loaded instruction sets. */
};

/* ── vm_parse.c ──────────────────────────────────────────────────────────── */

/**
 * @brief Parses a wizard instruction XML file and fills an instruction set.
 *
 * Accepts either a root @c \<wizard\> element (with a nested @c \<instructions\>
 * child) or a bare @c \<instructions\> root. The @c version attribute of the
 * @c \<instructions\> element is required.
 *
 * @param[in]  path  Path to the XML file on disk.
 * @param[out] iset  Instruction set struct to populate. Must be pre-allocated
 *                   by the caller; its fields will be initialised here.
 * @return 0 on success (at least one opcode loaded), -1 on any parse or
 *         allocation failure.
 */
int vm_parse_xml(const char *path, struct vm_iset_s *iset);

/* ── vm_decode.c ─────────────────────────────────────────────────────────── */

/**
 * @brief Decodes a single instruction argument from the bytecode stream.
 *
 * Advances @p ip by the number of bytes consumed. For array arguments, reads
 * the 4-byte element count first. String arguments are resolved through the
 * .WIZARD string table in @p ctx; all other types are read directly from
 * @p data.
 *
 * The caller must release the decoded value with @ref vm_free_args when done.
 *
 * @param[in]  ctx   Open wizard context (needed for string table lookups).
 * @param[in]  data  Pointer to the start of the current section's bytecode.
 * @param[in]  size  Total size of @p data in bytes.
 * @param[in,out] ip Instruction pointer: byte offset of the next byte to decode.
 *                   Advanced by this function.
 * @param[in]  type  Type descriptor indicating how to decode the argument.
 * @param[out] out   Populated with the decoded value on success.
 * @return 0 on success, -1 on bounds violation, unknown type, or allocation
 *         failure.
 */
int vm_decode_arg(const struct _wizard_ctx_s *ctx,
                  const unsigned char *data, uint64_t size,
                  uint64_t *ip, struct vm_arg_type type,
                  struct vm_arg_value *out);

/**
 * @brief Frees all heap memory owned by an array of decoded argument values.
 *
 * Releases string pointer arrays, length arrays, and integer arrays for each
 * of the first @p count entries. Does not free the @p args array itself.
 *
 * @param[in] args   Array of decoded argument values to free.
 * @param[in] count  Number of entries to process. No-op if @p args is NULL.
 */
void vm_free_args(struct vm_arg_value *args, uint32_t count);

#endif /* !VM_INTERNAL_H_ */
