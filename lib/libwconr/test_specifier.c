#include "pkg_specifier.h"
#include <stdio.h>

static void print_spec(const char *input, const struct pkg_specifier *s)
{
    printf("  input:   \"%s\"\n", input);
    printf("  name:    %s\n", s->name ? s->name : "(null)");
    printf("  version: %s\n", s->version ? s->version : "(null)");
    printf("  arch:    %s\n", s->arch ? s->arch : "(null)");
    printf("  machine: %s\n", s->machine ? s->machine : "(null)");
    printf("\n");
}

int main(void)
{
    const char *cases[] = {
        "gcc:1.0.0@windows#amd64",
        "gcc",
        "gcc:1.0.0",
        "gcc:1.0.0#amd64",
        "gcc@windows",
        "gcc#amd64",
        "libmpfr:2.3.1@linux#arm64",
        NULL
    };

    struct pkg_specifier spec;
    int i;

    printf("=== pkg_specifier_parse tests ===\n\n");

    for (i = 0; cases[i]; i++) {
        if (pkg_specifier_parse(cases[i], &spec) != 0) {
            printf("  FAIL: \"%s\" -> parse error\n\n", cases[i]);
            continue;
        }
        print_spec(cases[i], &spec);
        pkg_specifier_free(&spec);
    }

    return 0;
}
