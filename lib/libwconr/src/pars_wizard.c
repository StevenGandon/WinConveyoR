#include "wizard_private.h"

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

    ctx->base = (unsigned char *)mmap(
        NULL,
        ctx->file_size,
        PROT_READ,
        MAP_PRIVATE,
        ctx->fd,
        0);

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

static int wizard_validate_magic(const struct _wizard_file_header_raw_s *header)
{
    if (!header)
        return (-1);

    if (header->magic[0] != WIZARD_MAGIC_0 ||
        header->magic[1] != WIZARD_MAGIC_1 ||
        header->magic[2] != WIZARD_MAGIC_2 ||
        header->magic[3] != WIZARD_MAGIC_3)
        return (-1);

    return (0);
}

static int wizard_validate_bounds(const struct _wizard_ctx_s *ctx)
{
    if (!ctx || !ctx->header)
        return (-1);

    if (ctx->section_count > 1000)
        return (-1);

    if ((size_t)(ctx->strndx_base - ctx->base) > ctx->file_size)
        return (-1);

    size_t sections_end = (size_t)(((unsigned char *)ctx->sections) - ctx->base) +
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
    ctx->fd = -1;

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

uint16_t wizard_get_version(const struct _wizard_ctx_s *ctx)
{
    if (!ctx)
        return (0);
    return (ctx->version);
}

uint32_t wizard_get_flags(const struct _wizard_ctx_s *ctx)
{
    if (!ctx)
        return (0);
    return (ctx->flags);
}

uint64_t wizard_get_section_count(const struct _wizard_ctx_s *ctx)
{
    if (!ctx)
        return (0);
    return (ctx->section_count);
}

const struct _wizard_section_entry_raw_s *wizard_get_section(const struct _wizard_ctx_s *ctx,
                                                             uint64_t index)
{
    if (!ctx)
        return (NULL);
    if (index >= ctx->section_count)
        return (NULL);

    return (&ctx->sections[index]);
}

uint32_t wizard_get_section_type(const struct _wizard_section_entry_raw_s *section)
{
    if (!section)
        return (0);
    return wizard_be32(section->section_type);
}

uint32_t wizard_get_section_flags(const struct _wizard_section_entry_raw_s *section)
{
    if (!section)
        return (0);
    return wizard_be32(section->section_flags);
}

int wizard_get_string(const struct _wizard_ctx_s *ctx,
                      uint32_t strndx_offset,
                      const char **str_out,
                      uint32_t *len_out)
{
    const unsigned char *entry;
    uint32_t length;

    if (!ctx || !str_out || !len_out)
        return (-1);

    entry = ctx->strndx_base + strndx_offset;

    if ((size_t)(entry - ctx->base) + WIZARD_STRING_LENGTH_SIZE > ctx->file_size)
        return (-1);

    length = wizard_be32(entry);

    if ((size_t)(entry - ctx->base) + WIZARD_STRING_LENGTH_SIZE + length > ctx->file_size)
        return (-1);

    *len_out = length;
    *str_out = (const char *)(entry + WIZARD_STRING_LENGTH_SIZE);

    return (0);
}

int wizard_get_section_name(const struct _wizard_ctx_s *ctx,
                            const struct _wizard_section_entry_raw_s *section,
                            const char **name_out,
                            uint32_t *len_out)
{
    uint32_t name_strndx;

    if (!ctx || !section)
        return (-1);

    name_strndx = wizard_be32(section->name_strndx);

    return wizard_get_string(ctx, name_strndx, name_out, len_out);
}

const struct _wizard_section_entry_raw_s *wizard_get_section_by_name(const struct _wizard_ctx_s *ctx,
                                                                     const char *name)
{
    uint64_t i;
    size_t name_len;

    if (!ctx || !name)
        return (NULL);

    name_len = strlen(name);

    for (i = 0; i < ctx->section_count; i++) {
        const char *sec_name;
        uint32_t sec_name_len;

        if (wizard_get_section_name(ctx, &ctx->sections[i],
                                    &sec_name, &sec_name_len) == 0) {
            if (sec_name_len == name_len &&
                memcmp(sec_name, name, name_len) == 0) {
                return (&ctx->sections[i]);
            }
        }
    }

    return (NULL);
}

int wizard_get_section_data(const struct _wizard_ctx_s *ctx,
                            const struct _wizard_section_entry_raw_s *section,
                            const unsigned char **data_out,
                            uint64_t *size_out)
{
    uint64_t section_offset;
    uint64_t section_size;

    if (!ctx || !section || !data_out || !size_out)
        return (-1);

    section_offset = wizard_be64(section->section_offset);
    section_size = wizard_be64(section->section_size);

    if (section_offset + section_size > ctx->file_size)
        return (-1);

    *size_out = section_size;
    *data_out = ctx->base + section_offset;

    return (0);
}
