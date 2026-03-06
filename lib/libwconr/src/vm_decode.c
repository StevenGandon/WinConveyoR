#include "vm_internal.h"

static int vm_decode_str(const struct _wizard_ctx_s *ctx,
                         const unsigned char *data, uint64_t size,
                         uint64_t *ip, struct vm_arg_value *out)
{
    const char *s;
    uint32_t slen;
    uint32_t strndx;

    if (*ip + 4 > size) return (-1);
    strndx = wizard_be32(data + *ip);
    *ip += 4;
    if (wizard_get_string(ctx, strndx, &s, &slen) != 0) return (-1);
    out->v.str.data = (char *)malloc(slen + 1);
    if (!out->v.str.data) return (-1);
    memcpy(out->v.str.data, s, slen);
    out->v.str.data[slen] = '\0';
    out->v.str.len = slen;
    return (0);
}

static int vm_decode_str_array(const struct _wizard_ctx_s *ctx,
                               const unsigned char *data, uint64_t size,
                               uint64_t *ip, struct vm_arg_value *out)
{
    const char *s;
    uint32_t slen;
    uint32_t strndx;
    uint32_t count;
    uint32_t i;
    uint32_t j;

    if (*ip + 4 > size) return (-1);
    count = wizard_be32(data + *ip);
    *ip += 4;
    out->v.str_array.count = count;

    if (count == 0) {
        out->v.str_array.data = NULL;
        out->v.str_array.lens = NULL;
        return (0);
    }

    out->v.str_array.data = (char **)malloc(sizeof(char *) * count);
    out->v.str_array.lens = (uint32_t *)malloc(sizeof(uint32_t) * count);
    if (!out->v.str_array.data || !out->v.str_array.lens) {
        (void)free(out->v.str_array.data);
        (void)free(out->v.str_array.lens);
        return (-1);
    }

    for (i = 0; i < count; i++) {
        if (*ip + 4 > size) goto fail;
        strndx = wizard_be32(data + *ip);
        *ip += 4;
        if (wizard_get_string(ctx, strndx, &s, &slen) != 0) goto fail;
        out->v.str_array.data[i] = (char *)malloc(slen + 1);
        if (!out->v.str_array.data[i]) goto fail;
        memcpy(out->v.str_array.data[i], s, slen);
        out->v.str_array.data[i][slen] = '\0';
        out->v.str_array.lens[i] = slen;
        continue;
    fail:
        for (j = 0; j < i; j++)
            (void)free(out->v.str_array.data[j]);
        (void)free(out->v.str_array.data);
        (void)free(out->v.str_array.lens);
        return (-1);
    }
    return (0);
}

static int vm_decode_u8(const unsigned char *data, uint64_t size,
                        uint64_t *ip, struct vm_arg_value *out)
{
    if (*ip + 1 > size) return (-1);
    out->v.u8_val = data[*ip];
    *ip += 1;
    return (0);
}

static int vm_decode_u16(const unsigned char *data, uint64_t size,
                         uint64_t *ip, struct vm_arg_value *out)
{
    if (*ip + 2 > size) return (-1);
    out->v.u16_val = wizard_be16(data + *ip);
    *ip += 2;
    return (0);
}

static int vm_decode_u32(const unsigned char *data, uint64_t size,
                         uint64_t *ip, struct vm_arg_value *out)
{
    if (*ip + 4 > size) return (-1);
    out->v.u32_val = wizard_be32(data + *ip);
    *ip += 4;
    return (0);
}

static int vm_decode_u64(const unsigned char *data, uint64_t size,
                         uint64_t *ip, struct vm_arg_value *out)
{
    if (*ip + 8 > size) return (-1);
    out->v.u64_val = wizard_be64(data + *ip);
    *ip += 8;
    return (0);
}

static int vm_decode_u8_array(const unsigned char *data, uint64_t size,
                              uint64_t *ip, struct vm_arg_value *out)
{
    uint32_t count;
    uint32_t i;

    if (*ip + 4 > size) return (-1);
    count = wizard_be32(data + *ip);
    *ip += 4;
    out->v.u8_array.count = count;
    out->v.u8_array.data = (uint8_t *)malloc(sizeof(uint8_t) * count);
    if (!out->v.u8_array.data) return (-1);
    for (i = 0; i < count; i++) {
        if (*ip + 1 > size) { (void)free(out->v.u8_array.data); return (-1); }
        out->v.u8_array.data[i] = data[*ip];
        *ip += 1;
    }
    return (0);
}

static int vm_decode_u16_array(const unsigned char *data, uint64_t size,
                               uint64_t *ip, struct vm_arg_value *out)
{
    uint32_t count;
    uint32_t i;

    if (*ip + 4 > size) return (-1);
    count = wizard_be32(data + *ip);
    *ip += 4;
    out->v.u16_array.count = count;
    out->v.u16_array.data = (uint16_t *)malloc(sizeof(uint16_t) * count);
    if (!out->v.u16_array.data) return (-1);
    for (i = 0; i < count; i++) {
        if (*ip + 2 > size) { (void)free(out->v.u16_array.data); return (-1); }
        out->v.u16_array.data[i] = wizard_be16(data + *ip);
        *ip += 2;
    }
    return (0);
}

static int vm_decode_u32_array(const unsigned char *data, uint64_t size,
                               uint64_t *ip, struct vm_arg_value *out)
{
    uint32_t count;
    uint32_t i;

    if (*ip + 4 > size) return (-1);
    count = wizard_be32(data + *ip);
    *ip += 4;
    out->v.u32_array.count = count;
    out->v.u32_array.data = (uint32_t *)malloc(sizeof(uint32_t) * count);
    if (!out->v.u32_array.data) return (-1);
    for (i = 0; i < count; i++) {
        if (*ip + 4 > size) { (void)free(out->v.u32_array.data); return (-1); }
        out->v.u32_array.data[i] = wizard_be32(data + *ip);
        *ip += 4;
    }
    return (0);
}

static int vm_decode_u64_array(const unsigned char *data, uint64_t size,
                               uint64_t *ip, struct vm_arg_value *out)
{
    uint32_t count;
    uint32_t i;

    if (*ip + 4 > size) return (-1);
    count = wizard_be32(data + *ip);
    *ip += 4;
    out->v.u64_array.count = count;
    out->v.u64_array.data = (uint64_t *)malloc(sizeof(uint64_t) * count);
    if (!out->v.u64_array.data) return (-1);
    for (i = 0; i < count; i++) {
        if (*ip + 8 > size) { (void)free(out->v.u64_array.data); return (-1); }
        out->v.u64_array.data[i] = wizard_be64(data + *ip);
        *ip += 8;
    }
    return (0);
}

int vm_decode_arg(const struct _wizard_ctx_s *ctx,
                  const unsigned char *data, uint64_t size,
                  uint64_t *ip, enum vm_arg_type type,
                  struct vm_arg_value *out)
{
    if (!ctx || !data || !ip || !out)
        return (-1);

    memset(out, 0, sizeof(*out));
    out->type = type;

    switch (type) {
        case VM_TYPE_STR:        return vm_decode_str(ctx, data, size, ip, out);
        case VM_TYPE_STR_ARRAY:  return vm_decode_str_array(ctx, data, size, ip, out);
        case VM_TYPE_U8:         return vm_decode_u8(data, size, ip, out);
        case VM_TYPE_U16:        return vm_decode_u16(data, size, ip, out);
        case VM_TYPE_U32:        return vm_decode_u32(data, size, ip, out);
        case VM_TYPE_U64:        return vm_decode_u64(data, size, ip, out);
        case VM_TYPE_U8_ARRAY:   return vm_decode_u8_array(data, size, ip, out);
        case VM_TYPE_U16_ARRAY:  return vm_decode_u16_array(data, size, ip, out);
        case VM_TYPE_U32_ARRAY:  return vm_decode_u32_array(data, size, ip, out);
        case VM_TYPE_U64_ARRAY:  return vm_decode_u64_array(data, size, ip, out);
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
        switch (args[i].type) {
            case VM_TYPE_STR:
                if (args[i].v.str.data)
                    (void)free(args[i].v.str.data);
                break;
            case VM_TYPE_STR_ARRAY:
                if (args[i].v.str_array.data) {
                    for (j = 0; j < args[i].v.str_array.count; j++)
                        if (args[i].v.str_array.data[j])
                            (void)free(args[i].v.str_array.data[j]);
                    (void)free(args[i].v.str_array.data);
                }
                if (args[i].v.str_array.lens)
                    (void)free(args[i].v.str_array.lens);
                break;
            case VM_TYPE_U8_ARRAY:
                if (args[i].v.u8_array.data)
                    (void)free(args[i].v.u8_array.data);
                break;
            case VM_TYPE_U16_ARRAY:
                if (args[i].v.u16_array.data)
                    (void)free(args[i].v.u16_array.data);
                break;
            case VM_TYPE_U32_ARRAY:
                if (args[i].v.u32_array.data)
                    (void)free(args[i].v.u32_array.data);
                break;
            case VM_TYPE_U64_ARRAY:
                if (args[i].v.u64_array.data)
                    (void)free(args[i].v.u64_array.data);
                break;
            default: break;
        }
    }
}
