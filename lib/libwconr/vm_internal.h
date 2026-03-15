#ifndef VM_INTERNAL_H_
    #define VM_INTERNAL_H_

    #include "wizard_private.h"

    enum vm_base_type {
        VM_BASE_STR,
        VM_BASE_U8,
        VM_BASE_U16,
        VM_BASE_U32,
        VM_BASE_U64
    };

    struct vm_arg_type {
        enum vm_base_type base;
        int               is_array;
    };

    struct vm_arg_value {
        struct vm_arg_type type;
        uint32_t           count;
        union {
            struct { char **data; uint32_t *lens; } str;
            uint8_t  *u8;
            uint16_t *u16;
            uint32_t *u32;
            uint64_t *u64;
        } v;
    };

    typedef int (*vm_handler_t)(struct vm_arg_value *args, uint32_t count);

    struct vm_handler_entry {
        const char *name;
        vm_handler_t handler;
    };

    struct vm_handler_version {
        uint16_t version;
        const struct vm_handler_entry *table;
    };

    vm_handler_t vm_find_handler(uint16_t version, const char *name);

    #define VM_BUF_SIZE 4096

    struct vm_arg_def {
        struct vm_arg_type type;
        char              *label;
    };

    struct vm_op_def {
        char *name;
        uint16_t code;
        struct vm_arg_def *args;
        uint32_t arg_count;
    };

    struct vm_iset_s {
        uint16_t version;
        struct vm_op_def *ops;
        uint32_t op_count;
    };

    struct _wizard_vm_s {
        struct vm_iset_s *isets;
        uint32_t iset_count;
    };

    /* vm_parse.c */
    int vm_parse_xml(const char *path, struct vm_iset_s *iset);

    /* vm_decode.c */
    int  vm_decode_arg(const struct _wizard_ctx_s *ctx,
                    const unsigned char *data, uint64_t size,
                    uint64_t *ip, struct vm_arg_type type,
                    struct vm_arg_value *out);
    void vm_free_args(struct vm_arg_value *args, uint32_t count);

#endif /* !VM_INTERNAL_H_ */
