#include "wizard_install_info.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static FILE *open_info(const char *pkg_dir, const char *mode)
{
    char path[WIZARD_INFO_LINE_MAX];
    int n = snprintf(path, sizeof(path), "%s/%s", pkg_dir, WIZARD_INSTALL_INFO);

    if (n < 0 || (size_t)n >= sizeof(path))
        return (NULL);
    return (fopen(path, mode));
}

static void strip_eol(char *line)
{
    line[strcspn(line, "\r\n")] = '\0';
}

int wizard_info_save(const char *pkg_dir, const char *prefix, struct _wizard_vm_s *vm,
                     const struct wizard_option_s *options, size_t count)
{
    const char *value;
    FILE *fp;
    size_t i;

    fp = open_info(pkg_dir, "w");
    if (!fp)
        return (-1);
    fprintf(fp, "%s%s\n", WIZARD_PREFIX_KEY, prefix);
    for (i = 0; i < count; i++) {
        value = wizard_vm_get_var(vm, options[i].name);
        if (value)
            fprintf(fp, "%s%s=%s\n", WIZARD_VAR_KEY, options[i].name, value);
    }
    fclose(fp);
    return (0);
}

int wizard_info_load_prefix(const char *pkg_dir, char *out, size_t size)
{
    char line[WIZARD_INFO_LINE_MAX];
    size_t key_len = strlen(WIZARD_PREFIX_KEY);
    FILE *fp;
    int found = 0;

    fp = open_info(pkg_dir, "r");
    if (!fp)
        return (-1);
    while (!found && fgets(line, sizeof(line), fp)) {
        if (strncmp(line, WIZARD_PREFIX_KEY, key_len) != 0)
            continue;
        strip_eol(line);
        snprintf(out, size, "%s", line + key_len);
        found = 1;
    }
    fclose(fp);
    return (found ? 0 : -1);
}

/* "var.NAME=value" -> defines NAME in the VM */
static int load_var_line(struct _wizard_vm_s *vm, char *line)
{
    char *name = line + strlen(WIZARD_VAR_KEY);
    char *eq = strchr(name, '=');

    if (!eq)
        return (0);
    *eq = '\0';
    return (wizard_vm_set_var(vm, name, eq + 1));
}

int wizard_info_load_vars(const char *pkg_dir, struct _wizard_vm_s *vm)
{
    char line[WIZARD_INFO_LINE_MAX];
    size_t key_len = strlen(WIZARD_VAR_KEY);
    FILE *fp;
    int rc = 0;

    fp = open_info(pkg_dir, "r");
    if (!fp)
        return (0);
    while (rc == 0 && fgets(line, sizeof(line), fp)) {
        if (strncmp(line, WIZARD_VAR_KEY, key_len) != 0)
            continue;
        strip_eol(line);
        rc = load_var_line(vm, line);
    }
    fclose(fp);
    return (rc);
}
