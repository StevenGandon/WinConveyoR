#ifndef PKG_SPECIFIER_H_
    #define PKG_SPECIFIER_H_

    struct pkg_specifier {
        char *name;
        char *version;
        char *arch;
        char *machine;
    };

    int pkg_specifier_parse(const char *input, struct pkg_specifier *out);
    void pkg_specifier_free(struct pkg_specifier *spec);

#endif /* !PKG_SPECIFIER_H_ */
