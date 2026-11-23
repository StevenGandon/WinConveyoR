#include "vm_internal.h"

/* decodes the arguments of one instruction at *ip */
static struct vm_arg_value *vm_decode_all(struct _wizard_ctx_s *ctx,
                                          const unsigned char *data, uint64_t size,
                                          uint64_t *ip, const struct vm_op_def *def)
{
    struct vm_arg_value *args;
    uint32_t i;

    args = (struct vm_arg_value *)calloc(def->arg_count ? def->arg_count : 1, sizeof(*args));
    if (!args)
        return (NULL);

    for (i = 0; i < def->arg_count; i++) {
        if (vm_decode_arg(ctx, data, size, ip, def->args[i].type, &args[i]) != 0) {
            vm_free_args(args, i);
            (void)free(args);
            return (NULL);
        }
    }
    return (args);
}

int vm_visit_section(struct _wizard_vm_s *vm, struct _wizard_ctx_s *ctx,
                     const struct _wizard_section_entry_raw_s *section,
                     vm_visit_t visit, void *user_data)
{
    const struct vm_iset_s *iset;
    const struct vm_op_def *def;
    const unsigned char *data;
    struct vm_arg_value *args;
    uint64_t size;
    uint64_t ip = 0;
    int ret;

    if (!vm || !ctx || !section || !visit)
        return (-1);

    iset = vm_resolve_iset(vm, wizard_get_version(ctx));
    if (!iset || wizard_get_section_data(ctx, section, &data, &size) != 0)
        return (-1);

    while (ip < size) {
        if (ip + sizeof(uint16_t) > size)
            return (-1);
        def = vm_find_op(iset, wizard_be16(data + ip));
        ip += sizeof(uint16_t);
        if (!def)
            return (-1);

        args = vm_decode_all(ctx, data, size, &ip, def);
        if (!args)
            return (-1);

        ret = visit(def->name, args, def->arg_count, user_data);
        vm_free_args(args, def->arg_count);
        (void)free(args);
        if (ret != 0)
            return (-1);
    }
    return (0);
}
