#include "vm_internal.h"

#include <stdio.h>
#include <errno.h>

#ifdef _WIN32
#    include <windows.h>
#else
#    include <dirent.h>
#endif

#define VM_MAX_ARGS         8
#define VM_MAX_OPS          256
#define VM_MAX_ISETS        32
#define VM_NAME_SIZE        64

struct vm_arg_def {
    enum vm_arg_type type;
    char label[VM_NAME_SIZE];
};

struct vm_op_def {
    char name[VM_NAME_SIZE];
    uint16_t code;
    struct vm_arg_def args[VM_MAX_ARGS];
    uint32_t arg_count;
};

struct vm_iset_s {
    uint16_t version;
    struct vm_op_def ops[VM_MAX_OPS];
    uint32_t op_count;
};

struct _wizard_vm_s {
    struct vm_iset_s isets[VM_MAX_ISETS];
    uint32_t iset_count;
};

static char *vm_read_file(const char *path)
{
    FILE *f;
    long size;
    size_t usize;
    size_t nread;
    char *buf;

    f = fopen(path, "r");
    if (!f)
        return (NULL);

    fseek(f, 0, SEEK_END);
    size = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (size < 0) {
        fclose(f);
        return (NULL);
    }

    usize = (size_t)size;
    buf = (char *)malloc(usize + 1);
    if (!buf) {
        fclose(f);
        return (NULL);
    }

    nread = fread(buf, 1, usize, f);
    buf[nread] = '\0';
    fclose(f);

    return (buf);
}

static int xml_get_attr(const char *tag_start, const char *attr,
                        char *buf, size_t buf_size)
{
    char search[VM_NAME_SIZE + 2];
    const char *p;
    const char *end;
    const char *tag_end;
    size_t len;

    tag_end = strchr(tag_start, '>');
    if (!tag_end)
        return (-1);

    snprintf(search, sizeof(search), "%s=\"", attr);
    p = strstr(tag_start, search);
    if (!p || p > tag_end)
        return (-1);

    p += strlen(search);
    end = strchr(p, '"');
    if (!end || end > tag_end)
        return (-1);

    len = (size_t)(end - p);
    if (len >= buf_size)
        len = buf_size - 1;
    memcpy(buf, p, len);
    buf[len] = '\0';

    return (0);
}

static enum vm_arg_type vm_parse_type(const char *type_str)
{
    if (strcmp(type_str, "str[]") == 0) return (VM_TYPE_STR_ARRAY);
    if (strcmp(type_str, "str")   == 0) return (VM_TYPE_STR);
    if (strcmp(type_str, "u8")    == 0) return (VM_TYPE_U8);
    if (strcmp(type_str, "u16")   == 0) return (VM_TYPE_U16);
    if (strcmp(type_str, "u32")   == 0) return (VM_TYPE_U32);
    if (strcmp(type_str, "u64")   == 0) return (VM_TYPE_U64);
    return (VM_TYPE_U8);
}

static int vm_parse_xml(const char *xml, struct vm_iset_s *iset)
{
    const char *p = xml;
    const char *instr_tag;
    const char *op_tag;
    const char *arg_tag;
    const char *close_op;
    const char *arg_p;
    char ver_str[16];
    char name[VM_NAME_SIZE];
    char code_str[16];
    char type_str[VM_NAME_SIZE];
    char label[VM_NAME_SIZE];

    instr_tag = strstr(xml, "<instructions");
    if (!instr_tag)
        return (-1);

    if (xml_get_attr(instr_tag, "version", ver_str, sizeof(ver_str)) != 0)
        return (-1);

    iset->version = (uint16_t)atoi(ver_str);
    iset->op_count = 0;

    while ((op_tag = strstr(p, "<op ")) != NULL) {
        struct vm_op_def *def = &iset->ops[iset->op_count];

        if (xml_get_attr(op_tag, "name", name, sizeof(name)) != 0)
            break;
        if (xml_get_attr(op_tag, "code", code_str, sizeof(code_str)) != 0)
            break;

        memcpy(def->name, name, VM_NAME_SIZE);
        def->code = (uint16_t)atoi(code_str);
        def->arg_count = 0;

        close_op = strstr(op_tag + 1, "</op>");
        if (!close_op)
            close_op = xml + strlen(xml);

        arg_p = op_tag;
        while ((arg_tag = strstr(arg_p + 1, "<arg ")) != NULL &&
               arg_tag < close_op &&
               def->arg_count < VM_MAX_ARGS) {
            if (xml_get_attr(arg_tag, "type", type_str, sizeof(type_str)) != 0)
                break;
            if (xml_get_attr(arg_tag, "label", label, sizeof(label)) != 0)
                break;

            def->args[def->arg_count].type = vm_parse_type(type_str);
            memcpy(def->args[def->arg_count].label, label, VM_NAME_SIZE);
            def->arg_count++;
            arg_p = arg_tag;
        }

        iset->op_count++;
        if (iset->op_count >= VM_MAX_OPS)
            break;
        p = op_tag + 1;
    }

    return (iset->op_count > 0 ? 0 : -1);
}

struct _wizard_vm_s *wizard_vm_init(const char *instructions_dir)
{
    struct _wizard_vm_s *vm;
    char path[4096];
    char *xml_buf;

    vm = (struct _wizard_vm_s *)malloc(sizeof(*vm));
    if (!vm)
        return (NULL);

    memset(vm, 0, sizeof(*vm));

#ifdef _WIN32
    {
        WIN32_FIND_DATAA fd;
        HANDLE h;
        char pattern[4096];
        size_t nlen;

        snprintf(pattern, sizeof(pattern), "%s\\instructions_v*.xml",
                 instructions_dir);
        h = FindFirstFileA(pattern, &fd);
        if (h == INVALID_HANDLE_VALUE) {
            (void)free(vm);
            return (NULL);
        }
        do {
            if (vm->iset_count >= VM_MAX_ISETS)
                continue;
            nlen = strlen(fd.cFileName);
            if (nlen < 5 || strcmp(fd.cFileName + nlen - 4, ".xml") != 0)
                continue;
            snprintf(path, sizeof(path), "%s\\%s", instructions_dir,
                     fd.cFileName);
            xml_buf = vm_read_file(path);
            if (!xml_buf)
                continue;
            if (vm_parse_xml(xml_buf, &vm->isets[vm->iset_count]) == 0)
                vm->iset_count++;
            (void)free(xml_buf);
        } while (FindNextFileA(h, &fd));
        FindClose(h);
    }
#else
    {
        DIR *dir;
        struct dirent *entry;
        size_t nlen;

        dir = opendir(instructions_dir);
        if (!dir) {
            (void)free(vm);
            return (NULL);
        }
        while ((entry = readdir(dir)) != NULL) {
            if (strncmp(entry->d_name, "instructions_v", 14) != 0)
                continue;
            nlen = strlen(entry->d_name);
            if (nlen < 5 || strcmp(entry->d_name + nlen - 4, ".xml") != 0)
                continue;
            if (vm->iset_count >= VM_MAX_ISETS)
                continue;
            snprintf(path, sizeof(path), "%s/%s", instructions_dir,
                     entry->d_name);
            xml_buf = vm_read_file(path);
            if (!xml_buf)
                continue;
            if (vm_parse_xml(xml_buf, &vm->isets[vm->iset_count]) == 0)
                vm->iset_count++;
            (void)free(xml_buf);
        }
        closedir(dir);
    }
#endif

    if (vm->iset_count == 0) {
        (void)free(vm);
        return (NULL);
    }

    return (vm);
}

void wizard_vm_close(struct _wizard_vm_s *vm)
{
    (void)free(vm);
}

static const struct vm_iset_s *vm_resolve_iset(const struct _wizard_vm_s *vm,
                                               uint16_t version)
{
    const struct vm_iset_s *best = NULL;
    uint32_t i;

    for (i = 0; i < vm->iset_count; i++) {
        if (vm->isets[i].version > version)
            continue;
        if (!best || vm->isets[i].version > best->version)
            best = &vm->isets[i];
    }

    return (best);
}

static const struct vm_op_def *vm_find_op(const struct vm_iset_s *iset,
                                          uint16_t code)
{
    uint32_t i;

    for (i = 0; i < iset->op_count; i++) {
        if (iset->ops[i].code == code)
            return (&iset->ops[i]);
    }

    return (NULL);
}

static int vm_decode_arg(const struct _wizard_ctx_s *ctx,
                         const unsigned char *data, uint64_t size,
                         uint64_t *ip, enum vm_arg_type type,
                         struct vm_arg_value *out)
{
    uint32_t strndx;
    const char *s;
    uint32_t slen;

    memset(out, 0, sizeof(*out));
    out->type = type;

    switch (type) {
        case VM_TYPE_STR:
            if (*ip + 4 > size) return (-1);
            strndx = wizard_be32(data + *ip);
            *ip += 4;
            if (wizard_get_string(ctx, strndx, &s, &slen) != 0) return (-1);
            out->v.str.data = (char *)malloc(slen + 1);
            if (!out->v.str.data) return (-1);
            memcpy(out->v.str.data, s, slen);
            out->v.str.data[slen] = '\0';
            out->v.str.len = slen;
            break;

        case VM_TYPE_STR_ARRAY: {
            uint32_t count;
            uint32_t i;

            if (*ip + 4 > size) return (-1);
            count = wizard_be32(data + *ip);
            *ip += 4;
            out->v.str_array.count = count;

            if (count == 0) {
                out->v.str_array.data = NULL;
                out->v.str_array.lens = NULL;
                break;
            }

            out->v.str_array.data = (char **)malloc(sizeof(char *) * count);
            out->v.str_array.lens = (uint32_t *)malloc(sizeof(uint32_t) * count);
            if (!out->v.str_array.data || !out->v.str_array.lens) {
                (void)free(out->v.str_array.data);
                (void)free(out->v.str_array.lens);
                return (-1);
            }

            for (i = 0; i < count; i++) {
                if (*ip + 4 > size) goto str_arr_fail;
                strndx = wizard_be32(data + *ip);
                *ip += 4;
                if (wizard_get_string(ctx, strndx, &s, &slen) != 0)
                    goto str_arr_fail;
                out->v.str_array.data[i] = (char *)malloc(slen + 1);
                if (!out->v.str_array.data[i]) goto str_arr_fail;
                memcpy(out->v.str_array.data[i], s, slen);
                out->v.str_array.data[i][slen] = '\0';
                out->v.str_array.lens[i] = slen;
                continue;
            str_arr_fail:
                for (uint32_t j = 0; j < i; j++)
                    (void)free(out->v.str_array.data[j]);
                (void)free(out->v.str_array.data);
                (void)free(out->v.str_array.lens);
                return (-1);
            }
            break;
        }

        case VM_TYPE_U8:
            if (*ip + 1 > size) return (-1);
            out->v.u8_val = data[*ip];
            *ip += 1;
            break;

        case VM_TYPE_U16:
            if (*ip + 2 > size) return (-1);
            out->v.u16_val = wizard_be16(data + *ip);
            *ip += 2;
            break;

        case VM_TYPE_U32:
            if (*ip + 4 > size) return (-1);
            out->v.u32_val = wizard_be32(data + *ip);
            *ip += 4;
            break;

        case VM_TYPE_U64:
            if (*ip + 8 > size) return (-1);
            out->v.u64_val = wizard_be64(data + *ip);
            *ip += 8;
            break;
    }

    return (0);
}

static void vm_free_args(struct vm_arg_value *args, uint32_t count)
{
    uint32_t i;
    uint32_t j;

    for (i = 0; i < count; i++) {
        if (args[i].type == VM_TYPE_STR) {
            (void)free(args[i].v.str.data);
        } else if (args[i].type == VM_TYPE_STR_ARRAY) {
            for (j = 0; j < args[i].v.str_array.count; j++)
                (void)free(args[i].v.str_array.data[j]);
            (void)free(args[i].v.str_array.data);
            (void)free(args[i].v.str_array.lens);
        }
    }
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
    struct vm_arg_value args[VM_MAX_ARGS];
    vm_handler_t handler;
    uint32_t i;
    int ret;

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

        memset(args, 0, sizeof(args));

        for (i = 0; i < def->arg_count; i++) {
            if (vm_decode_arg(ctx, data, size, &ip,
                              def->args[i].type, &args[i]) != 0) {
                vm_free_args(args, i);
                return (-1);
            }
        }

        handler = vm_find_handler(iset->version, def->name);
        ret = handler ? handler(args, def->arg_count) : -1;

        vm_free_args(args, def->arg_count);

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

static const char *vm_type_name(enum vm_arg_type type)
{
    static const char *names[] = { "str", "str[]", "u8", "u16", "u32", "u64" };

    if (type <= VM_TYPE_U64)
        return (names[type]);
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
    struct vm_arg_value args[VM_MAX_ARGS];
    uint32_t i;
    uint32_t j;
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

        memset(args, 0, sizeof(args));

        for (i = 0; i < def->arg_count; i++) {
            if (vm_decode_arg(ctx, data, size, &ip,
                              def->args[i].type, &args[i]) != 0) {
                vm_free_args(args, i);
                return (-1);
            }
        }

        printf("  0x%04lX  [%d] %-16s",
               (unsigned long)instr_start, n, def->name);

        for (i = 0; i < def->arg_count; i++) {
            switch (args[i].type) {
                case VM_TYPE_STR:
                    printf(" %s=\"%s\"", def->args[i].label,
                           args[i].v.str.data);
                    break;
                case VM_TYPE_STR_ARRAY:
                    printf(" %s=[", def->args[i].label);
                    for (j = 0; j < args[i].v.str_array.count; j++)
                        printf("%s\"%s\"", j > 0 ? ", " : "",
                               args[i].v.str_array.data[j]);
                    printf("]");
                    break;
                case VM_TYPE_U8:
                    printf(" %s=%u", def->args[i].label, args[i].v.u8_val);
                    break;
                case VM_TYPE_U16:
                    printf(" %s=0%03o", def->args[i].label, args[i].v.u16_val);
                    break;
                case VM_TYPE_U32:
                    printf(" %s=%u", def->args[i].label, args[i].v.u32_val);
                    break;
                case VM_TYPE_U64:
                    printf(" %s=%lu", def->args[i].label,
                           (unsigned long)args[i].v.u64_val);
                    break;
            }
        }

        printf("\n");
        vm_free_args(args, def->arg_count);
    }

    return (0);
}
