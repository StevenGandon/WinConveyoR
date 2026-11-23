#ifndef WIZARD_OPTIONS_H_
    #define WIZARD_OPTIONS_H_

    #include "libwconr.h"
    #include "wizard_private.h"

    /* option types declared by the packager (options: section of the YAML) */
    #define WIZARD_OPTION_PATH      "path"
    #define WIZARD_OPTION_USER      "user"
    #define WIZARD_OPTION_MODE      "mode"
    #define WIZARD_OPTION_STRING    "string"

    /* name of the data-only instruction describing one option */
    #define WIZARD_OPTION_OP        "option"
    #define WIZARD_OPTION_ARGS      4

    /* textual permission mode ("0755") */
    #define WIZARD_MODE_BASE        8
    #define WIZARD_MODE_MAX         07777

    /* one field the user can fill before installing */
    struct wizard_option_s {
        char *name;
        char *type;
        char *prompt;
        char *default_value;
    };

    /* reads the options section (count = 0 if the package declares none) */
    int wizard_read_options(struct _wizard_vm_s *vm, struct _wizard_ctx_s *ctx,
                            struct wizard_option_s **out, size_t *count);
    void wizard_free_options(struct wizard_option_s *options, size_t count);

    /*
     * Resolves every option (user answer from opts, else default), expands
     * ${VAR} in it, validates it against its type and defines it in the VM.
     */
    int wizard_apply_options(const wcr_state *state, struct _wizard_vm_s *vm,
                             const struct wizard_option_s *options, size_t count,
                             const wcr_install_opts *opts);

#endif /* !WIZARD_OPTIONS_H_ */
