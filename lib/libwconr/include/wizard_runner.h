#ifndef WIZARD_RUNNER_H_
    #define WIZARD_RUNNER_H_

    #include "libwconr.h"

    /* files shipped inside a packaged archive (see wcrpackager) */
    #define WIZARD_FILE_NAME    ".WIZARD"
    #define WIZARD_PAYLOAD_NAME "package.tar.gz"
    #define WIZARD_FILES_DIR    "files"

    #define WIZARD_PATH_MAX     1024

    /* install prefix: defaults are per-user, no admin rights needed */
    #define WIZARD_DEFAULT_PREFIX_POSIX ".local"
    #define WIZARD_DEFAULT_PREFIX_WIN   "Programs"

    /* standard subdirs exposed as ${MACHINE_BIN}, ${MACHINE_LIB}... */
    #define WIZARD_SUBDIR_BIN       "bin"
    #define WIZARD_SUBDIR_LIB       "lib"
    #define WIZARD_SUBDIR_INCLUDE   "include"
    #define WIZARD_SUBDIR_MAN       "share/man"


    /*
     * Runs the install procedure of an extracted package.
     * Expects the package archive to be extracted in <cache>/packages/<name>/.
     * If a .WIZARD is present: extracts the payload into files/ then runs every
     * install section matching the current OS, with files/ as working directory.
     * Variables ${PREFIX}, ${MACHINE_BIN|LIB|INCLUDE|MAN}, ${PKG_NAME}, ${PKG_DIR},
     * ${USER} and the package options (answers in opts, else defaults) are
     * available to the steps. opts NULL / opts->prefix NULL = per-user default.
     * Packages without .WIZARD are left untouched (returns 0).
     */
    int wizard_install(const wcr_state *state, const char *package_name, const wcr_install_opts *opts);

    /*
     * Runs the uninstall section of an installed package (if it has a .WIZARD),
     * with the prefix and option values saved at install time.
     * Must be called before the package directory is removed.
     */
    int wizard_uninstall(const wcr_state *state, const char *package_name);

    /*
     * Lists the options declared by the package unpacked in pkg_dir, defaults
     * resolved against prefix (NULL = per-user default). count = 0 if none.
     */
    int wizard_list_options(const wcr_state *state, const char *pkg_dir, const char *package_name,
                            const char *prefix, wcr_pkg_option **out, size_t *out_count);

#endif /* !WIZARD_RUNNER_H_ */
