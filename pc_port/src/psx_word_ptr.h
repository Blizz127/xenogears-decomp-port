/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Blizz127 and contributors
 */
#ifndef XENO_PSX_WORD_PTR_H
#define XENO_PSX_WORD_PTR_H

#include "psx_memory.h"

static inline void *PsxMemory_WordPtr(uint32_t word)
{
    /* Four adjacent guest windows map onto the same two MiB of RAM. */
    if (word >= UINT32_C(0x80000000) && word < UINT32_C(0x80800000))
        return PSX_ADDR(word);
    return (void *)(uintptr_t)word;
}

#endif
