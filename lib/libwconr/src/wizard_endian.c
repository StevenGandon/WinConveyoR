#ifdef _WIN32
#    include <stdlib.h>
#elif defined(__APPLE__)
#    include <libkern/OSByteOrder.h>
#    define be16toh(x) OSSwapBigToHostInt16(x)
#    define be32toh(x) OSSwapBigToHostInt32(x)
#    define be64toh(x) OSSwapBigToHostInt64(x)
#else
#    include <endian.h>
#endif

#include "wizard_private.h"

uint16_t wizard_be16(const unsigned char *p)
{
    uint16_t v;

    if (!p)
        return (0);
    memcpy(&v, p, sizeof(v));
#ifdef _WIN32
    return _byteswap_ushort(v);
#else
    return be16toh(v);
#endif
}

uint32_t wizard_be32(const unsigned char *p)
{
    uint32_t v;

    if (!p)
        return (0);
    memcpy(&v, p, sizeof(v));
#ifdef _WIN32
    return _byteswap_ulong(v);
#else
    return be32toh(v);
#endif
}

uint64_t wizard_be64(const unsigned char *p)
{
    uint64_t v;

    if (!p)
        return (0);
    memcpy(&v, p, sizeof(v));
#ifdef _WIN32
    return _byteswap_uint64(v);
#else
    return be64toh(v);
#endif
}
