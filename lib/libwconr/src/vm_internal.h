#ifndef VM_INTERNAL_H_
#define VM_INTERNAL_H_

#include "wizard_private.h"

enum vm_arg_type {
    VM_TYPE_STR,
    VM_TYPE_STR_ARRAY,
    VM_TYPE_U8,
    VM_TYPE_U16,
    VM_TYPE_U32,
    VM_TYPE_U64
};

struct vm_arg_value {
    enum vm_arg_type type;
    union {
        struct { char *data; uint32_t len; } str;
        struct { char **data; uint32_t *lens; uint32_t count; } str_array;
        uint8_t  u8_val;
        uint16_t u16_val;
        uint32_t u32_val;
        uint64_t u64_val;
    } v;
};

typedef int (*vm_handler_t)(struct vm_arg_value *args, uint32_t count);

vm_handler_t vm_find_handler(uint16_t version, const char *name);

#endif /* !VM_INTERNAL_H_ */
