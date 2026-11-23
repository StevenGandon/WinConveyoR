#include "wizard_options.h"
#include "vm_internal.h"
#include "wcr_event_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef _WIN32
    #include <pwd.h>
#endif

/* ── Reading ─────────────────────────────────────────────────────────────── */

struct options_acc_s {
    struct wizard_option_s *items;
    size_t count;
};

static int collect_option(const char *op_name, struct vm_arg_value *args,
                          uint32_t count, void *user_data)
{
    struct options_acc_s *acc = (struct options_acc_s *)user_data;
    struct wizard_option_s *grown;
    struct wizard_option_s *opt;

    if (strcmp(op_name, WIZARD_OPTION_OP) != 0 || count != WIZARD_OPTION_ARGS)
        return (-1);

    grown = (struct wizard_option_s *)realloc(acc->items, (acc->count + 1) * sizeof(*grown));
    if (!grown)
        return (-1);
    acc->items = grown;
    opt = &acc->items[acc->count];
    opt->name = strdup(args[0].v.str.data[0]);
    opt->type = strdup(args[1].v.str.data[0]);
    opt->prompt = strdup(args[2].v.str.data[0]);
    opt->default_value = strdup(args[3].v.str.data[0]);
    acc->count++;
    if (!opt->name || !opt->type || !opt->prompt || !opt->default_value)
        return (-1);
    return (0);
}

static const struct _wizard_section_entry_raw_s *find_options_section(const struct _wizard_ctx_s *ctx)
{
    const struct _wizard_section_entry_raw_s *section;
    uint64_t count = wizard_get_section_count(ctx);
    uint64_t i;

    for (i = 0; i < count; i++) {
        section = wizard_get_section(ctx, i);
        if (section && wizard_get_section_type(section) == WIZARD_SECTION_OPTIONS)
            return (section);
    }
    return (NULL);
}

int wizard_read_options(struct _wizard_vm_s *vm, struct _wizard_ctx_s *ctx,
                        struct wizard_option_s **out, size_t *count)
{
    const struct _wizard_section_entry_raw_s *section;
    struct options_acc_s acc = { NULL, 0 };

    if (!out || !count)
        return (-1);
    *out = NULL;
    *count = 0;

    section = find_options_section(ctx);
    if (!section)
        return (0);

    if (vm_visit_section(vm, ctx, section, collect_option, &acc) != 0) {
        wizard_free_options(acc.items, acc.count);
        return (-1);
    }
    *out = acc.items;
    *count = acc.count;
    return (0);
}

void wizard_free_options(struct wizard_option_s *options, size_t count)
{
    size_t i;

    if (!options)
        return;
    for (i = 0; i < count; i++) {
        free(options[i].name);
        free(options[i].type);
        free(options[i].prompt);
        free(options[i].default_value);
    }
    free(options);
}

/* ── Resolution ──────────────────────────────────────────────────────────── */

/* returns the "NAME=value" answer for @p name, or NULL */
static const char *find_answer(const wcr_install_opts *opts, const char *name)
{
    size_t len = strlen(name);
    size_t i;

    if (!opts || !opts->option_values)
        return (NULL);
    for (i = 0; i < opts->option_values_count; i++) {
        const char *entry = opts->option_values[i];

        if (entry && strncmp(entry, name, len) == 0 && entry[len] == '=')
            return (entry + len + 1);
    }
    return (NULL);
}

static int validate_mode(const char *value)
{
    char *end = NULL;
    unsigned long mode = strtoul(value, &end, WIZARD_MODE_BASE);

    return (end != value && *end == '\0' && mode <= WIZARD_MODE_MAX) ? 0 : -1;
}

static int validate_user(const char *value)
{
#ifdef _WIN32
    return (value[0] != '\0') ? 0 : -1;
#else
    return getpwnam(value) ? 0 : -1;
#endif
}

static int validate_option(const wcr_state *state, const struct wizard_option_s *opt, const char *value)
{
    int rc = 0;

    if (strcmp(opt->type, WIZARD_OPTION_MODE) == 0)
        rc = validate_mode(value);
    else if (strcmp(opt->type, WIZARD_OPTION_USER) == 0)
        rc = validate_user(value);
    else if (strcmp(opt->type, WIZARD_OPTION_PATH) == 0)
        rc = (value[0] != '\0') ? 0 : -1;

    if (rc != 0)
        wcr_emit(state, WCR_EVENT_ERROR, "[ERROR] wizard: invalid %s '%s' for option %s",
                 opt->type, value, opt->name);
    return rc;
}

static int apply_option(const wcr_state *state, struct _wizard_vm_s *vm,
                        const struct wizard_option_s *opt, const wcr_install_opts *opts)
{
    const char *answer = find_answer(opts, opt->name);
    char *value;
    int rc;

    value = wizard_vm_expand(vm, answer ? answer : opt->default_value);
    if (!value) {
        wcr_emit(state, WCR_EVENT_ERROR, "[ERROR] wizard: option %s: %s", opt->name, wizard_vm_error(vm));
        return -1;
    }

    rc = validate_option(state, opt, value);
    if (rc == 0)
        rc = wizard_vm_set_var(vm, opt->name, value);
    if (rc == 0)
        wcr_emit(state, WCR_EVENT_INFO, "[INFO] wizard: %s=%s%s", opt->name, value, answer ? "" : " (default)");
    free(value);
    return rc;
}

/* warns about answers that match no declared option (typo in --set) */
static void warn_unknown_answers(const wcr_state *state, const struct wizard_option_s *options,
                                 size_t count, const wcr_install_opts *opts)
{
    size_t i;
    size_t j;
    size_t len;
    int known;

    if (!opts || !opts->option_values)
        return;
    for (i = 0; i < opts->option_values_count; i++) {
        const char *entry = opts->option_values[i];

        known = 0;
        for (j = 0; entry && j < count && !known; j++) {
            len = strlen(options[j].name);
            known = strncmp(entry, options[j].name, len) == 0 && entry[len] == '=';
        }
        if (entry && !known)
            wcr_emit(state, WCR_EVENT_WARNING, "[WARNING] wizard: '%s' does not match any option of the package", entry);
    }
}

int wizard_apply_options(const wcr_state *state, struct _wizard_vm_s *vm,
                         const struct wizard_option_s *options, size_t count,
                         const wcr_install_opts *opts)
{
    size_t i;

    warn_unknown_answers(state, options, count, opts);
    for (i = 0; i < count; i++) {
        if (apply_option(state, vm, &options[i], opts) != 0)
            return -1;
    }
    return 0;
}
