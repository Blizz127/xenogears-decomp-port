#include "common.h"
#include "psx_memory.h"
#include <string.h>

extern u8 D_800591AD;
extern s32 D_800591A8;
extern s32 func_80023440(void*);
extern s32 func_80023468(s32);
extern void* func_80023A48(s32, s32, void*, s32, void*);
extern void func_80023538(void*, void*);
extern void func_80024730(void*);

static u32 read32(const u8* p) { u32 v; memcpy(&v,p,4); return v; }
static u16 read16(const u8* p) { u16 v; memcpy(&v,p,2); return v; }
static void write32(u8* p,u32 v) { memcpy(p,&v,4); }
static void write16(u8* p,u16 v) { memcpy(p,&v,2); }

/* Pointer stored in the existing PSX RAM slot, not a new native global.
 * Accept PSX RAM aliases and native pointers already published by adapters. */
static u8* playerPointer(u32 address)
{
    if (address == 0) return NULL;
    if (address < 0x200000u || (address & 0xFFE00000u) == 0x80000000u
        || (address & 0xFFE00000u) == 0xA0000000u) return PSX_ADDR(address);
    return (u8*)(uintptr_t)address;
}

/* Retail main executable [80023FD8,80024294),700 bytes, SHA-256
 * 6291763a42ce40bf3073a031287166b8401aa0e6655b0cd4a47cfad702393c65.
 * This restores the constructor body, not its complete callee closure. */
u8* func_80023FD8(s32 index, u8* package, s16* position, s32 extra)
{
    u32 scripts = read32(package + 0x10);
    u32 entry = scripts + (u32)index * 2u;
    u8* script = (u8*)(uintptr_t)(scripts + read16((u8*)(uintptr_t)(entry + 2u)));
    s32 type = func_80023440(script);
    s32 mode = func_80023468(type);
    u8* wrapper = func_80023A48(type, mode, package, extra, NULL);
    u8* sprite = wrapper + 0x38;
    write32(wrapper + 0x14, read32(wrapper + 0x14) | 0x20000000u);
    package = (u8*)(uintptr_t)read32(sprite + 0x24);
    write32(sprite + 0x70, 0);
    write32(sprite + 0x74, 0);
    if (D_800591AD != 0) {
        u8* parent = playerPointer(read32(PSX_ADDR(0x800C3E1C)));
        if (parent != NULL) {
            write32(sprite + 0x44, read32(parent + 0x44));
            write32(sprite + 0x48, read32(parent + 0x48));
            write32(sprite + 0x74, read32(parent + 0x74));
            write32(sprite + 0x18, read32(parent + 0x18));
            write16(sprite + 0x32, read16(parent + 0x32));
            write32(sprite + 0x40, (read32(sprite + 0x40) & ~0x1F00u) | (read32(parent + 0x40) & 0x1F00u));
            write32(sprite + 0x3C, (read32(sprite + 0x3C) & ~8u) | (read32(parent + 0x3C) & 8u));
            write32(sprite + 0x3C, (read32(sprite + 0x3C) & ~0x10u) | (read32(parent + 0x3C) & 0x10u));
            sprite[0x3D] = parent[0x3D];
            write16(sprite + 0x2C, read16(parent + 0x2C));
            write32(sprite + 0x3C, (read32(sprite + 0x3C) | 0x04000000u) & ~4u);
            write32(sprite + 0xAC, (read32(sprite + 0xAC) & ~4u) | (read32(parent + 0xAC) & 4u));
            write32(sprite + 0xAC, (read32(sprite + 0xAC) & 0xFFF8007Fu) | (read32(parent + 0xAC) & 0x7FF80u));
            u32 a8 = read32(sprite + 0xA8);
            write32(sprite + 0x7C, read32(parent + 0x7C));
            write32(sprite + 0x7C, read32(parent + 0x7C));
            u32 split = (read32(parent + 0xA8) >> 30) | ((read32(parent + 0xAC) & 3u) << 2);
            write32(sprite + 0xA8, (a8 & 0x3FFFFFFFu) | (split << 30));
            write32(sprite + 0xAC, (read32(sprite + 0xAC) & ~3u) | (split >> 2));
            write32(sprite + 0x50, read32(parent + 0x50));
            sprite[0x8D] = parent[0xAF];
        }
    }
    u32 flags40 = read32(sprite + 0x40), flags3c = read32(sprite + 0x3C);
    u16 scale = (u16)D_800591A8;
    write32(sprite + 0x44, 0);
    write32(sprite + 0x48, 0);
    write16(sprite + 0x34, 0);
    write32(sprite + 0x24, (u32)(uintptr_t)package);
    write32(sprite + 0x40, (flags40 & 0xFFFE1FFFu) | (((u32)type & 0xFu) << 13));
    write32(sprite + 0x3C, (flags3c & ~3u) | ((u32)mode & 3u));
    write16(sprite + 0x82, scale);
    for (u32 axis = 0; axis < 3; ++axis)
        write32(sprite + axis * 4, (u32)(s32)(s16)read16((u8*)position + axis * 2) << 16);
    func_80023538(sprite, script);
    func_80024730(wrapper);
    return wrapper;
}
