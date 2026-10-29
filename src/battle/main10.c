#include "common.h"


#ifndef XENO_PC_PORT
extern u8* D_800C3EAC;
extern u8 D_800C402F[];
extern u8 D_800D2DE4[];
extern u8 D_800CCD64[];
extern void func_800785D4(u8, u8);

/* Four-byte records at D_800C3D18, indexed by actor - 3 (enemies). */
typedef struct {
    u8 unk0;
    u8 unk1;
    u8 unk2;
    u8 unk3;
} BattleEnemyFlags4;
extern BattleEnemyFlags4 D_800C3D18[];

/* func_80078D6C.s: set the current command row's D_800C402F byte to 0xF9
 * (as func_800791FC does with 0xF4), call func_800785D4(a, b), then mark
 * actor a: D_800D2DE4[a] = 0xFF, byte 3 of its D_800C3D18 record = 1, and
 * keep only bit 15 of the halfword at D_800CCD64 + a * 368.
 * The record index a - 3 goes through the struct array (a raw
 * D_800C3D1B[(a - 3) * 4] folds the -12 into the address), and the 368
 * offset is its own variable so both halfword accesses use D_800CCD64($at)
 * (a multiply in the address makes expand load D_800CCD64 into a
 * register). */
void func_80078D6C(u8 a, u8 b) {
    u32 o;

    D_800C402F[D_800C3EAC[0x2DA] * 0x48] = 0xF9;
    func_800785D4(a, b);
    D_800D2DE4[a] = 0xFF;
    D_800C3D18[a - 3].unk3 = 1;
    o = a * 368;
    *(u16*)(D_800CCD64 + o) &= 0x8000;
}

extern u8 D_800D2E62[];
extern u8 D_800C3E3D[];
typedef struct {
    u32 w[16];
} BattleAiRec64;
extern BattleAiRec64 D_800D3400[];
extern u8 D_800CCCE8[];
extern u8 D_800C4022[];
extern u8 D_800C400B[];
extern u16 func_80089C9C(s32 mask, u8 idx);
extern void* memmove(void* dst, const void* src, u32 n);

/* func_80078E24.s: copy enemy src into actor slot dst, src being the first
 * actor 0..10 that func_80089C9C selects from the u16 mask at
 * D_800D2E62 + idx * 8: copy its D_800C3E3D byte, the first four words of
 * its 64-byte D_800D3400 record, bytes 0-1 of its D_800C3D18 record and
 * its 0x170-byte D_800CCCE8 actor record (memmove), then write a 0xFB
 * command (src in the halfword, dst in the byte at +0x23) at the
 * D_800C3EAC+0x2DA row and advance the row cursor.  As in retail, src is
 * left unassigned when no actor is selected.  k (a copy of i) gives
 * retail's loop-top copy into $s1; the halfword goes through a u16 array
 * index, so D_800C3EAC is not reloaded after it (only the byte stores
 * force reloads). */
void func_80078E24(u8 dst, u8 idx) {
    s32 i;
    u8 src;
    u8 k;

    for (i = 0; i < 11; i++) {
        k = i;
        if (func_80089C9C(*(u16*)(D_800D2E62 + (idx << 3)), k) != 0) {
            src = k;
            break;
        }
    }
    D_800C3E3D[dst] = D_800C3E3D[src];
    D_800D3400[dst - 3].w[0] = D_800D3400[src - 3].w[0];
    D_800D3400[dst - 3].w[1] = D_800D3400[src - 3].w[1];
    D_800D3400[dst - 3].w[2] = D_800D3400[src - 3].w[2];
    D_800D3400[dst - 3].w[3] = D_800D3400[src - 3].w[3];
    D_800C3D18[dst - 3].unk0 = D_800C3D18[src - 3].unk0;
    D_800C3D18[dst - 3].unk1 = D_800C3D18[src - 3].unk1;
    memmove(D_800CCCE8 + dst * 368, D_800CCCE8 + src * 368, 0x170);
    ((u16*)D_800C4022)[D_800C3EAC[0x2DA] * 0x24] = src;
    D_800C402F[D_800C3EAC[0x2DA] * 0x48] = 0xFB;
    D_800C400B[D_800C3EAC[0x2DA] * 0x48] = dst;
    D_800C3EAC[0x2DA]++;
}
#endif


#ifndef XENO_PC_PORT
extern u8 D_800D2E5D[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800D2E60[];
#endif
extern void func_80079ED8(u8 a0, u8 a1, u8 a2, s32 a3);


/* func_80079054.s */
void func_80079054(u8 a, u8 idx) {
    func_80079ED8(a & 0xFF, *(u8*)(D_800D2E5D + ((u32)idx << 3)),
                  *(u8*)(D_800D2E60 + ((u32)idx << 3)), 0);
}
