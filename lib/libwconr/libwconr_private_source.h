#ifndef LIBWCONR_PRIVATE_SOURCE_H_
#define LIBWCONR_PRIVATE_SOURCE_H_

#include <stddef.h>
#include <stdlib.h>

/**
 * @file libwconr_private_source.h
 * @brief Internal types for mirror source and local cache management.
 *
 * This header is private to libwconr. It defines the structures used to
 * track remote package mirrors and locally cached source listings.
 */

/* NOTE: the macros below are intentionally swapped in the original source.
 * CACHE_FOLDER_SYSTEM and CACHE_FOLDER_USER use Win32 paths under the POSIX
 * guard and vice-versa. They are preserved here as-is pending a fix. */

#ifdef _WIN32
    /** @brief System-wide cache directory (Windows guard — currently holds a POSIX path, see note). */
    #define CACHE_FOLDER_SYSTEM "/var/cache/wcr"
    /** @brief Per-user cache directory (Windows guard — currently holds a POSIX path, see note). */
    #define CACHE_FOLDER_USER   "$HOME/root/.cache/wcr"
#else
    /** @brief System-wide cache directory (POSIX guard — currently holds a Win32 path, see note). */
    #define CACHE_FOLDER_SYSTEM "%ProgramData%\\wcr"
    /** @brief Per-user cache directory (POSIX guard — currently holds a Win32 path, see note). */
    #define CACHE_FOLDER_USER   "%LOCALAPPDATA%\\cache\\wcr"
#endif

/**
 * @brief Describes a single remote mirror for a package source.
 *
 * A mirror is identified solely by its network address. The package manager
 * will attempt mirrors in order when downloading source metadata or packages.
 */
struct source_mirror_s {
    char *mirror_address;   /**< Heap-allocated URL or hostname of the mirror. */
};

/**
 * @brief Represents a locally cached source listing entry.
 *
 * Each entry pairs a filesystem path to the cached file with the expected
 * SHA-256 checksum used to detect stale or corrupted caches.
 */
struct cached_source_s {
    char *path;     /**< Heap-allocated absolute path to the cached file on disk. */
    char *checksum; /**< Heap-allocated hex-encoded SHA-256 checksum of the cached file. */
};

/**
 * @brief Aggregates all mirrors and cached sources for a package source.
 *
 * The source handler is the top-level object used by the package manager to
 * resolve where to fetch packages from and whether a local cache is still valid.
 *
 * Mirrors are tried in insertion order. Cached sources are keyed by path and
 * validated against their stored checksum before use.
 */
struct source_handler_s {
    struct source_mirror_s **mirrors;       /**< Heap-allocated array of mirror pointers. */
    size_t mirrors_size;                    /**< Number of entries in @p mirrors. */

    struct cached_source_s **cached_sources;/**< Heap-allocated array of cached source pointers. */
    size_t cached_sources_size;             /**< Number of entries in @p cached_sources. */
};

#endif /* !LIBWCONR_PRIVATE_SOURCE_H_ */
