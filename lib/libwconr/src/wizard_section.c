#include "wizard_private.h"

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
