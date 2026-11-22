#ifndef WIZARD_RUNNER_H_
    #define WIZARD_RUNNER_H_

    #include "libwconr.h"

    /* files shipped inside a packaged archive (see wcrpackager) */
    #define WIZARD_FILE_NAME    ".WIZARD"
    #define WIZARD_PAYLOAD_NAME "package.tar.gz"
    #define WIZARD_FILES_DIR    "files"

    #define WIZARD_PATH_MAX     1024

    /*
     * Runs the install procedure of an extracted package.
     * Expects the package archive to be extracted in <cache>/packages/<name>/.
     * If a .WIZARD is present: extracts the payload into files/ then runs every
     * install section matching the current OS, with files/ as working directory.
     * Packages without .WIZARD are left untouched (returns 0).
     */
    int wizard_install(const wcr_state *state, const char *package_name);

    /*
     * Runs the uninstall sections of an installed package (if it has a .WIZARD).
     * Must be called before the package directory is removed.
     */
    int wizard_uninstall(const wcr_state *state, const char *package_name);

#endif /* !WIZARD_RUNNER_H_ */
