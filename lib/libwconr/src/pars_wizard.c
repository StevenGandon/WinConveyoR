#include "wizard_private.h"

#ifdef _WIN32

static int wizard_mmap_file(struct _wizard_ctx_s *ctx, const char *path)
{
    HANDLE hFile;
    HANDLE hMapping;
    LARGE_INTEGER size;
    void *base;

    if (!ctx || !path)
        return (-1);

    hFile = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL,
                        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        return (-1);

    if (!GetFileSizeEx(hFile, &size) || size.QuadPart == 0) {
        CloseHandle(hFile);
        return (-1);
    }

    hMapping = CreateFileMappingA(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
    CloseHandle(hFile);
    if (!hMapping)
        return (-1);

    base = MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, 0);
    CloseHandle(hMapping);
    if (!base)
        return (-1);

    ctx->file_size = (size_t)size.QuadPart;
    ctx->base = (unsigned char *)base;
    return (0);
}

static void wizard_munmap_file(struct _wizard_ctx_s *ctx)
{
    if (!ctx)
        return;

    if (ctx->base) {
        UnmapViewOfFile(ctx->base);
        ctx->base = NULL;
    }
}

#else /* POSIX */

static int wizard_mmap_file(struct _wizard_ctx_s *ctx, const char *path)
{
    struct stat st;

    if (!ctx || !path)
        return (-1);

    ctx->fd = open(path, O_RDONLY);
    if (ctx->fd < 0)
        return (-1);

    if (fstat(ctx->fd, &st) != 0) {
        (void)close(ctx->fd);
        ctx->fd = -1;
        return (-1);
    }

    ctx->file_size = (size_t)st.st_size;

    if (ctx->file_size == 0) {
        (void)close(ctx->fd);
        ctx->fd = -1;
        return (-1);
    }

    ctx->base = (unsigned char *)mmap(NULL, ctx->file_size, PROT_READ,
                                      MAP_PRIVATE, ctx->fd, 0);
    if (ctx->base == MAP_FAILED) {
        ctx->base = NULL;
        (void)close(ctx->fd);
        ctx->fd = -1;
        return (-1);
    }

    return (0);
}

static void wizard_munmap_file(struct _wizard_ctx_s *ctx)
{
    if (!ctx)
        return;

    if (ctx->base) {
        (void)munmap(ctx->base, ctx->file_size);
        ctx->base = NULL;
    }

    if (ctx->fd >= 0) {
        (void)close(ctx->fd);
        ctx->fd = -1;
    }
}

#endif /* _WIN32 */

static int wizard_validate_magic(const struct _wizard_file_header_raw_s *header)
{
    if (!header)
        return (-1);

    if (strncmp((const char *)header->magic, WIZARD_MAGIC, WIZARD_MAGIC_SIZE) != 0)
        return (-1);

    return (0);
}

static int wizard_validate_bounds(const struct _wizard_ctx_s *ctx)
{
    size_t sections_end;

    if (!ctx || !ctx->header)
        return (-1);

    if (ctx->section_count > WIZARD_MAX_SECTIONS)
        return (-1);

    if ((size_t)(ctx->strndx_base - ctx->base) > ctx->file_size)
        return (-1);

    sections_end = (size_t)(((unsigned char *)ctx->sections) - ctx->base) +
                   (ctx->section_count * WIZARD_SECTION_ENTRY_SIZE);
    if (sections_end > ctx->file_size)
        return (-1);

    return (0);
}

struct _wizard_ctx_s *wizard_open(const char *path)
{
    struct _wizard_ctx_s *ctx = NULL;
    uint64_t section_header_offset;
    uint64_t strndx_offset;

    if (!path)
        return (NULL);

    ctx = (struct _wizard_ctx_s *)malloc(sizeof(struct _wizard_ctx_s));
    if (!ctx)
        return (NULL);

    memset(ctx, 0, sizeof(*ctx));
#ifndef _WIN32
    ctx->fd = -1;
#endif

    if (wizard_mmap_file(ctx, path) != 0) {
        (void)free(ctx);
        return (NULL);
    }

    if (ctx->file_size < WIZARD_HEADER_SIZE) {
        wizard_munmap_file(ctx);
        (void)free(ctx);
        return (NULL);
    }

    ctx->header = (struct _wizard_file_header_raw_s *)ctx->base;

    if (wizard_validate_magic(ctx->header) != 0) {
        wizard_munmap_file(ctx);
        (void)free(ctx);
        return (NULL);
    }

    ctx->version = wizard_be16(ctx->header->version);
    ctx->flags = wizard_be32(ctx->header->flags);

    section_header_offset = wizard_be64(ctx->header->section_header_offset);
    strndx_offset = wizard_be64(ctx->header->strndx_offset);

    if (section_header_offset + WIZARD_SECTION_HEADER_PREFIX > ctx->file_size) {
        wizard_munmap_file(ctx);
        (void)free(ctx);
        return (NULL);
    }

    ctx->section_header = (struct _wizard_section_header_raw_s *)
                          (ctx->base + section_header_offset);

    ctx->section_count = wizard_be64(ctx->section_header->section_count);

    ctx->sections = (struct _wizard_section_entry_raw_s *)
                    (ctx->base + section_header_offset + WIZARD_SECTION_HEADER_PREFIX);

    ctx->strndx_base = ctx->base + strndx_offset;

    if (wizard_validate_bounds(ctx) != 0) {
        wizard_munmap_file(ctx);
        (void)free(ctx);
        return (NULL);
    }

    return (ctx);
}

void wizard_close(struct _wizard_ctx_s *ctx)
{
    if (!ctx)
        return;

    wizard_munmap_file(ctx);
    (void)free(ctx);
}
