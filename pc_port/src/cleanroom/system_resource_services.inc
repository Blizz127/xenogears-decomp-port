#include <stdint.h>

void *GetSystemFontFile(void)
{
    return *(void *volatile *)&g_SystemFontFile;
}

void *GetSystemDataFile(void)
{
    return *(void *volatile *)&g_SystemDataFile;
}

static void systemReleaseResource(void *volatile *slot)
{
    HeapUnpinBlock((HeapBlock *)*slot);
    HeapFree(*slot);
    *slot = NULL;
}

void SystemFreeFont(void)
{
    systemReleaseResource((void *volatile *)&g_SystemFontFile);
}

void SystemFreeData(void)
{
    systemReleaseResource((void *volatile *)&g_SystemDataFile);
}

void SystemInitialize(void *font, void *data)
{
    SystemInitializeFont(font);
    SystemInitializeData(data);
}

void SystemTransferPaletteToVRAM(short x, short y)
{
    RECT destination = {x, y, 32, 1};
    LoadImage(&destination, (u_long *)g_SystemPaletteData);
    *(volatile s16 *)&g_SystemPalette1 = (s16)GetClut(x, y);
    *(volatile s16 *)&g_SystemPalette2 = (s16)GetClut((int)x + 16, y);
}

static u8 systemDialogDimension(u16 *base, int index, unsigned component)
{
    unsigned directory = *(const volatile u16 *)base;
    uint32_t position = 6u + 2u * ((uint32_t)index + directory) + component;
    int64_t offset = position <= INT32_MAX ? (int64_t)position : (int64_t)position - INT64_C(4294967296);
    return *(const volatile u8 *)((uintptr_t)base + (uintptr_t)offset);
}

u8 DialogGetWidth(u16 *base, int index)
{
    return systemDialogDimension(base, index, 0u);
}

u8 DialogGetHeight(u16 *base, int index)
{
    return systemDialogDimension(base, index, 1u);
}
