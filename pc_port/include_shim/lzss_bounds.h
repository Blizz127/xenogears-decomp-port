/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Blizz127 and contributors
 */
#ifndef XENO_LZSS_BOUNDS_H
#define XENO_LZSS_BOUNDS_H

#include <stddef.h>

/* On success consumed receives the prefix and token byte count, excluding
 * trailing input. On failure its previous value is preserved. */
static inline int PcPort_LzssStreamExtent(const unsigned char *stream, size_t bytes,
                                          size_t output_size, size_t *consumed)
{
    size_t cursor = 4;
    size_t produced = 0;

    if (bytes < cursor)
        return 0;

    while (produced < output_size) {
        if (cursor == bytes)
            return 0;
        unsigned int flags = stream[cursor++];

        /* Each flag byte describes a complete group of eight tokens. */
        for (unsigned int mask = 1; mask < 256; mask <<= 1) {
            size_t count = 1;
            if ((flags & mask) == 0) {
                if (cursor == bytes)
                    return 0;
                ++cursor;
            } else {
                if (bytes - cursor < 2)
                    return 0;
                unsigned int pair = (unsigned int)stream[cursor] |
                                    ((unsigned int)stream[cursor + 1] << 8);
                cursor += 2;
                size_t distance = pair & 0xfffu;
                if (distance == 0 || distance > produced)
                    return 0;
                count = 3u + (pair >> 12);
            }
            if (count > output_size - produced)
                return 0;
            produced += count;
        }
    }
    if (consumed != NULL)
        *consumed = cursor;
    return 1;
}

static inline int PcPort_LzssStreamFits(const unsigned char *stream, size_t bytes,
                                        size_t output_size)
{
    return PcPort_LzssStreamExtent(stream, bytes, output_size, NULL);
}

#endif
