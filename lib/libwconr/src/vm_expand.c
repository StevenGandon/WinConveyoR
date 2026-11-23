#include "vm_internal.h"

/** @brief Maximum length of a variable name inside ${...}. */
#define VM_VAR_NAME_MAX 128

#define VM_VAR_OPEN     "${"
#define VM_VAR_OPEN_LEN 2
#define VM_VAR_CLOSE    '}'

/* ── Variable table ──────────────────────────────────────────────────────── */

static struct vm_var_s *vm_find_var(struct _wizard_vm_s *vm, const char *name)
{
    uint32_t i;

    for (i = 0; i < vm->var_count; i++)
        if (strcmp(vm->vars[i].name, name) == 0)
            return (&vm->vars[i]);
    return (NULL);
}

int wizard_vm_set_var(struct _wizard_vm_s *vm, const char *name, const char *value)
{
    struct vm_var_s *var;
    char *dup;

    if (!vm || !name || !value)
        return (-1);

    dup = strdup(value);
    if (!dup)
        return (-1);

    var = vm_find_var(vm, name);
    if (var) {
        (void)free(var->value);
        var->value = dup;
        return (0);
    }

    if (vm->var_count >= VM_MAX_VARS) {
        (void)free(dup);
        return (-1);
    }

    var = &vm->vars[vm->var_count];
    var->name = strdup(name);
    if (!var->name) {
        (void)free(dup);
        return (-1);
    }
    var->value = dup;
    vm->var_count++;
    return (0);
}

void vm_clear_vars(struct _wizard_vm_s *vm)
{
    uint32_t i;

    for (i = 0; i < vm->var_count; i++) {
        (void)free(vm->vars[i].name);
        (void)free(vm->vars[i].value);
    }
    vm->var_count = 0;
}

const char *wizard_vm_error(const struct _wizard_vm_s *vm)
{
    return (vm ? vm->error : "");
}

/* ── Expansion ───────────────────────────────────────────────────────────── */

static const char *vm_lookup(struct _wizard_vm_s *vm, const char *name)
{
    struct vm_var_s *var = vm_find_var(vm, name);

    if (var)
        return (var->value);
    return (getenv(name));
}

/* Appends @p n bytes of @p src to the growing buffer @p *buf. */
static int vm_buf_append(char **buf, size_t *len, size_t *cap, const char *src, size_t n)
{
    char *grown;

    while (*len + n + 1 > *cap) {
        *cap = (*cap == 0) ? VM_VAR_NAME_MAX : *cap * 2;
        grown = (char *)realloc(*buf, *cap);
        if (!grown)
            return (-1);
        *buf = grown;
    }
    memcpy(*buf + *len, src, n);
    *len += n;
    (*buf)[*len] = '\0';
    return (0);
}

/*
 * Resolves the reference starting at @p ref (pointing at "${").
 * On success appends the value and returns the length consumed, else -1.
 */
static long vm_expand_ref(struct _wizard_vm_s *vm, const char *ref,
                          char **buf, size_t *len, size_t *cap)
{
    char name[VM_VAR_NAME_MAX];
    const char *start = ref + VM_VAR_OPEN_LEN;
    const char *end = strchr(start, VM_VAR_CLOSE);
    const char *value;
    size_t name_len;

    if (!end) {
        snprintf(vm->error, sizeof(vm->error), "unterminated variable in '%s'", ref);
        return (-1);
    }
    name_len = (size_t)(end - start);
    if (name_len == 0 || name_len >= sizeof(name)) {
        snprintf(vm->error, sizeof(vm->error), "invalid variable name in '%s'", ref);
        return (-1);
    }
    memcpy(name, start, name_len);
    name[name_len] = '\0';

    value = vm_lookup(vm, name);
    if (!value) {
        snprintf(vm->error, sizeof(vm->error), "unknown variable '${%s}'", name);
        return (-1);
    }
    if (vm_buf_append(buf, len, cap, value, strlen(value)) != 0)
        return (-1);
    return ((long)(end - ref) + 1);
}

/* Returns a newly allocated copy of @p in with every ${VAR} expanded. */
static char *vm_expand_str(struct _wizard_vm_s *vm, const char *in)
{
    char *buf = NULL;
    size_t len = 0;
    size_t cap = 0;
    const char *p = in;
    const char *ref;
    long used;

    if (vm_buf_append(&buf, &len, &cap, "", 0) != 0)
        return (NULL);

    while ((ref = strstr(p, VM_VAR_OPEN)) != NULL) {
        if (vm_buf_append(&buf, &len, &cap, p, (size_t)(ref - p)) != 0)
            break;
        used = vm_expand_ref(vm, ref, &buf, &len, &cap);
        if (used < 0)
            break;
        p = ref + used;
    }

    if (ref != NULL || vm_buf_append(&buf, &len, &cap, p, strlen(p)) != 0) {
        (void)free(buf);
        return (NULL);
    }
    return (buf);
}

int vm_expand_args(struct _wizard_vm_s *vm, struct vm_arg_value *args, uint32_t count)
{
    uint32_t i;
    uint32_t j;
    char *expanded;

    for (i = 0; i < count; i++) {
        if (args[i].type.base != VM_BASE_STR || !args[i].v.str.data)
            continue;
        for (j = 0; j < args[i].count; j++) {
            if (!args[i].v.str.data[j] || !strstr(args[i].v.str.data[j], VM_VAR_OPEN))
                continue;
            expanded = vm_expand_str(vm, args[i].v.str.data[j]);
            if (!expanded)
                return (-1);
            (void)free(args[i].v.str.data[j]);
            args[i].v.str.data[j] = expanded;
            args[i].v.str.lens[j] = (uint32_t)strlen(expanded);
        }
    }
    return (0);
}

char *wizard_vm_expand(struct _wizard_vm_s *vm, const char *str)
{
    if (!vm || !str)
        return (NULL);
    vm->error[0] = '\0';
    return (vm_expand_str(vm, str));
}

const char *wizard_vm_get_var(struct _wizard_vm_s *vm, const char *name)
{
    struct vm_var_s *var;

    if (!vm || !name)
        return (NULL);
    var = vm_find_var(vm, name);
    return (var ? var->value : NULL);
}
