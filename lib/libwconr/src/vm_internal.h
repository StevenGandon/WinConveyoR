#ifndef VM_INTERNAL_H_
    #define VM_INTERNAL_H_

    #include "wizard_private.h"

    enum vm_arg_type {
        VM_TYPE_STR,
        VM_TYPE_STR_ARRAY,
        VM_TYPE_U8,
        VM_TYPE_U16,
        VM_TYPE_U32,
        VM_TYPE_U64,
        VM_TYPE_U8_ARRAY,
        VM_TYPE_U16_ARRAY,
        VM_TYPE_U32_ARRAY,
        VM_TYPE_U64_ARRAY
    };

    struct vm_arg_value {
        enum vm_arg_type type;
        union {
            struct { char    *data; uint32_t len; }                   str;
            struct { char   **data; uint32_t *lens; uint32_t count; } str_array;
            struct { uint8_t  *data; uint32_t count; }                u8_array;
            struct { uint16_t *data; uint32_t count; }                u16_array;
            struct { uint32_t *data; uint32_t count; }                u32_array;
            struct { uint64_t *data; uint32_t count; }                u64_array;
            uint8_t  u8_val;
            uint16_t u16_val;
            uint32_t u32_val;
            uint64_t u64_val;
        } v;
    };

    typedef int (*vm_handler_t)(struct vm_arg_value *args, uint32_t count);

    vm_handler_t vm_find_handler(uint16_t version, const char *name);

    #define VM_BUF_SIZE 4096

    struct vm_arg_def {
        enum vm_arg_type type;
        char *label;
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
                    uint64_t *ip, enum vm_arg_type type,
                    struct vm_arg_value *out);
    void vm_free_args(struct vm_arg_value *args, uint32_t count);

#endif /* !VM_INTERNAL_H_ */
