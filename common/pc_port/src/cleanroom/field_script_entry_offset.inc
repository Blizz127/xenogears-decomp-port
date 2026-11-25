#include <stdint.h>

u16 FieldScriptGetBytecodeOffset(int script, int routine)
{
    uintptr_t base = (uintptr_t)g_FieldCurScriptFile;
    u32 displacement = 132u + (u32)script * 64u + (u32)routine * 2u;
    uintptr_t address = base <= UINT32_MAX
        ? (uintptr_t)((u32)base + displacement)
        : base + (intptr_t)(s32)displacement;
    const unsigned char *entry = (const unsigned char *)address;
    return (u16)(entry[0] | (u16)entry[1] << 8);
}
