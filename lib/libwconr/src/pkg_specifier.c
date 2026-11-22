#include "pkg_specifier.h"

#include <stdlib.h>
#include <string.h>

static char *extract_field(const char *start, const char *stops)
{
    const char *p = start;
    size_t len;

    while (*p && !strchr(stops, *p))
        p++;

    len = (size_t)(p - start);
    if (len == 0)
        return (NULL);

    {
        char *dup = malloc(len + 1);
        if (!dup) return NULL;
        memcpy(dup, start, len);
        dup[len] = '\0';
        return dup;
    }
}

int pkg_specifier_parse(const char *input, struct pkg_specifier *out)
{
    const char *p;

    if (!input || !out)
        return (-1);

    memset(out, 0, sizeof(*out));

    p = input;
    while (*p && *p != ':' && *p != '@' && *p != '#')
        p++;

    if (p == input)
        return (-1);

    {
        size_t nlen = (size_t)(p - input);
        out->name = malloc(nlen + 1);
        if (out->name) {
            memcpy(out->name, input, nlen);
            out->name[nlen] = '\0';
        }
    }
    if (!out->name)
        return (-1);

    while (*p) {
        if (*p == ':') {
            free(out->version);
            out->version = extract_field(p + 1, "@#\n");
            p += 1 + (out->version ? strlen(out->version) : 0);
        } else if (*p == '@') {
            free(out->arch);
            out->arch = extract_field(p + 1, ":#\n");
            p += 1 + (out->arch ? strlen(out->arch) : 0);
        } else if (*p == '#') {
            free(out->machine);
            out->machine = extract_field(p + 1, "@:\n");
            p += 1 + (out->machine ? strlen(out->machine) : 0);
        } else {
            p++;
        }
    }

    return (0);
}

void pkg_specifier_free(struct pkg_specifier *spec)
{
    if (!spec)
        return;

    free(spec->name);
    free(spec->version);
    free(spec->arch);
    free(spec->machine);
    memset(spec, 0, sizeof(*spec));
}
