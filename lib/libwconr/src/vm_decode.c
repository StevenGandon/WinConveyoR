#include "vm_internal.h"

static void vm_free_str_partial(char **data, uint32_t *lens, uint32_t count)
{
    uint32_t j;

    for (j = 0; j < count; j++)
        (void)free(data[j]);
    (void)free(data);
    (void)free(lens);
}

static int vm_decode_str(const struct _wizard_ctx_s *ctx,
                         const unsigned char *data, uint64_t size,
                         uint64_t *ip, struct vm_arg_value *out, int is_array)
{
    const char *s;
    uint32_t slen;
    uint32_t strndx;
    uint32_t count;
    uint32_t i;

    if (is_array) {
        if (*ip + 4 > size) return (-1);
        count = wizard_be32(data + *ip);
        *ip += 4;
    } else {
        count = 1;
    }

    out->count = count;

    if (count == 0) {
        out->v.str.data = NULL;
        out->v.str.lens = NULL;
        return (0);
    }

    out->v.str.data = (char **)malloc(sizeof(char *) * count);
    out->v.str.lens = (uint32_t *)malloc(sizeof(uint32_t) * count);
    if (!out->v.str.data || !out->v.str.lens) {
        (void)free(out->v.str.data);
        (void)free(out->v.str.lens);
        return (-1);
    }

    for (i = 0; i < count; i++) {
        if (*ip + 4 > size) {
            vm_free_str_partial(out->v.str.data, out->v.str.lens, i);
            return (-1);
        }
        strndx = wizard_be32(data + *ip);
        *ip += 4;
        if (wizard_get_string(ctx, strndx, &s, &slen) != 0) {
            vm_free_str_partial(out->v.str.data, out->v.str.lens, i);
            return (-1);
        }
        out->v.str.data[i] = (char *)malloc(slen + 1);
        if (!out->v.str.data[i]) {
            vm_free_str_partial(out->v.str.data, out->v.str.lens, i);
            return (-1);
        }
        memcpy(out->v.str.data[i], s, slen);
        out->v.str.data[i][slen] = '\0';
        out->v.str.lens[i] = slen;
    }
    return (0);
}

static int vm_decode_u8(const unsigned char *data, uint64_t size,
                        uint64_t *ip, struct vm_arg_value *out, int is_array)
{
    uint32_t count;
    uint32_t i;

    if (is_array) {
        if (*ip + 4 > size) return (-1);
        count = wizard_be32(data + *ip);
        *ip += 4;
    } else {
        count = 1;
    }
    out->count = count;
    out->v.u8  = (uint8_t *)malloc(sizeof(uint8_t) * count);
    if (!out->v.u8) return (-1);
    for (i = 0; i < count; i++) {
        if (*ip + 1 > size) { (void)free(out->v.u8); return (-1); }
        out->v.u8[i] = data[*ip];
        *ip += 1;
    }
    return (0);
}

static int vm_decode_u16(const unsigned char *data, uint64_t size,
                         uint64_t *ip, struct vm_arg_value *out, int is_array)
{
    uint32_t count;
    uint32_t i;

    if (is_array) {
        if (*ip + 4 > size) return (-1);
        count = wizard_be32(data + *ip);
        *ip += 4;
    } else {
        count = 1;
    }
    out->count = count;
    out->v.u16 = (uint16_t *)malloc(sizeof(uint16_t) * count);
    if (!out->v.u16) return (-1);
    for (i = 0; i < count; i++) {
        if (*ip + 2 > size) { (void)free(out->v.u16); return (-1); }
        out->v.u16[i] = wizard_be16(data + *ip);
        *ip += 2;
    }
    return (0);
}

static int vm_decode_u32(const unsigned char *data, uint64_t size,
                         uint64_t *ip, struct vm_arg_value *out, int is_array)
{
    uint32_t count;
    uint32_t i;

    if (is_array) {
        if (*ip + 4 > size) return (-1);
        count = wizard_be32(data + *ip);
        *ip += 4;
    } else {
        count = 1;
    }
    out->count = count;
    out->v.u32 = (uint32_t *)malloc(sizeof(uint32_t) * count);
    if (!out->v.u32) return (-1);
    for (i = 0; i < count; i++) {
        if (*ip + 4 > size) { (void)free(out->v.u32); return (-1); }
        out->v.u32[i] = wizard_be32(data + *ip);
        *ip += 4;
    }
    return (0);
}

static int vm_decode_u64(const unsigned char *data, uint64_t size,
                         uint64_t *ip, struct vm_arg_value *out, int is_array)
{
    uint32_t count;
    uint32_t i;

    if (is_array) {
        if (*ip + 4 > size) return (-1);
        count = wizard_be32(data + *ip);
        *ip += 4;
    } else {
        count = 1;
    }
    out->count = count;
    out->v.u64 = (uint64_t *)malloc(sizeof(uint64_t) * count);
    if (!out->v.u64) return (-1);
    for (i = 0; i < count; i++) {
        if (*ip + 8 > size) { (void)free(out->v.u64); return (-1); }
        out->v.u64[i] = wizard_be64(data + *ip);
        *ip += 8;
    }
    return (0);
}

int vm_decode_arg(const struct _wizard_ctx_s *ctx,
                  const unsigned char *data, uint64_t size,
                  uint64_t *ip, struct vm_arg_type type,
                  struct vm_arg_value *out)
{
    if (!ctx || !data || !ip || !out)
        return (-1);

    memset(out, 0, sizeof(*out));
    out->type = type;

    switch (type.base) {
        case VM_BASE_STR:  return vm_decode_str(ctx, data, size, ip, out, type.is_array);
        case VM_BASE_U8:   return vm_decode_u8(data, size, ip, out, type.is_array);
        case VM_BASE_U16:  return vm_decode_u16(data, size, ip, out, type.is_array);
        case VM_BASE_U32:  return vm_decode_u32(data, size, ip, out, type.is_array);
        case VM_BASE_U64:  return vm_decode_u64(data, size, ip, out, type.is_array);
    }

    return (-1);
}

void vm_free_args(struct vm_arg_value *args, uint32_t count)
{
    uint32_t i;
    uint32_t j;

    if (!args)
        return;

    for (i = 0; i < count; i++) {
        switch (args[i].type.base) {
            case VM_BASE_STR:
                if (args[i].v.str.data) {
                    for (j = 0; j < args[i].count; j++)
                        if (args[i].v.str.data[j])
                            (void)free(args[i].v.str.data[j]);
                    (void)free(args[i].v.str.data);
                }
                if (args[i].v.str.lens)
                    (void)free(args[i].v.str.lens);
                break;
            case VM_BASE_U8:
                if (args[i].v.u8)  (void)free(args[i].v.u8);
                break;
            case VM_BASE_U16:
                if (args[i].v.u16) (void)free(args[i].v.u16);
                break;
            case VM_BASE_U32:
                if (args[i].v.u32) (void)free(args[i].v.u32);
                break;
            case VM_BASE_U64:
                if (args[i].v.u64) (void)free(args[i].v.u64);
                break;
        }
    }
}
