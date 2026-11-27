/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Blizz127 and contributors
 */
#include <stdint.h>

/* The field and movie overlays share these display and diagnostic words. */
int32_t D_801D68B4;
int32_t D_801E89D4;
int32_t D_801E89E0;

static int unavailable;

int MoviePlayerStartFailed(void)
{
    return unavailable;
}

int32_t func_801D3538(int32_t width, int32_t height, int32_t scale,
                      int32_t strips, int32_t sectors, int32_t sector_size,
                      int32_t mode)
{
    (void)width;
    (void)height;
    (void)scale;
    (void)strips;
    (void)sectors;
    (void)sector_size;
    (void)mode;
    unavailable = 1;
    return -1;
}

void func_801D37CC(uint16_t file, int32_t offset, uint16_t first_frame,
                   uint16_t last_frame, uint16_t channel, int32_t flags,
                   uint16_t loop, uint16_t x0, uint16_t y0, uint16_t x1,
                   uint16_t y1, uint16_t lines,
                   void (*present)(uint16_t, uint16_t, uint16_t))
{
    (void)file;
    (void)offset;
    (void)first_frame;
    (void)last_frame;
    (void)channel;
    (void)flags;
    (void)loop;
    (void)x0;
    (void)y0;
    (void)x1;
    (void)y1;
    (void)lines;
    (void)present;
    unavailable = 1;
}

/* Playback reserves no buffers, starts no reader and delivers no frames. */
void func_801D3F7C(void) {}
void func_801D4318(void) {}
void func_801D43B0(void) {}
