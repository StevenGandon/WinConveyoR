#include "vm_internal.h"

static const char * const vm_xml_files[] = {
    "instructions_v0.xml",
    NULL
};

struct _wizard_vm_s *wizard_vm_init(const char *instructions_dir)
{
    struct _wizard_vm_s *vm;
    char path[VM_BUF_SIZE];
    struct vm_iset_s *new_isets;
    size_t i;

    if (!instructions_dir)
        return (NULL);

    vm = (struct _wizard_vm_s *)malloc(sizeof(*vm));
    if (!vm)
        return (NULL);

    vm->isets = NULL;
    vm->iset_count = 0;

    for (i = 0; vm_xml_files[i] != NULL; i++) {
        snprintf(path, sizeof(path), "%s/%s", instructions_dir,
                 vm_xml_files[i]);
        new_isets = (struct vm_iset_s *)realloc(vm->isets,
                        (vm->iset_count + 1) * sizeof(struct vm_iset_s));
        if (!new_isets)
            continue;
        vm->isets = new_isets;
        if (vm_parse_xml(path, &vm->isets[vm->iset_count]) == 0)
            vm->iset_count++;
    }

    if (vm->iset_count == 0) {
        (void)free(vm);
        return (NULL);
    }

    return (vm);
}

void wizard_vm_close(struct _wizard_vm_s *vm)
{
    uint32_t i;
    uint32_t j;
    uint32_t k;

    if (!vm)
        return;

    for (i = 0; i < vm->iset_count; i++) {
        for (j = 0; j < vm->isets[i].op_count; j++) {
            struct vm_op_def *def = &vm->isets[i].ops[j];
            if (def->name)
                (void)free(def->name);
            for (k = 0; k < def->arg_count; k++)
                if (def->args[k].label)
                    (void)free(def->args[k].label);
            if (def->args)
                (void)free(def->args);
        }
        if (vm->isets[i].ops)
            (void)free(vm->isets[i].ops);
    }

    (void)free(vm->isets);
    (void)free(vm);
}

static const struct vm_iset_s *vm_resolve_iset(const struct _wizard_vm_s *vm,
                                               uint16_t version)
{
    uint32_t i;

    if (!vm)
        return (NULL);

    for (i = 0; i < vm->iset_count; i++) {
        if (vm->isets[i].version == version)
            return (&vm->isets[i]);
    }

    return (NULL);
}

static const struct vm_op_def *vm_find_op(const struct vm_iset_s *iset,
                                          uint16_t code)
{
    uint32_t i;

    if (!iset)
        return (NULL);

    for (i = 0; i < iset->op_count; i++) {
        if (iset->ops[i].code == code)
            return (&iset->ops[i]);
    }

    return (NULL);
}

static int vm_run_section(const struct vm_iset_s *iset,
                          struct _wizard_ctx_s *ctx,
                          const struct _wizard_section_entry_raw_s *section)
{
    const unsigned char *data;
    uint64_t size;
    uint64_t ip;
    uint16_t opcode;
    const struct vm_op_def *def;
    struct vm_arg_value *args;
    vm_handler_t handler;
    uint32_t i;
    int ret;

    if (!iset || !ctx || !section)
        return (-1);

    if (wizard_get_section_data(ctx, section, &data, &size) != 0)
        return (-1);

    ip = 0;

    while (ip < size) {
        if (ip + 2 > size)
            return (-1);
        opcode = wizard_be16(data + ip);
        ip += 2;

        def = vm_find_op(iset, opcode);
        if (!def)
            return (-1);

        args = NULL;
        if (def->arg_count > 0) {
            args = (struct vm_arg_value *)malloc(
                       def->arg_count * sizeof(*args));
            if (!args)
                return (-1);
            memset(args, 0, def->arg_count * sizeof(*args));
        }

        for (i = 0; i < def->arg_count; i++) {
            if (vm_decode_arg(ctx, data, size, &ip,
                              def->args[i].type, &args[i]) != 0) {
                vm_free_args(args, i);
                (void)free(args);
                return (-1);
            }
        }

        handler = vm_find_handler(iset->version, def->name);
        ret = handler ? handler(args, def->arg_count) : -1;

        vm_free_args(args, def->arg_count);
        (void)free(args);

        if (ret != 0)
            return (-1);
    }

    return (0);
}

int wizard_vm_exec(struct _wizard_vm_s *vm,
                   struct _wizard_ctx_s *ctx,
                   const struct _wizard_section_entry_raw_s *section)
{
    const struct vm_iset_s *iset;

    if (!vm || !ctx || !section)
        return (-1);

    iset = vm_resolve_iset(vm, wizard_get_version(ctx));
    if (!iset)
        return (-1);

    return vm_run_section(iset, ctx, section);
}

static const char *vm_type_name(struct vm_arg_type type)
{
    switch (type.base) {
        case VM_BASE_STR:  return (type.is_array ? "str[]"  : "str");
        case VM_BASE_U8:   return (type.is_array ? "u8[]"   : "u8");
        case VM_BASE_U16:  return (type.is_array ? "u16[]"  : "u16");
        case VM_BASE_U32:  return (type.is_array ? "u32[]"  : "u32");
        case VM_BASE_U64:  return (type.is_array ? "u64[]"  : "u64");
    }
    return ("?");
}

void wizard_vm_dump_instructions(const struct _wizard_vm_s *vm)
{
    uint32_t i;
    uint32_t j;
    uint32_t k;

    if (!vm)
        return;

    printf("Loaded %u instruction set(s):\n\n", vm->iset_count);

    for (k = 0; k < vm->iset_count; k++) {
        const struct vm_iset_s *iset = &vm->isets[k];

        printf("  version %u — %u opcodes\n", iset->version, iset->op_count);

        for (i = 0; i < iset->op_count; i++) {
            const struct vm_op_def *def = &iset->ops[i];

            printf("    0x%04X  %-16s", def->code, def->name);

            if (def->arg_count == 0) {
                printf("(no args)");
            } else {
                printf("(");
                for (j = 0; j < def->arg_count; j++)
                    printf("%s%s %s", j > 0 ? ", " : "",
                           vm_type_name(def->args[j].type),
                           def->args[j].label);
                printf(")");
            }
            printf("\n");
        }
        printf("\n");
    }
}

static void vm_disasm_print_arg(const struct vm_arg_def *adef,
                                const struct vm_arg_value *aval)
{
    uint32_t j;

    if (!adef || !aval)
        return;

    switch (aval->type.base) {
        case VM_BASE_STR:
            if (aval->type.is_array) {
                printf(" %s=[", adef->label);
                for (j = 0; j < aval->count; j++)
                    printf("%s\"%s\"", j > 0 ? ", " : "",
                           aval->v.str.data[j]);
                printf("]");
            } else {
                printf(" %s=\"%s\"", adef->label, aval->v.str.data[0]);
            }
            break;
        case VM_BASE_U8:
            printf(" %s=%u", adef->label, aval->v.u8[0]);
            break;
        case VM_BASE_U16:
            printf(" %s=0%03o", adef->label, aval->v.u16[0]);
            break;
        case VM_BASE_U32:
            printf(" %s=%u", adef->label, aval->v.u32[0]);
            break;
        case VM_BASE_U64:
            printf(" %s=%lu", adef->label, (unsigned long)aval->v.u64[0]);
            break;
    }
}

int wizard_vm_disasm(struct _wizard_vm_s *vm,
                     struct _wizard_ctx_s *ctx,
                     const struct _wizard_section_entry_raw_s *section)
{
    const unsigned char *data;
    uint64_t size;
    uint64_t ip;
    uint64_t instr_start;
    uint16_t opcode;
    const struct vm_iset_s *iset;
    const struct vm_op_def *def;
    struct vm_arg_value *args;
    uint32_t i;
    int n = 0;

    if (!vm || !ctx || !section)
        return (-1);

    iset = vm_resolve_iset(vm, wizard_get_version(ctx));
    if (!iset)
        return (-1);

    if (wizard_get_section_data(ctx, section, &data, &size) != 0)
        return (-1);

    ip = 0;

    while (ip < size) {
        instr_start = ip;

        if (ip + 2 > size)
            return (-1);
        opcode = wizard_be16(data + ip);
        ip += 2;
        n++;

        def = vm_find_op(iset, opcode);
        if (!def) {
            printf("  0x%04lX  [%d] unknown (0x%04X)\n",
                   (unsigned long)instr_start, n, opcode);
            return (-1);
        }

        args = NULL;
        if (def->arg_count > 0) {
            args = (struct vm_arg_value *)malloc(
                       def->arg_count * sizeof(*args));
            if (!args)
                return (-1);
            memset(args, 0, def->arg_count * sizeof(*args));
        }

        for (i = 0; i < def->arg_count; i++) {
            if (vm_decode_arg(ctx, data, size, &ip,
                              def->args[i].type, &args[i]) != 0) {
                vm_free_args(args, i);
                (void)free(args);
                return (-1);
            }
        }

        printf("  0x%04lX  [%d] %-16s", (unsigned long)instr_start, n,
               def->name);

        for (i = 0; i < def->arg_count; i++)
            vm_disasm_print_arg(&def->args[i], &args[i]);

        printf("\n");
        vm_free_args(args, def->arg_count);
        (void)free(args);
    }

    return (0);
}
