#ifndef WIZARD_INSTALL_INFO_H_
    #define WIZARD_INSTALL_INFO_H_

    #include "wizard_private.h"
    #include "wizard_options.h"

    /*
     * <cache>/packages/<name>/.INSTALL remembers what was chosen at install
     * time so that uninstall resolves the same ${VAR}:
     *     prefix=/home/me/.local
     *     var.INSTALL_DIR=/home/me/.local/share/demo
     */
    #define WIZARD_INSTALL_INFO     ".INSTALL"
    #define WIZARD_PREFIX_KEY       "prefix="
    #define WIZARD_VAR_KEY          "var."
    #define WIZARD_INFO_LINE_MAX    2048

    /* writes prefix + the value of every option currently defined in the VM */
    int wizard_info_save(const char *pkg_dir, const char *prefix, struct _wizard_vm_s *vm,
                         const struct wizard_option_s *options, size_t count);

    /* reads the saved prefix into out (-1 if absent) */
    int wizard_info_load_prefix(const char *pkg_dir, char *out, size_t size);

    /* defines every saved option value in the VM (0 if no file) */
    int wizard_info_load_vars(const char *pkg_dir, struct _wizard_vm_s *vm);

#endif /* !WIZARD_INSTALL_INFO_H_ */
