/* Copyright (c) 2026 Blizz127 and contributors
 * SPDX-License-Identifier: MIT
 * Validate the complete packed stream before installing a game-state overlay. */

static int DecodeOverlayStream(const void *src, size_t srcSize, void *dst, size_t dstSize,
                               size_t expectedSize) {
    if (!src || !dst || ((uintptr_t)src & 3u) || srcSize < sizeof(u32))
        return -1;
    u32 declared;
    memcpy(&declared, src, sizeof(declared));
    if (!declared || declared > dstSize || (expectedSize && declared != expectedSize))
        return -1;
    uintptr_t input = (uintptr_t)src, output = (uintptr_t)dst;
    if ((output >= input ? output - input < srcSize : input - output < declared) ||
        !PcPort_LzssStreamFits(src, srcSize, declared))
        return -1;
    LZSSDecompress((void *)src, dst);
    return 0;
}

int PcPort_DecodeGameOverlay(unsigned int state, const void *src, unsigned int srcSize,
                             void *dst, unsigned int dstSize) {
    const PcPortOverlayLayout *layout = PcPort_GetOverlayLayout(state);
    if (!layout || srcSize < (size_t)layout->sourceSize)
        return -1;
    return DecodeOverlayStream(src, layout->sourceSize, dst, dstSize, layout->imageSize);
}

int PcPort_DecodeCachedGameOverlay(unsigned int state, void *dst, unsigned int dstSize) {
    if (!PcPort_GetOverlayLayout(state) || state != g_CurGameStateOverlayID)
        return -1;
    return DecodeOverlayStream(g_CurGameStateOverlayBuffer, cachedSourceSize, dst, dstSize,
                               cachedExpectedImageSize);
}
