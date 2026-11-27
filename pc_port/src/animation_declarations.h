/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Blizz127 and contributors
 */
#ifndef XENO_NATIVE_ANIMATION_DECLARATIONS_H
#define XENO_NATIVE_ANIMATION_DECLARATIONS_H

#include <stddef.h>
#include <stdint.h>
#include <psx/types.h>

typedef int8_t s8;
typedef uint8_t u8;
typedef int16_t s16;
typedef uint16_t u16;
typedef int32_t s32;
typedef uint32_t u32;
typedef signed long long s64;
typedef unsigned long long u64;

#include "psyq/libgte.h"
#include "animation_sprite_records.h"
#include <psx/inline_c.h>
#include "battle_guest_resolver.h"

/* The work-list service takes an opaque native record. */
typedef struct WorkListEntry WorkListEntry;

/* Data symbols are shared with other game translation units. */
extern MATRIX D_80018644, D_8004FBB8;
extern s32 D_80059198;
extern u8 D_800591AD, D_800591B0, D_800591B3;
extern u32 D_800592E4;
extern s16 D_800592E8, D_800592EA;
extern u8 D_8006BE10[];
extern u32 D_8006F99C[4], D_8006F9AC[4];
extern u8 D_800C3EB0[];
extern s32 g_WorkListCurTimer;
extern u8 D_8004FC40[256];

/* Host pointers, sprite services and animation state. */
void *PsxMemory_GuestToHost(uint32_t) __attribute__((weak));
uint32_t PsxMemory_HostToGuest(const void *) __attribute__((weak));
void AnimScriptTick(void *);
void *HeapAlloc(u_int, u_int);
void HeapChangeCurrentUser(u_int, char **);
u_int HeapFree(void *);
void Retail8003F738RotMatrix(const SVECTOR *, MATRIX *);
void func_8001CE74(WorkListEntry *);
void func_8001D2B0(void *, s16);
void func_8001D4E8(void *);
int func_8001EE68(u8 *);
void func_8001F6B0(void *);
void func_8001FB30(void);
s32 func_80022CAC(void *, s32);
void func_80022CDC(u8 *);
void func_80022D44(void *);
s32 func_80023124(s32, s32);
void func_80023290(u8 *, s32);
void *func_80023B84(void *, void *, void *);
void func_800245D8(void *, s16);
int func_8002C3E8(u8 *);
void func_8002C59C(u8 *);
void func_8002C8CC(u8 *, void *, s32);
void func_8002CB54(u8 *, u32 *, u32 *);
void func_8002CC10(s32, s32);
void func_80039E60(s32);
void func_800B2AEC(void *, void *, void *, s32, s32, s32);
MATRIX *ScaleMatrixL(MATRIX *, VECTOR *);

/* Entry points shared between native animation providers. */
s32 AnimScriptStackPopU16(SpriteData *);
s32 AnimScriptStackPopU24(SpriteData *);
u_char AnimScriptStackPopU8(SpriteData *);
void AnimScriptStackPushU16(SpriteData *, u16);
void AnimScriptStackPushU24(SpriteData *, s32);
void AnimScriptStackPushU8(SpriteData *, u8);
int AnimationScriptOpcodeIsNoop(u32);
void SpriteComputeTransformMatrix(SpriteData *);
void SpriteSetColor(SpriteData *, u8, u8, u8);
void SpriteSetScale(SpriteData *, short);
void SpriteSetSpecialAnimFile(SpriteData *, void *);
void *func_8001FBA4(SpriteData *, u8 *);
void func_8001FBE4(void *, u32, void *);
int func_80021AD8(s32, s32);
void func_80021B04(void *, s16, s16, s16);
void func_80021B14(void *, s32, s32, s32);
void func_80021B24(void *, void *);
void func_80021B48(void *, void *);
void func_80021B6C(SpriteData *);
void func_80021BCC(void *, u32);
void func_80021BF8(void *, s32);
void func_80021C00(void *, u32);
void func_80021D3C(void *, s32, s32);
void func_80021D50(void *, void *);
void func_80021EBC(void *, void *);
void func_80021FB8(u8 *, u8);
void func_80021FC0(void *, s32);
void func_80021FE0(void *, s16);
void func_80022038(SpriteData *);
void func_80022224(SpriteAnimPackage *, void *, SVEC2, SVEC2, u32);
void func_800222BC(SpriteData *, SpriteAnimPackageFileHeader *);
void func_800223B0(void *, s16);
void func_80022660(void *, void *, s32);
void func_80022974(void *);
s32 func_80022A00(s32 *);

#endif
