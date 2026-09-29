/*
 * Field object overlay (archive 0x6B9, retail load address 0x801DC000).
 *
 * Phase-2B treated func_801E742C / 7D14 as mid-function entries of
 * member_change_menu.bin. That was the wrong overlay: during field play
 * archive 0x6B9 occupies 0x801DC000, and 0x801E72CC..0x801E8330 are
 * standalone functions in that blob (func_801E72CC is already ported in
 * game_overrides.c).
 *
 * This file ports the instantiate + draw path MAP16 needs so the four
 * registered object archives become field models (func_8002CB54 /
 * func_8002C8CC / func_8002C700), the same pipeline FieldLoad uses.
 */
#include "common.h"
#include "field/main.h"
#include "field/actor.h"
#include "psyq/libgte.h"
#include "psyq/libgpu.h"
#include "psx/inline_c.h"
#include "psx_memory.h"
#include "work_list_callback.h"
#include "field_effect_constructor.h"
#include "field_clip_prelude.h"
#include "field_clip_control.h"
#include "model_prim_link.h"
#include "system/memory.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HEAP_USER_MASA 0x4
extern void* HeapAlloc(u32 size, u32 flags);
extern u_int HeapFree(void* pMem);
extern void HeapChangeCurrentUser(u32 userTag, char** pContentTypes);
extern s32 func_8002C644(u8* a0);
extern void OuterProduct12(VECTOR* first, VECTOR* second, VECTOR* output);

extern int func_8002C3E8(u8* pModel);
extern void func_8002CBBC(u8* modelData);
extern void func_8002CB54(u8* modelData, u32* out1, u32* out2);
extern void func_8002C8CC(u8* a0, void* a1, s32 a2);
extern s32 func_8002C4BC(u8* pBlock);
extern void func_8002CC10(u16 x, u16 y);
extern void func_8002CC74(u16 a0, u16 a1);
extern s32 func_8002C700(void* a0, void* a1, void* a2, s32 a3);
extern s32 func_8002DDE4(void* pImageData, s32 texMode, s32 texX, s32 texY,
                         s32 clutMode, s32 clutX, s32 clutY);
extern unsigned int ResolveArchiveEntryPointers(u32* pFile);
extern u16 D_80059308;
extern u16 D_8005930C;
extern s32 D_80050104;
extern s32 D_80059578;
extern int g_FieldNumActors;
extern FieldActor* volatile g_FieldActors;

/* Cross-TU table consumed by func_801E72CC (game_overrides.c) and the
 * field object-anim path. 10 slots, matching overlay slti 0xA. */
u32 D_801E8670[10];
/* Archive 6B9 initialized word at 801E85CC is zero. Written by 7378 and
 * consumed after pose lookup by E39F0 opcode 13. */
u32 D_801E85CC;
/* Retail initialized-data bytes are zero in archive 6B9. Keep registry
 * entries packed and effect records contiguous for their indexed consumers. */
u32 D_801E85F4[8][2];
u32 D_801E8640;
u16 D_801E8648[20];
u16 D_801E869C;
/* Retail 0x801E8698: sway magnitude derived from D_801E869C each frame. */
s16 D_801E8698;
void* D_801E8644;
/* Retail 0x801E86A0: pointer, signed capacity, signed next index. */
u8 D_801E86A0[8];
/* Retail 0x801E86A8..86B0: track-pool header (pointer, next index, capacity),
 * passed to 35D0/36BC/39F0. The following slot selector is a separate owner. */
u8 D_801E86A8[8];
s16 D_801E86B0;
s16 D_801E863C;

#define OVLY_NODE_STRIDE 0x7C
#define OVLY_GROUP_STRIDE 0x38
#define OVLY_OBJ_SIZE 0x134
#define OVLY_SLOT_MAX 10
#define OVLY_CLIP_OPS 0x71

extern MATRIX* RotMatrix(SVECTOR* r, MATRIX* m);

typedef struct OvlyPtrTab {
    u32* ptrs;
    s32 count;
} OvlyPtrTab;

static OvlyPtrTab s_ptrTab[OVLY_SLOT_MAX];

static int OvlyDiag(void) {
    static int s_on = -1;
    if (s_on < 0) {
        const char* env = getenv("XENO_FIELD_DIAG");
        s_on = (env != NULL && env[0] != '\0' && env[0] != '0');
    }
    return s_on;
}

static void OvlyIdentMatrix(MATRIX* m, s32 tx, s32 ty, s32 tz) {
    memset(m, 0, sizeof(*m));
    m->m[0][0] = 0x1000;
    m->m[1][1] = 0x1000;
    m->m[2][2] = 0x1000;
    m->t[0] = tx;
    m->t[1] = ty;
    m->t[2] = tz;
}

/* Extra bytes after the 2-byte opcode word, from overlay 39F0 handlers.
 * Wait (01) is special-cased. 04-07 etc. are no-ops at 801E5974. */
static const u8 s_clipExtra[OVLY_CLIP_OPS] = {
    [0x01] = 2, [0x11] = 2, [0x13] = 4, [0x14] = 2, [0x15] = 2, [0x16] = 2,
    [0x17] = 2, [0x18] = 2, [0x1A] = 12, [0x1D] = 18, [0x22] = 2, [0x23] = 2,
    [0x25] = 8, [0x28] = 2, [0x29] = 2, [0x2E] = 2, [0x31] = 2, [0x32] = 2,
    [0x33] = 2, [0x34] = 2, [0x35] = 2, [0x36] = 4, [0x37] = 2, [0x38] = 4,
    [0x39] = 2, [0x3B] = 2, [0x3C] = 2, [0x40] = 12, [0x41] = 6, [0x44] = 6,
    [0x45] = 6, [0x46] = 6, [0x47] = 6, [0x49] = 6, [0x4B] = 6, [0x4C] = 6,
    [0x4D] = 6, [0x4E] = 6, [0x50] = 6, [0x54] = 2, [0x55] = 4, [0x56] = 2,
    [0x5B] = 2, [0x5C] = 2, [0x5D] = 2, [0x5E] = 2, [0x5F] = 2, [0x62] = 8,
    [0x63] = 2, [0x64] = 2, [0x6B] = 2, [0x6E] = 2, [0x70] = 2,
};

/* Retail 801E3534: install clip table and zero per-object anim state. */
static void OvlyClipInit(u8* obj, u8* clipTable, u32 clipAux) {
    s32 i;
    *(u16*)(obj + 0x3C) = 0xFFFF;
    obj[0x5C] = 0xFF;
    obj[0x39] = 0x6B;
    *(u32*)(obj + 8) = (u32)(uintptr_t)clipTable;
    *(u32*)(obj + 0xC) = 0;
    *(u32*)(obj + 0x10) = 0;
    *(u32*)(obj + 0x14) = clipAux;
    *(u32*)(obj + 0x18) = 0;
    obj[0x2B] = 0;
    *(s16*)(obj + 0x98) = -1;
    *(u16*)(obj + 0x58) = 0;
    obj[0x35] = 0;
    obj[0x37] = 0;
    obj[0x38] = 0;
    *(s16*)(obj + 0x3A) = -1;
    for (i = 0x70; i <= 0x8C; i += 2) {
        *(u16*)(obj + i) = 0;
    }
    *(u16*)(obj + 0x8E) = 1;
    obj[0x36] = 0;
    *(s16*)(obj + 0x1E) = -1;
}

/* Retail [801E3534,801E35D0), SHA-256
 * dcca05b1af95b39ec18df4bb4a29aed5b33da46091e9bbd34f20c607882edc7b. */
void func_801E3534(u8* obj, void* unused, u8* clipTable, u32 clipAux)
{
    (void)unused;
    OvlyClipInit(obj, clipTable, clipAux);
}

/* Bounded retail-backed interpreter entry. Unsupported retail opcodes remain
 * fail-closed inside func_801E39F0; no host fallback is substituted. */
void func_801E39F0(u8*, void*, s32, s32, s32);

/* Retail [801E35D0,801E36BC), SHA-256
 * 7b59dc488b040e5b2b64a898276871a668e7a1795002fb826e140ac020ddfb9b. */
void func_801E35D0(u8* obj, u8* src, void* context, s32 index)
{
    if (obj == NULL || src == NULL) return;
    if (obj[0x2B] != 0) {
        if (obj[0x2B] < 5) ++obj[0x2B];
        obj[0x2A + obj[0x2B]] = src[0x20];
        obj[0x2E + obj[0x2B]] = (u8)index;
        return;
    }
    u32 entry;
    if (index < 0x50) {
        entry = *(u32*)(uintptr_t)(*(u32*)(src + 8) + (u32)index * 4u);
    } else {
        u8* aux = (u8*)(uintptr_t)*(u32*)(src + 0xC);
        entry = *(u32*)(uintptr_t)(*(u32*)(aux + 4) + (u32)index * 4u - 0x138u);
    }
    *(u32*)(obj + 0x10) = entry;
    u16 phase = (u16)D_801E863C;
    *(u16*)(obj + 0x42) = 0;
    *(u16*)(obj + 0x40) = 0;
    *(u32*)(obj + 0x50) = 0;
    *(u32*)(obj + 0x54) = 0;
    *(u32*)(obj + 0x4C) = 0;
    obj[0x23] = 0;
    *(u16*)(obj + 0x10A) = phase;
    func_801E39F0(obj, context, -1, 1, 0);
}

/* Retail [801E5C74,801E5CD8), SHA-256
 * aa7406e2a5c218eda80812514fa67b6fad6eb6f82f1325fcc91140decdf6fb09. */
void func_801E5C74(u8* object, u8* data, s32 loop)
{
    u32 stream;
    if (*(u16*)(data + 0x12) == 0) {
        *(u16*)(object + 0x98) = 0xFFFF;
        return;
    }
    *(u16*)(object + 0x98) = 0;
    *(u16*)(object + 0x9A) = loop != 0 ? *(u16*)(data + 2) : 0xFFFF;
    *(u16*)(object + 0x9C) = 0;
    *(u16*)(object + 0x9E) = *(u16*)(data + 0x12);
    /* The word may overlap halfwords written above. memcpy retains the
     * retail load ordering without C's incompatible-type alias assumption. */
    memcpy(&stream, data + 0x14, sizeof(stream));
    stream += (u32)(uintptr_t)data;
    memcpy(object + 0xA0, &stream, sizeof(stream));
    memcpy(object + 0xA4, &stream, sizeof(stream));
}

/* Retail [801E63A8,801E6578), SHA-256
 * 4ddc077241fbfd1dea89eb13ff3500e1f20af9b3c45c4f70c32fd4d239e3973d. */
void func_801E63A8(u8* object)
{
    s32 selector = *(s16*)(object + 0x58);
    u32 slot = 0;
    if (selector == 0xFF) {
        u32 bits = *(u16*)(object + 0x10A);
        while (slot < 8 && ((bits >> slot) & 1u) == 0) ++slot;
    }
    if (selector == 0xFE) slot = (u16)D_801E86B0;
    if (selector == 0xFD) slot = object[0x20];
    if (selector == 0xFC) slot = object[0x21];
    if (selector == 0xFA) slot = 10;
    if ((u32)selector - 1u < 0x7Fu) slot = (u32)selector - 1u;
    u8* target = (u8*)(uintptr_t)*(u32*)(uintptr_t)
        ((u32)(uintptr_t)D_801E8670 + slot * 4u);
    if (target == NULL || slot == object[0x20]) return;
    s32 child = *(s16*)(object + 0x5A);
    u8* root = (u8*)(uintptr_t)*(u32*)(target + 4);
    MATRIX* matrix;
    if (child != 0) {
        matrix = (MATRIX*)g_PsxScratchpad;
        u32 childMatrix = (u32)(uintptr_t)root + (u32)child * 124u + 0x2Cu;
        CompMatrix((MATRIX*)(root + 0xC), (MATRIX*)(uintptr_t)childMatrix, matrix);
    } else matrix = (MATRIX*)(root + 0xC);
    SetRotMatrix(matrix);
    SetTransMatrix(matrix);
    u32 xy, zpad;
    memcpy(&xy, object + 0x64, 4);
    memcpy(&zpad, object + 0x68, 4);
    MTC2(xy, 0);
    MTC2(zpad, 1);
    doCOP2(0x00480012);
    u32 result[3] = {MFC2(25), MFC2(26), MFC2(27)};
    u8* ownRoot = (u8*)(uintptr_t)*(u32*)(object + 4);
    for (u32 axis = 0; axis < 3; ++axis)
        *(u16*)(object + 0x88 + axis * 2) = (u16)result[axis];
    u8* track = (u8*)(uintptr_t)*(u32*)(ownRoot + 0x70);
    if (track != NULL && (u32)track[2] - 7u < 2u) {
        for (u32 axis = 0; axis < 3; ++axis)
            *(u16*)(track + 0xA + axis * 2) = (u16)result[axis];
    }
}

/* Retail [801E6F64,801E7094), SHA-256
 * 641195ca393cf6c7f11befe4ad49bd40aa20a7b045b327f0b9a1a393af3f75b4. */
void func_801E6F64(void* task)
{
    u8* wrapper = task;
    u8* attachment = (u8*)(uintptr_t)((u32)(uintptr_t)wrapper
        + (u32)(s32)*(s16*)(wrapper + 0xBE));
    s32 child = *(s16*)(attachment + 0xC);
    u8* parent = (u8*)(uintptr_t)*(u32*)(attachment + 8);
    u8* root = (u8*)(uintptr_t)*(u32*)(parent + 4);
    MATRIX* matrix;
    if (child != 0) {
        matrix = (MATRIX*)g_PsxScratchpad;
        u32 childMatrix = (u32)(uintptr_t)root + (u32)child * 124u + 0x2Cu;
        CompMatrix((MATRIX*)(root + 0xC), (MATRIX*)(uintptr_t)childMatrix, matrix);
    } else matrix = (MATRIX*)(root + 0xC);
    SetRotMatrix(matrix);
    SetTransMatrix(matrix);
    u32 xy, zpad;
    memcpy(&xy, attachment + 0x10, 4);
    memcpy(&zpad, attachment + 0x14, 4);
    MTC2(xy, 0);
    MTC2(zpad, 1);
    doCOP2(0x00480012);
    u32 position[3] = {MFC2(25), MFC2(26), MFC2(27)};
    if (*(s16*)(attachment + 0xE) != 0) {
        parent = (u8*)(uintptr_t)*(u32*)(attachment + 8);
        position[1] = (u32)(s32)*(s16*)(parent + 0x60);
    }
    for (u32 axis = 0; axis < 3; ++axis)
        *(u32*)(wrapper + 0x38 + axis * 4) = position[axis] << 16;
    PcPort_WorkListInvokeSavedCallback(*(u32*)(attachment + 4), wrapper);
}

extern u8* func_80023FD8(s32, u8*, s16*, s32);
extern void func_80021FE0(void*, s16);
extern void func_800223B0(void*, s16);
extern void SpriteSetScale(SpriteData*, short);
extern void (*WorkListTaskGetTaskCallback(void*))(void*);
extern void TimerWorkListSetTaskCallback(void*, void (*)(void*));

/* Retail [801E6E48,801E6F64), SHA-256
 * 74668c6567f1addcc854107a218b19d830fa31e737d2fde330cd918e0927c733. */
void func_801E6E48(u8* package, s32 index, s16* position,
                  s32 angle, s32 scale, u8* data, u8* parent)
{
    u8* wrapper = func_80023FD8(index, package, position, 0x18);
    u8* sprite = wrapper + 0x38;
    func_80021FE0(sprite, (s16)angle);
    func_800223B0(sprite, (s16)angle);
    SpriteSetScale((SpriteData*)sprite, (s16)scale);
    u8* attachment = (u8*)(uintptr_t)((u32)(uintptr_t)wrapper
        + (u32)(s32)*(s16*)(wrapper + 0xBE));
    *(u32*)(attachment + 8) = (u32)(uintptr_t)parent;
    *(u16*)(attachment + 0xC) = data[5];
    if (data[0x13] != 0) {
        void (*previous)(void*) = WorkListTaskGetTaskCallback(wrapper);
        *(u32*)(attachment + 4) = (u32)(uintptr_t)previous;
        TimerWorkListSetTaskCallback(wrapper, func_801E6F64);
        *(u16*)(attachment + 0x10) = *(u16*)(data + 6);
        *(u16*)(attachment + 0x12) = *(u16*)(data + 8);
        *(u16*)(attachment + 0x14) = *(u16*)(data + 0xA);
        *(u16*)(attachment + 0xE) = data[0xC];
    }
}

/* Retail [801E6D94,801E6E48), SHA-256
 * c568e5ac466edda111c10c930ff6a52fa982ea2621cb3f59640f6ef2f51cb0e1. */
void func_801E6D94(u8* object, u8* node, s32 flags)
{
    node[7] = (u8)((u32)flags & 1u);
    if (flags & 0x80) {
        u8* root = (u8*)(uintptr_t)*(u32*)(object + 4);
        u8* child = root + 124;
        for (u32 index = 1; index < *(u16*)(root + 0xA); ++index, child += 124) {
            if (*(u32*)child == (u32)(uintptr_t)node)
                func_801E6D94(object, child, flags);
            root = (u8*)(uintptr_t)*(u32*)(object + 4);
        }
    }
}

/* Retail [801E7094,801E7298), SHA-256
 * 5c90ef3a247d87241cc96c3b48d765f87fc62fb943df332a9d546f39111b38fa. */
void func_801E7094(u8* object, u8* node, s32 flags, s32 x, s32 y, s32 z)
{
    u32 kind = (u32)flags & 7u;
    s32 values[3] = {x, y, z};
    if (kind == 1) {
        for (u32 axis = 0; axis < 3; ++axis) {
            u32 value = (u32)(s32)(s16)values[axis];
            if (flags & 0x20) value += *(u32*)(node + 0x5C + axis * 4);
            *(u32*)(node + 0x5C + axis * 4) = value;
        }
    } else {
        u32 offset = kind == 0 ? 0x54 : 0x4C;
        for (u32 axis = 0; axis < 3; ++axis) {
            u32 value = (u32)values[axis];
            if (flags & 0x20) value += *(u16*)(node + offset + axis * 2);
            *(u16*)(node + offset + axis * 2) = (u16)value;
        }
    }
    node[4] = 1;
    node[5] = 1;
    if (flags & 0x80) {
        u8* root = (u8*)(uintptr_t)*(u32*)(object + 4);
        u8* child = root + 124;
        for (u32 index = 1; index < *(u16*)(root + 0xA); ++index, child += 124) {
            if (*(u32*)child == (u32)(uintptr_t)node)
                func_801E7094(object, child, (u8)flags, (s16)x, (s16)y, (s16)z);
            root = (u8*)(uintptr_t)*(u32*)(object + 4);
        }
    }
}

/* Retail [801E6668,801E66BC), SHA-256
 * 04a67648e51fed448553166db07002d74bc3dd0addbf007985a7eeb83c9c9e09. */
void func_801E6668(u8* source, u8* target)
{
    u32 count = *(u16*)(source + 0xA);
    for (u32 index = 1; index < count; ++index) {
        u8* from = source + index * 0x7C;
        u8* to = target + index * 0x7C;
        if (from[7] != 0) {
            from[7] = 0;
            to[7] = 1;
        }
    }
}

/* Retail [801E67F8,801E6830), SHA-256
 * 3196805b21c24b52a7665109676897485a3738c80ddfa2c49076633956c1fef3. */
u32 func_801E67F8(void)
{
    u32 bits = (u16)D_801E863C;
    u32 index = 0;
    while (index < 8 && ((bits >> index) & 1u) == 0) ++index;
    return index;
}

/* Retail [801E6830,801E6910), SHA-256
 * 15e3cad3fe23b5c63e7fb5311c088884c7e14cd8c6ca101e198b55108815ea48. */
u32 func_801E6830(u8* obj, s32 selector, u16* mask)
{
    u32 index = (u8)selector;
    switch (index) {
    case 0xFF: index = func_801E67F8(); break;
    case 0xFE: index = (u8)D_801E86B0; break;
    case 0xFD:
    case 0xF9: index = obj[0x20]; break;
    case 0xFC: index = obj[0x21]; break;
    case 0xFA: index = 10; break;
    case 0xF8: index = (u8)(obj[0x20] * 2u + 8u); break;
    case 0xF7: index = (u8)(obj[0x20] * 2u + 9u); break;
    }
    *mask = (u16)(1u << (index & 31u));
    return index;
}

/* Retail [801E6910,801E6974), SHA-256
 * 8419a785639aafafb67573a38a3fb70a9efbdee1ecb90d7cbc798c17011f8fd2. */
u32 func_801E6910(u8* obj, s32 index, u32* flags)
{
    *flags = 0;
    if ((u8)index >= 0xFE) {
        index = obj[0x2A] & 0x7F;
        *flags = obj[0x2A] & 0x80;
    }
    u32 selected = (u8)index;
    if (selected < 0x40)
        return *(u32*)(uintptr_t)(*(u32*)(obj + 0x14) + selected * 4u + 4u);
    return *(u32*)(uintptr_t)(*(u32*)(obj + 0x18) + selected * 4u - 0xFCu);
}

static u8* OvlyClipLookup(u8* obj, s32 index) {
    u32 base;
    if (obj == NULL) {
        return NULL;
    }
    index &= 0xFF;
    if (index >= 0xFE) {
        index = obj[0x2A] & 0x7F;
    }
    if (index < 0x40) {
        base = *(u32*)(obj + 0x14);
        if (base == 0) {
            return NULL;
        }
        return (u8*)(uintptr_t)*(u32*)((u8*)(uintptr_t)base + index * 4 + 4);
    }
    base = *(u32*)(obj + 0x18);
    if (base == 0) {
        return NULL;
    }
    return (u8*)(uintptr_t)*(u32*)((u8*)(uintptr_t)base + index * 4 - 0xFC);
}

/* Retail [801DEF10,801DF0B4), SHA-256
 * e12a4093ee078d91cb5fbfd8699e8ced3af26cd2f1fe848bacc52b1e57bef7d4. */
void func_801DEF10(u8* root, u8* pose)
{
    s16 prefix = *(s16*)(pose + 6);
    u32 rotationLimit = *(u16*)(pose + 0xC);
    u32 translationLimit = *(u16*)(pose + 0xE);
    u16 flags = *(u16*)(pose + 4);
    s16* source = (s16*)(pose + 0x18);
    if (prefix == 0) source += (rotationLimit + 1u) * 3u;
    u32 children = (u16)(*(u16*)(root + 0xA) - 1u);
    u32 rotations = 0, translations = 0;
    for (u32 i = 0; i < children; ++i) {
        u8* node = root + (i + 1u) * 0x7Cu;
        if (!(flags & 1) && (u16)rotations < rotationLimit) {
            s16 x = *source++, y = *source++, z = *source++;
            ++rotations;
            if (*(s16*)(node + 0x54) != x || *(s16*)(node + 0x56) != y ||
                *(s16*)(node + 0x58) != z) {
                u8* track = (u8*)(uintptr_t)*(u32*)(node + 0x70);
                if (track == NULL || track[3] != 0xFF) {
                    *(s16*)(node + 0x54) = x;
                    *(s16*)(node + 0x56) = y;
                    *(s16*)(node + 0x58) = z;
                    node[4] = 1;
                    node[5] = 1;
                }
            }
        }
        if (!(flags & 2) && (u16)translations < translationLimit) {
            s32 x = *source++, y = *source++, z = *source++;
            ++translations;
            if (*(s32*)(node + 0x5C) != x || *(s32*)(node + 0x60) != y ||
                *(s32*)(node + 0x64) != z) {
                u8* track = (u8*)(uintptr_t)*(u32*)(node + 0x74);
                if (track == NULL || track[3] != 0xFF) {
                    *(s32*)(node + 0x5C) = x;
                    *(s32*)(node + 0x60) = y;
                    *(s32*)(node + 0x64) = z;
                    node[4] = 1;
                }
            }
        }
    }
}

/* Retail 801DEF10 (bounded): pose s16 triples onto child rot (+0x54) and
 * translation (+0x5C). Flag bit 0 skips rot, bit 1 skips trans. */
static void OvlyApplyPose(u8* root, u8* pose) {
    s32 nnodes;
    s32 i;
    s32 irot;
    s32 itrans;
    s16* src;
    u16 flags;
    u16 nrot;
    u16 ntrans;
    if (root == NULL || pose == NULL) {
        return;
    }
    nnodes = *(u16*)(root + 0xA);
    if (nnodes < 2 || nnodes > 0x40) {
        return;
    }
    flags = *(u16*)(pose + 4);
    nrot = *(u16*)(pose + 0xC);
    ntrans = *(u16*)(pose + 0xE);
    src = (s16*)(pose + 0x18);
    if (*(s16*)(pose + 6) == 0) {
        src += (s32)(nrot + 1) * 3;
    }
    irot = 0;
    itrans = 0;
    for (i = 1; i < nnodes; i++) {
        u8* node = root + i * OVLY_NODE_STRIDE;
        if ((flags & 1) == 0 && irot < (s32)nrot) {
            *(s16*)(node + 0x54) = src[0];
            *(s16*)(node + 0x56) = src[1];
            *(s16*)(node + 0x58) = src[2];
            src += 3;
            irot++;
        }
        if ((flags & 2) == 0 && itrans < (s32)ntrans) {
            *(s32*)(node + 0x5C) = (s32)src[0];
            *(s32*)(node + 0x60) = (s32)src[1];
            *(s32*)(node + 0x64) = (s32)src[2];
            src += 3;
            itrans++;
        }
    }
    if (OvlyDiag()) {
        static s32 s_poseMeta;
        if (s_poseMeta < 12) {
            fprintf(stderr,
                    "[obj-ovly] pose-meta flags=%04x nrot=%u ntrans=%u nnodes=%d "
                    "skip6=%d applied-rot=%d applied-t=%d\n",
                    (unsigned)flags, (unsigned)nrot, (unsigned)ntrans, (int)nnodes,
                    (int)(*(s16*)(pose + 6) == 0), (int)irot, (int)itrans);
            s_poseMeta++;
        }
    }
}

/* Retail 801E39F0 (bounded): apply velocity deltas to the root node, then
 * walk the clip bytecode. Wait/end/zero-delta are live; other opcodes skip
 * their payload so IP stays in sync. */
static void OvlyClipTick(u8* obj, s32 ticks, s32 startMode) {
    u8* node;
    u8* ip;
    u8* pkt;
    s32 n;
    s32 guard;
    static s32 s_clipLogs;

    if (obj == NULL) {
        return;
    }
    node = (u8*)(uintptr_t)*(u32*)(obj + 4);
    ip = (u8*)(uintptr_t)*(u32*)(obj + 0x10);
    if (ip == NULL) {
        return;
    }
    if (ticks < 0) {
        ticks = 0;
    }
    if (ticks > 8) {
        ticks = 8;
    }
    if (ticks > 0 && node != NULL) {
        for (n = 0; n < ticks; n++) {
            *(u16*)(obj + 0x70) = (u16)(*(u16*)(obj + 0x70) + *(u16*)(obj + 0x76));
            *(u16*)(obj + 0x72) = (u16)(*(u16*)(obj + 0x72) + *(u16*)(obj + 0x78));
            *(u16*)(obj + 0x74) = (u16)(*(u16*)(obj + 0x74) + *(u16*)(obj + 0x7A));
            *(u16*)(obj + 0x7C) = (u16)(*(u16*)(obj + 0x7C) + *(u16*)(obj + 0x82));
            *(u16*)(obj + 0x7E) = (u16)(*(u16*)(obj + 0x7E) + *(u16*)(obj + 0x84));
            *(u16*)(obj + 0x80) = (u16)(*(u16*)(obj + 0x80) + *(u16*)(obj + 0x86));
            *(s16*)(node + 0x54) = (s16)(*(s16*)(node + 0x54) +
                                         ((s16)*(u16*)(obj + 0x70) >> 3));
            *(s16*)(node + 0x56) = (s16)(*(s16*)(node + 0x56) +
                                         ((s16)*(u16*)(obj + 0x72) >> 3));
            *(s16*)(node + 0x58) = (s16)(*(s16*)(node + 0x58) +
                                         ((s16)*(u16*)(obj + 0x74) >> 3));
            /* Retail 39F0: scale vel by node+0x4C, rotate by +0x2C, then
             * add obj+0x1C * result / 0x1000 onto root translation. */
            {
                SVECTOR vel;
                VECTOR out;
                s32 scale = (s32)*(s16*)(obj + 0x1C);
                vel.vx = (s16)((s32)*(s16*)(obj + 0x7C) * (s32)*(s16*)(node + 0x4C) >> 12);
                vel.vy = (s16)((s32)*(s16*)(obj + 0x7E) * (s32)*(s16*)(node + 0x4E) >> 12);
                vel.vz = (s16)((s32)*(s16*)(obj + 0x80) * (s32)*(s16*)(node + 0x50) >> 12);
                ApplyMatrix((MATRIX*)(node + 0x2C), &vel, &out);
                *(s32*)(node + 0x5C) += (s32)((scale * out.vx) >> 12);
                *(s32*)(node + 0x60) += (s32)((scale * out.vy) >> 12);
                *(s32*)(node + 0x64) += (s32)((scale * out.vz) >> 12);
            }
        }
    }

    guard = 0;
    while (guard++ < 0x100) {
        u8 op;
        u8 param;
        s32 extra;
        pkt = ip;
        op = ip[0];
        param = ip[1];
        ip += 2;
        if (op >= OVLY_CLIP_OPS) {
            ip = pkt;
            break;
        }
        if (op == 0x00) {
            ip = pkt;
            break;
        }
        if (op == 0x01) {
            if (startMode) {
                ip = pkt;
                break;
            }
            {
                u16 wait = *(u16*)ip;
                s16 cur;
                ip += 2;
                cur = (s16)(*(u16*)(obj + 0x40) + (u16)ticks);
                *(u16*)(obj + 0x40) = (u16)cur;
                if (cur < (s16)wait) {
                    ip = pkt;
                    break;
                }
                *(u16*)(obj + 0x40) = 0;
                ticks = 0;
            }
            continue;
        }
        if (op == 0x0C) {
            for (n = 0x70; n <= 0x86; n += 2) {
                *(u16*)(obj + n) = 0;
            }
            continue;
        }
        if (op == 0x10 || op == 0x13) {
            s32 idx = param;
            if (op == 0x13) {
                u16 w1 = *(u16*)ip;
                ip += 2;
                ip += 2; /* second extra word (channel ids) */
                idx = (s32)(w1 & 0xFF);
            }
            {
                u8* pose = OvlyClipLookup(obj, idx);
                if (pose != NULL && (uintptr_t)pose > 0x10000) {
                    OvlyApplyPose(node, pose);
                    if (OvlyDiag() && s_clipLogs < 24 && node != NULL &&
                        *(u16*)(node + 0xA) >= 2) {
                        u8* ch = node + OVLY_NODE_STRIDE;
                        fprintf(stderr,
                                "[obj-ovly] pose op=%02x idx=%d pose=%p "
                                "child-r=(%d,%d,%d) child-t=(%d,%d,%d)\n",
                                op, idx, (void*)pose,
                                (int)*(s16*)(ch + 0x54),
                                (int)*(s16*)(ch + 0x56),
                                (int)*(s16*)(ch + 0x58),
                                (int)*(s32*)(ch + 0x5C),
                                (int)*(s32*)(ch + 0x60),
                                (int)*(s32*)(ch + 0x64));
                    }
                }
            }
            continue;
        }
        if (op == 0x21) {
            *(u16*)(obj + 0x3C) = param;
            ip = pkt;
            break;
        }
        if (op == 0x17) {
            ip = pkt;
            break;
        }
        extra = (s32)s_clipExtra[op];
        ip += extra;
        (void)param;
    }
    *(u32*)(obj + 0x10) = (u32)(uintptr_t)ip;
    if (OvlyDiag() && s_clipLogs < 48) {
        fprintf(stderr,
                "[obj-ovly] clip-tick ip=%p op=%02x rx=%d ry=%d rz=%d "
                "d70=%04x t=(%d,%d,%d)\n",
                (void*)ip, ip != NULL ? ip[0] : 0xFF,
                node ? (int)*(s16*)(node + 0x54) : 0,
                node ? (int)*(s16*)(node + 0x56) : 0,
                node ? (int)*(s16*)(node + 0x58) : 0,
                (unsigned)*(u16*)(obj + 0x70),
                node ? (int)*(s32*)(node + 0x5C) : 0,
                node ? (int)*(s32*)(node + 0x60) : 0,
                node ? (int)*(s32*)(node + 0x64) : 0);
        s_clipLogs++;
    }
}

/* Retail E39F0 dispatcher slice. The entry/update phase is sourced from the
 * pinned overlay prefix and each covered instruction is delegated to its
 * separately verified retail handler. An unknown in-range opcode aborts with
 * its address instead of advancing the stream with invented payload rules. */
void func_801E39F0(u8* obj, void* context, s32 limit, s32 ticks, s32 mode)
{
    PcPortFieldClipControl state;
    u32 stream;
    int guard;
    (void)mode;
    if (obj == NULL || *(u32*)(obj + 0x10) == 0) return;
    if (!PcPort_FieldClipPrelude(obj, ticks, &stream)) return;
    state.object = obj;
    state.origin = obj;
    state.pool = (u8*)context;
    state.stream = stream;
    state.limit = limit;
    state.ticks = ticks;
    state.running = 1;
    state.postprocess = 0;
    state.operand = 0;
    for (guard = 0; guard < 0x100 && state.running; ++guard) {
        if (PcPort_FieldClipControlStep(&state)) continue;
        if (PcPort_FieldClipDataStep(&state)) continue;
        fprintf(stderr,
                "[obj-ovly] E39F0 unsupported retail clip opcode "
                "obj=%p ip=%08x opcode=%02x limit=%d ticks=%d mode=%d\n",
                (void*)obj, state.stream,
                *(u8*)(uintptr_t)state.stream, limit, ticks, mode);
        abort();
    }
    if (state.running) {
        fprintf(stderr, "[obj-ovly] E39F0 clip guard exhausted obj=%p ip=%08x\n",
                (void*)obj, state.stream);
        abort();
    }
    *(u32*)(state.object + 0x10) = state.stream;
}

/* Retail 801E35D0: queue a clip if one is already running, else bind
 * table[anim] and start the interpreter. */
static void OvlyClipBind(u8* obj, u8* src, s32 anim) {
    u8 q;
    u8* table;
    u32 clip;
    static s32 s_bindClipLogs;

    if (obj == NULL || src == NULL) {
        return;
    }
    q = obj[0x2B];
    if (q != 0) {
        if (q < 5) {
            obj[0x2B] = (u8)(q + 1);
        }
        q = obj[0x2B];
        obj[0x2A + q] = src[0x20];
        obj[0x2E + q] = (u8)anim;
        return;
    }
    table = (u8*)(uintptr_t)*(u32*)(src + 8);
    if (table == NULL) {
        return;
    }
    if (anim < 0x50) {
        clip = *(u32*)(table + anim * 4);
    } else {
        u8* aux = (u8*)(uintptr_t)*(u32*)(src + 0xC);
        if (aux == NULL) {
            return;
        }
        clip = *(u32*)((u8*)(uintptr_t)*(u32*)(aux + 4) + anim * 4 - 0x138);
    }
    if (clip < 0x10000) {
        if (OvlyDiag() && s_bindClipLogs < 8) {
            fprintf(stderr, "[obj-ovly] clip-bind reject anim=%d clip=%08x table=%p\n",
                    (int)anim, clip, (void*)table);
            s_bindClipLogs++;
        }
        return;
    }
    *(u32*)(obj + 0x10) = clip;
    *(u16*)(obj + 0x42) = 0;
    *(u16*)(obj + 0x40) = 0;
    *(u32*)(obj + 0x50) = 0;
    *(u32*)(obj + 0x54) = 0;
    *(u32*)(obj + 0x4C) = 0;
    obj[0x23] = 0;
    *(u16*)(obj + 0x10A) = (u16)D_801E863C;
    if (OvlyDiag() && s_bindClipLogs < 12) {
        u8* p = (u8*)(uintptr_t)clip;
        fprintf(stderr,
                "[obj-ovly] clip-bind anim=%d clip=%p bytes="
                "%02x%02x %02x%02x %02x%02x %02x%02x %02x%02x %02x%02x "
                "%02x%02x %02x%02x %02x%02x %02x%02x %02x%02x %02x%02x "
                "%02x%02x %02x%02x %02x%02x %02x%02x\n",
                (int)anim, (void*)(uintptr_t)clip,
                p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7],
                p[8], p[9], p[10], p[11], p[12], p[13], p[14], p[15],
                p[16], p[17], p[18], p[19], p[20], p[21], p[22], p[23],
                p[24], p[25], p[26], p[27], p[28], p[29], p[30], p[31]);
        s_bindClipLogs++;
    }
    OvlyClipTick(obj, 1, 1);
}

/* Resolve an existing packed bank reference. RAM aliases are hardware
 * addresses, including address zero; other values are already native
 * pointers. This does not create a bank or establish its lifetime. */
static u8* FieldSoundBankPointer(u32 address)
{
    if (address < 0x200000u || (address & 0xffe00000u) == 0x80000000u
        || (address & 0xffe00000u) == 0xa0000000u)
        return PSX_ADDR(address);
    return (u8*)(uintptr_t)address;
}

/* Retail [801E5CD8,801E5D44), SHA-256
 * e1810ab5a08ecf8609402635dbd2c7a0c00f24ce0b4a11263a6685beff1ca198.
 * Selector is fullword here; only its script caller supplies a byte. */
u32 func_801E5CD8(u8* object, s32 selector)
{
    u32 bank;
    u16 id;
    if (selector == 0) {
        memcpy(&bank, PSX_ADDR(0x8005919c), sizeof(bank));
    } else if (selector == 1 || selector == 2) {
        u32 package;
        memcpy(&package, object + (selector == 1 ? 0xb0 : 0xb4), sizeof(package));
        memcpy(&bank, FieldSoundBankPointer(package) + 8, sizeof(bank));
    } else {
        return 2;
    }
    memcpy(&id, FieldSoundBankPointer(bank) + 0x14, sizeof(id));
    return (u32)id << 16;
}

/* Retail [801E0844,801E0850), SHA-256
 * 4ff83a93b28850a90cc018298227c8bda54d9fb9734eb51f729dec1998e4e0f0.
 * The timed-command caller supplies a second argument; retail ignores it. */
s32 func_801E0844(u8* trail, s32 unused)
{
    (void)unused;
    *(u16*)trail = 0xffff;
    return -1;
}

/* Retail [801E632C,801E6338), SHA-256
 * e72b17d79ecd6e56890a26389eca4dd0d82b0fff7f30845317c7871ce006ff9c. */
s32 func_801E632C(u8* object)
{
    *(u16*)(object + 0x98) = 0xffff;
    return -1;
}

/* Retail 801E6338..801E63A8: distance to the object's signed16 target.
 * SUBU, MULT/MFLO, and ADDU all wrap before SquareRoot0 sees the sum. */
s32 func_801E6338(u8* obj) {
    u8* root = (u8*)(uintptr_t)*(u32*)(obj + 4);
    u32 sum = 0;
    for (u32 axis = 0; axis < 3; ++axis) {
        u32 delta = (u32)(s32)*(s16*)(obj + 0x88 + axis * 2) - *(u32*)(root + 0x5C + axis * 4);
        sum += delta * delta;
    }
    return SquareRoot0((s32)sum);
}

/* Retail 801DF6A8..801DF6F0: reset flags, not entire track records. */
void func_801DF6A8(u8* pool) {
    u8* entries = (u8*)(uintptr_t)*(u32*)pool;
    if (entries != NULL) {
        *(u16*)(pool + 4) = 0;
        for (u32 i = 0; i < *(u16*)(pool + 6); ++i, entries += 20) entries[0] = 0;
    }
}

/* Retail 801DF5F4..801DF668: positive allocation size is full32-bit;
 * capacity is stored as16-bit before allocation, including failure paths. */
u8* func_801DF5F4(u8* pool, s32 count) {
    if (count <= 0) return NULL;
    *(u16*)(pool + 6) = (u16)count;
    HeapChangeCurrentUser(HEAP_USER_MASA, NULL);
    *(u32*)pool = (u32)(uintptr_t)HeapAlloc((u32)count * 20u, 0);
    if (*(u32*)pool == 0) return NULL;
    func_801DF6A8(pool);
    return pool;
}

/* Retail 801DF668..801DF6A8: the free-index clear precedes HeapFree. */
void func_801DF668(u8* pool) {
    void* entries = (void*)(uintptr_t)*(u32*)pool;
    *(u16*)(pool + 4) = 0;
    if (entries != NULL) HeapFree(entries);
    *(u32*)pool = 0;
}

/* Retail 801DF6F0..801DF7A8: return a free record and advance the search
 * cursor, without marking that record occupied. An occupied initial slot
 * returns NULL; retail does not search past it in that branch. */
u8* func_801DF6F0(u8* pool) {
    u32 index = *(u16*)(pool + 4);
    u32 capacity = *(u16*)(pool + 6);
    u8* entry;
    if (index >= capacity) return NULL;
    entry = (u8*)(uintptr_t)*(u32*)pool + index * 20u;
    if (entry[0] != 0) return NULL;
    *(u16*)(pool + 4) = (u16)(index + 1);
    capacity = *(u16*)(pool + 6);
    while (*(u16*)(pool + 4) < capacity) {
        u8* next = (u8*)(uintptr_t)*(u32*)pool + *(u16*)(pool + 4) * 20u;
        if (next[0] == 0) break;
        *(u16*)(pool + 4) = (u16)(*(u16*)(pool + 4) + 1);
    }
    return entry;
}

/* Retail [801E6974,801E6D94), SHA-256
 * dce06cc781ab77e9c6d0fc5818e91b0ceb1e946bff67ad2e279b03d0cb2500e2.
 * General recursive track binding, not an effect constructor. */
void func_801E6974(u8* object, u8* pool, u8* node, s32 flags,
                  s32 modeArg, s32 tag, s32 loop,
                  s32 sx, s32 sy, s32 sz, s32 ex, s32 ey, s32 ez,
                  s32 duration)
{
    u8 mode = (u8)modeArg;
    u32 kind = (u32)flags & 7u;
    u32 slot = kind == 0 ? 0x70 : kind == 1 ? 0x74 : 0x78;
    u32 current = kind == 0 ? 0x54 : kind == 1 ? 0x5c : 0x4c;
    u32 stride = kind == 1 ? 4 : 2;
    u16 starts[3] = {(u16)sx, (u16)sy, (u16)sz};
    u16 ends[3] = {(u16)ex, (u16)ey, (u16)ez};
    u16 startOffsets[3] = {0}, endOffsets[3] = {0};
    u8* track = (u8*)(uintptr_t)*(u32*)(node + slot);
    if (track == NULL) track = func_801DF6F0(pool);
    if (track != NULL) {
        track[0] = 1;
        track[2] = (u8)(mode + 3);
        track[1] = (u8)loop;
        track[3] = (u8)tag;
        if (flags & 0x20)
            for (u32 axis = 0; axis < 3; ++axis)
                startOffsets[axis] = *(u16*)(node + current + axis * stride);
        if (flags & 0x40)
            for (u32 axis = 0; axis < 3; ++axis)
                endOffsets[axis] = *(u16*)(node + current + axis * stride);
        for (u32 axis = 0; axis < 3; ++axis)
            *(u16*)(track + 4 + axis * 2) = (u16)(starts[axis] + startOffsets[axis]);
        for (u32 axis = 0; axis < 3; ++axis) {
            u32 value = ends[axis] + endOffsets[axis];
            if (mode == 0) value -= *(u16*)(track + 4 + axis * 2);
            *(u16*)(track + 0xa + axis * 2) = (u16)value;
        }
        *(u16*)(track + 0x10) = 0;
        *(u16*)(track + 0x12) = (u16)duration;
        if (mode < 2) {
            for (u32 axis = 0; axis < 3; ++axis) {
                if (kind == 1)
                    *(s32*)(node + current + axis * stride) = *(s16*)(track + 4 + axis * 2);
                else
                    *(u16*)(node + current + axis * stride) = *(u16*)(track + 4 + axis * 2);
            }
        }
        *(u32*)(node + slot) = (u32)(uintptr_t)track;
    }
    if (flags & 0x80) {
        u8* root = (u8*)(uintptr_t)*(u32*)(object + 4);
        u8* child = root + 124;
        for (u32 index = 1; index < *(u16*)(root + 0xa); ++index, child += 124) {
            if (*(u32*)child == (u32)(uintptr_t)node)
                func_801E6974(object, pool, child, (u8)flags, mode, (u8)tag, (u8)loop,
                    (s16)sx, (s16)sy, (s16)sz, (s16)ex, (s16)ey, (s16)ez, (s16)duration);
            root = (u8*)(uintptr_t)*(u32*)(object + 4);
        }
    }
}

/* Retail [801E59D4,801E5B50), SHA-256
 * 312c073a2f1d21558e69536b6472742d75a21ccb022cb0f48b69b63729aeba82. */
void func_801E59D4(u8* pool, u8* node, s32 duration, s32 x, s32 y, s32 z)
{
    s32 target[3] = {x, y, z};
    u8* track;
    if (duration < 2) {
        for (u32 axis = 0; axis < 3; ++axis)
            *(s16*)(node + 0x54 + axis * 2) = (s16)target[axis];
        node[5] = 1;
        return;
    }
    if (*(s16*)(node + 0x54) == x && *(s16*)(node + 0x56) == y
        && *(s16*)(node + 0x58) == z) return;
    track = (u8*)(uintptr_t)*(u32*)(node + 0x70);
    if (track == NULL) track = func_801DF6F0(pool);
    if (track == NULL) return;
    track[0] = 1;
    track[2] = 3;
    track[1] = 0;
    track[3] = 0xFE;
    for (u32 axis = 0; axis < 3; ++axis)
        *(u16*)(track + 4 + axis * 2) = *(u16*)(node + 0x54 + axis * 2);
    for (u32 axis = 0; axis < 3; ++axis) {
        s32 delta = ((u32)target[axis] - (u32)*(s16*)(node + 0x54 + axis * 2)) & 0xFFFu;
        if (delta >= 0x800) delta -= 0x1000;
        *(s16*)(track + 0xA + axis * 2) = (s16)delta;
    }
    *(u16*)(track + 0x10) = 0;
    *(u16*)(track + 0x12) = (u16)duration;
    *(u32*)(node + 0x70) = (u32)(uintptr_t)track;
}

/* Retail [801E5B50,801E5C74), SHA-256
 * 2ed8c6c81c3f3ad28f2ad4b20f6e72d4d34e78d5a93b1dab326ebce2a2cbfd21. */
void func_801E5B50(u8* pool, u8* node, s32 mode, s32 first, s32 second,
                  s32 duration, s32 x, s32 y, s32 z)
{
    u8* track = (u8*)(uintptr_t)*(u32*)(node + 0x70);
    u32 dx, dy, dz, distance;
    if (track == NULL) track = func_801DF6F0(pool);
    if (track == NULL) return;
    track[0] = 1;
    track[2] = (u8)((u32)mode + 7u);
    track[1] = 0;
    track[3] = 0xFE;
    dx = (u32)x - *(u32*)(node + 0x5C);
    dy = (u32)y - *(u32*)(node + 0x60);
    dz = (u32)z - *(u32*)(node + 0x64);
    distance = (u32)SquareRoot0((s32)(dx * dx + dy * dy + dz * dz)) + 1u;
    *(u16*)(track + 4) = (u16)distance;
    *(u16*)(track + 6) = (u16)first;
    *(u16*)(track + 8) = (u16)second;
    *(u16*)(track + 0xA) = (u16)x;
    *(u16*)(track + 0xC) = (u16)y;
    *(u16*)(track + 0xE) = (u16)z;
    *(u16*)(track + 0x10) = 0;
    *(u16*)(track + 0x12) = (u16)duration;
    *(u32*)(node + 0x70) = (u32)(uintptr_t)track;
}

/* Track slot release, retail 801DF7A8..801DF7F4. */
s32 func_801DF7A8(u8* pool, u8* entry) {
    u32 index;
    if (entry == NULL) return -1;
    /* Retail SUBU then unsigned reciprocal multiply implements /20. */
    index = ((u32)(uintptr_t)entry - *(u32*)pool) / 20u;
    if (index < *(u16*)(pool + 4)) *(u16*)(pool + 4) = (u16)index;
    entry[0] = 0;
    return (s32)index;
}

/* Retail [801E6578,801E6668), SHA-256
 * c2e23bce6a66f51de4c9e42c8338d43fe4f10a9f219d430ff78d6ac94f50d812. */
void func_801E6578(u8* pool, s32 index, u8* source, u8* target)
{
    u32 offset = (u32)index * 124u;
    u8* node = (u8*)(uintptr_t)((u32)(uintptr_t)source + offset);
    u8* destination = (u8*)(uintptr_t)((u32)(uintptr_t)target + offset);
    u32 count = *(u16*)(source + 0xA);
    u8* track;
    node[7] = 0;
    destination[7] = 1;
    func_801DF7A8(pool, (u8*)(uintptr_t)*(u32*)(node + 0x70));
    track = (u8*)(uintptr_t)*(u32*)(node + 0x74);
    *(u32*)(node + 0x70) = 0;
    func_801DF7A8(pool, track);
    track = (u8*)(uintptr_t)*(u32*)(node + 0x78);
    *(u32*)(node + 0x74) = 0;
    func_801DF7A8(pool, track);
    *(u32*)(node + 0x78) = 0;
    for (u32 child = 1; child < count; ++child) {
        u8* candidate = source + child * 124u;
        if (*(u32*)candidate == (u32)(uintptr_t)node) {
            func_801E6578(pool, *(u16*)(candidate + 0xA), source, target);
        }
    }
}

static s32 FieldTrackDivide(s32 numerator, s32 denominator) {
    /* Preserve retail BREAK 7/6 failure conditions as host exceptions. */
    if (denominator == 0 || (numerator == (-2147483647 - 1) && denominator == -1)) __builtin_trap();
    return numerator / denominator;
}

/* Retail [801E66BC,801E67F8), SHA-256
 * 37f897ed031858abadec3c7fe2bb4759629819f7e3756bc4c1c04ecd24433cbe. */
s32 func_801E66BC(VECTOR* direction, VECTOR* first, VECTOR* second, s32 divisor)
{
    VECTOR cross;
    u32 dot, squared, length;
    s32 scaled;
    OuterProduct12(first, second, &cross);
    dot = (u32)cross.vx * (u32)direction->vx
        + (u32)cross.vy * (u32)direction->vy
        + (u32)cross.vz * (u32)direction->vz;
    squared = (u32)cross.vx * (u32)cross.vx
            + (u32)cross.vy * (u32)cross.vy
            + (u32)cross.vz * (u32)cross.vz;
    length = (u32)SquareRoot0((s32)squared) + 1u;
    scaled = FieldTrackDivide((s32)(dot << 4), (s32)length);
    scaled = FieldTrackDivide((s32)((u32)scaled << 8), divisor);
    return (s16)scaled;
}

/* Retail effect callbacks, archive6B9 [801E0850,801E0A00).
 * rsin is the header-adapted retail 8003F8CC entry, not host sine. */
s32 func_801E0850(s32 phase, s32 divisor, s32 offset) {
    s32 numerator = (s32)((u32)rsin((s16)phase) + 4096u);
    s32 quotient = FieldTrackDivide(numerator, (s16)divisor);
    return (s16)((u32)offset + (u32)quotient);
}

s32 func_801E08D4(s32 phase, s32 divisor, s32 offset) {
    s32 quotient = FieldTrackDivide((s16)phase, (s16)divisor);
    s32 value = (s16)((u32)offset + (u32)quotient);
    return value > 32 ? -1 : value;
}

s32 func_801E0938(s32 phase, s32 divisor, s32 offset) {
    s32 quotient = FieldTrackDivide((s16)phase, (s16)divisor);
    return (s16)((u32)offset - (u32)quotient);
}

s32 func_801E0988(s32 phase, s32 divisor, s32 offset) {
    s32 quotient = FieldTrackDivide((s16)phase, (s16)divisor);
    s32 value = (s16)(32u - (u32)quotient);
    return value < (s16)offset ? (s16)offset : value;
}

/* Keep the packed owner's guest code address; the eventual indirect-call
 * bridge must resolve it explicitly, not truncate a host function pointer.
 * Retail [801E34BC,801E3534) has this explicit selector default. */
u32 func_801E34BC(s32 selector) {
    switch (selector) {
    case 1: return 0x801E08D4;
    case 2: return 0x801E0938;
    case 3: return 0x801E0988;
    default: return 0x801E0850;
    }
}

/* Retail [801DF0B4,801DF52C), SHA-256
 * eef6ffe9066cfd82b44ebd1d4048221527ef6d2ebf18019e30dc3f97141b7e5e. */
u32 func_801DF0B4(u8* pool, u8* root, u8* pose, s32 duration,
                 s32 absolute, s32 loop, s32 tag)
{
    if (duration == 0) duration = 1;
    absolute &= 1;
    loop &= 1;
    u32 limits[2] = {*(u16*)(pose + 0xC), *(u16*)(pose + 0xE)};
    s16 prefix = *(s16*)(pose + 6);
    u16 flags = *(u16*)(pose + 4);
    s16* source = (s16*)(pose + 0x18);
    if (prefix == 0) source += (limits[0] + 1u) * 3u;
    u32 children = (u16)(*(u16*)(root + 0xA) - 1u);
    u16 consumed[2] = {0, 0};
    for (u32 i = 0; i < children; ++i) {
        u8* node = root + (i + 1u) * 0x7Cu;
        for (unsigned channel = 0; channel < 2; ++channel) {
            u32* slot = (u32*)(node + 0x70 + channel * 4);
            s32 target[3];
            int changed = 0;
            if (!(flags & (1u << channel)) && consumed[channel] < limits[channel]) {
                for (unsigned axis = 0; axis < 3; ++axis) target[axis] = *source++;
                ++consumed[channel];
                for (unsigned axis = 0; axis < 3; ++axis) {
                    s32 current = channel ? *(s32*)(node + 0x5C + axis * 4)
                                          : *(s16*)(node + 0x54 + axis * 2);
                    if (current != target[axis]) changed = 1;
                }
            }
            u8* track = (u8*)(uintptr_t)*slot;
            if (changed) {
                if (track != NULL && track[3] == 0xFF) continue;
                if (track == NULL) track = func_801DF6F0(pool);
                if (track != NULL) {
                    track[0] = 1;
                    track[2] = (u8)(absolute + 3);
                    track[1] = (u8)loop;
                    track[3] = (u8)tag;
                    for (unsigned axis = 0; axis < 3; ++axis)
                        *(u16*)(track + 4 + axis * 2) = channel
                            ? *(u16*)(node + 0x5C + axis * 4)
                            : *(u16*)(node + 0x54 + axis * 2);
                    for (unsigned axis = 0; axis < 3; ++axis) {
                        s32 delta;
                        if (channel) {
                            delta = absolute ? target[axis] : (s32)((u32)target[axis] -
                                *(u16*)(node + 0x5C + axis * 4));
                        } else {
                            delta = (s32)(((u32)target[axis] -
                                (u32)(s32)*(s16*)(node + 0x54 + axis * 2)) & 0xFFFu);
                            if (delta >= 0x800) delta -= 0x1000;
                        }
                        *(u16*)(track + 0xA + axis * 2) = (u16)delta;
                    }
                    if (!channel && absolute)
                        for (unsigned axis = 0; axis < 3; ++axis)
                            *(u16*)(track + 0xA + axis * 2) = (u16)(
                                *(u16*)(track + 0xA + axis * 2) +
                                *(u16*)(node + 0x54 + axis * 2));
                    *(u16*)(track + 0x10) = 0;
                    *(u16*)(track + 0x12) = (u16)duration;
                    *slot = (u32)(uintptr_t)track;
                    continue;
                }
            }
            track = (u8*)(uintptr_t)*slot;
            if (track != NULL && track[3] != 0xFF) {
                func_801DF7A8(pool, track);
                *slot = 0;
            }
        }
    }
    return children;
}

/* Retail [801DF52C,801DF5F4), SHA-256
 * a0a446eac8e947dd26933b66a3a750963d7bfb05dcf32e07af3abbbdeafc3fad. */
void func_801DF52C(u8* pool, u8* root, s32 index, s32 mask)
{
    if (index < *(u16*)(root + 0xA)) {
        u8* node = (u8*)(uintptr_t)((u32)(uintptr_t)root + (u32)index * 0x7Cu);
        for (unsigned channel = 0; channel < 3; ++channel) {
            u32* slot = (u32*)(node + 0x70 + channel * 4);
            if (*slot != 0 && ((u32)mask & (1u << channel))) {
                func_801DF7A8(pool, (u8*)(uintptr_t)*slot);
                *slot = 0;
            }
        }
    }
}

/* Retail [801DFE8C,801DFF78), SHA-256
 * c6a3090588554602e9bb56ccb10bfd96dcedb713542115a7eca6cc2adac7075a. */
void func_801DFE8C(u8* pool, u8* root)
{
    u32 count = *(u16*)(root + 0xA);
    for (u32 i = 0; i < count; ++i) {
        u8* node = root + i * 0x7Cu;
        for (unsigned channel = 0; channel < 3; ++channel) {
            u32* slot = (u32*)(node + 0x70 + channel * 4);
            u8* track = (u8*)(uintptr_t)*slot;
            if (track != NULL && track[3] != 0xFF) {
                func_801DF7A8(pool, track);
                *slot = 0;
            }
        }
    }
}

/* Retail [801DF7F4,801DFAC4), SHA-256
 * dc5578a95a99d8a88c5fcef27b75cb918c0bd1727e809aa3754ba08a28db0a3e. */
u32 func_801DF7F4(u8* pool, u8* root, u8* pose, s32 loop, s32 tag)
{
    if (*(u16*)(pose + 6) != 0) {
        func_801DFE8C(pool, root);
        func_801DEF10(root, pose);
        return 1;
    }
    u32 rotations = *(u16*)(pose + 0xC);
    u32 count = *(u16*)(root + 0xA);
    u32 translations = *(u16*)(pose + 0xE);
    loop &= 1;
    if (rotations + 1u < count) count = (u16)(rotations + 1u);
    u32 frames = *(u16*)(pose + 2);
    if (!loop) --frames;
    u16 flags = *(u16*)(pose + 4);
    u8* descriptor = pose + 0x18;
    u32 streamBase = (u32)(uintptr_t)descriptor + (rotations + 1u) * 6u;
    if (!(flags & 1)) streamBase += rotations * 6u;
    if (!(flags & 2)) streamBase += translations * 6u;
    for (u32 i = 0; i < count; ++i, descriptor += 6) {
        u8* node = root + i * 0x7Cu;
        for (unsigned channel = 0; channel < 2; ++channel) {
            u16 offset = *(u16*)(descriptor + channel * 2);
            u32* slot = (u32*)(node + 0x70 + channel * 4);
            u8* track = (u8*)(uintptr_t)*slot;
            if (offset != 0xFFFF) {
                if (track != NULL && track[3] == 0xFF) continue;
                if (track == NULL) track = func_801DF6F0(pool);
                if (track == NULL) continue;
                track[0] = 1;
                track[1] = (u8)loop;
                u8 mode = descriptor[4 + channel];
                track[3] = (u8)tag;
                track[2] = mode;
                offset = *(u16*)(descriptor + channel * 2);
                *(u16*)(track + 0x10) = 0;
                *(u16*)(track + 0x12) = (u16)frames;
                *(u32*)(track + 4) = streamBase + offset;
                *(u32*)(track + 8) = streamBase + offset;
                *slot = (u32)(uintptr_t)track;
            } else if (track != NULL && i != 0 && track[3] != 0xFF) {
                func_801DF7A8(pool, track);
                *slot = 0;
            }
        }
    }
    return 0;
}

/* Retail [801DFAC4,801DFE8C), SHA-256
 * 91826341a3950ea20ff85e1ca4ea296f0d4056dab6df4a6b471daab2a3570f31. */
u32 func_801DFAC4(u8* pool, u8* root, u8* pose, s32 loop, s32 tag)
{
    if (*(u16*)(pose + 6) != 0) {
        func_801DFE8C(pool, root);
        func_801DEF10(root, pose);
        return 1;
    }
    loop &= 1;
    u32 limits[2] = {*(u16*)(pose + 0xC), *(u16*)(pose + 0xE)};
    u32 count = *(u16*)(root + 0xA);
    if (limits[0] + 1u < count) count = (u16)(limits[0] + 1u);
    u16 frames = *(u16*)(pose + 2);
    if (!loop) --frames;
    u16 flags = *(u16*)(pose + 4);
    u8* descriptor = pose + 0x18;
    u16* initialPose = (u16*)(descriptor + (limits[0] + 1u) * 6u);
    u32 streamBase = (u32)(uintptr_t)initialPose;
    if (!(flags & 1)) streamBase += limits[0] * 6u;
    if (!(flags & 2)) streamBase += limits[1] * 6u;
    u16 consumed[2] = {0, 0};
    for (u32 i = 0; i < count; ++i, descriptor += 6) {
        u8* node = root + i * 0x7Cu;
        for (unsigned channel = 0; channel < 2; ++channel) {
            u16 offset = *(u16*)(descriptor + channel * 2);
            u32* slot = (u32*)(node + 0x70 + channel * 4);
            u8* track = NULL;
            int eligible = offset != 0xFFFF;
            if (eligible) {
                track = (u8*)(uintptr_t)*slot;
                if (track != NULL && track[3] == 0xFF) eligible = 0;
                else if (track == NULL) track = func_801DF6F0(pool);
            }
            if (!(flags & (1u << channel)) && i != 0 && consumed[channel] < limits[channel]) {
                ++consumed[channel];
                if (eligible) {
                    if (channel) {
                        *(s32*)(node + 0x5C) = (s16)*initialPose++;
                        *(s32*)(node + 0x60) = (s16)*initialPose++;
                        s32 z = (s16)*initialPose++;
                        node[4] = 1;
                        *(s32*)(node + 0x64) = z;
                    } else {
                        *(u16*)(node + 0x54) = *initialPose++;
                        *(u16*)(node + 0x56) = *initialPose++;
                        u16 z = *initialPose++;
                        node[4] = 1;
                        node[5] = 1;
                        *(u16*)(node + 0x58) = z;
                    }
                } else {
                    initialPose += 3;
                }
            }
            if (!eligible || track == NULL) continue;
            track[0] = 1;
            track[1] = (u8)loop;
            u8 mode = descriptor[4 + channel];
            track[3] = (u8)tag;
            track[2] = mode;
            offset = *(u16*)(descriptor + channel * 2);
            *(u16*)(track + 0x10) = 0;
            *(u16*)(track + 0x12) = frames;
            *(u32*)(track + 4) = streamBase + offset;
            *(u32*)(track + 8) = streamBase + offset;
            *slot = (u32)(uintptr_t)track;
        }
    }
    return 0;
}

/* Rotation stage of DDBF8: retail 801DDC4C..801DE3EC. The complete
 * decoder must still run its translation and scale stages afterward. */
u32 FieldDecodeRotationTrack(u8* node, u8* pool, u32 eventTag, u32 status) {
    u8* track = (u8*)(uintptr_t)*(u32*)(node + 0x70);
    u16* rotation = (u16*)(node + 0x54);
    u32 mode, flags;
    if (track == NULL) return status;
    flags = track[2];
    mode = flags & 15;
    if (mode < 3) {
        for (u32 axis = 0; axis < 3; ++axis) {
            u8* stream;
            if (flags & (0x10u << axis)) continue;
            stream = (u8*)(uintptr_t)*(u32*)(track + 8);
            if (mode == 1) {
                s32 delta = *(s8*)stream;
                *(u32*)(track + 8) += 1;
                if (delta == -128) {
                    u16 absolute = (u16)stream[1] | ((u16)stream[2] << 8);
                    *(u32*)(track + 8) += 2;
                    rotation[axis] = absolute;
                } else {
                    rotation[axis] = (u16)(rotation[axis] + delta);
                }
            } else {
                u16 value = *(u16*)stream;
                rotation[axis] = mode == 0 ? value : (u16)(rotation[axis] + value);
                *(u32*)(track + 8) += 2;
            }
        }
    } else if (mode == 3) {
        s32 next = (s16)(*(u16*)(track + 0x10) + 1);
        for (u32 axis = 0; axis < 3; ++axis) {
            s32 product = (s32)((u32)(s32)*(s16*)(track + 0xA + axis * 2) * (u32)next);
            rotation[axis] = (u16)(*(u16*)(track + 4 + axis * 2) +
                FieldTrackDivide(product, *(s16*)(track + 0x12)));
        }
    } else if (mode == 4) {
        s32 delta[3];
        s32 duration = *(s16*)(track + 0x12);
        for (u32 axis = 0; axis < 3; ++axis)
            delta[axis] = FieldTrackDivide((s32)*(s16*)(track + 0xA + axis * 2) -
                                          (s16)rotation[axis], duration);
        if (((u16)delta[0] | (u16)delta[1] | (u16)delta[2]) == 0) {
            *(u16*)(track + 0x10) = (u16)duration;
            for (u32 axis = 0; axis < 3; ++axis) rotation[axis] = *(u16*)(track + 0xA + axis * 2);
        } else {
            for (u32 axis = 0; axis < 3; ++axis) rotation[axis] = (u16)(rotation[axis] + delta[axis]);
            *(u16*)(track + 0x10) = 0;
        }
    } else if (mode == 5) {
        for (u32 axis = 0; axis < 3; ++axis) {
            u16 velocity = (u16)(*(u16*)(track + 4 + axis * 2) + *(u16*)(track + 0xA + axis * 2));
            *(u16*)(track + 4 + axis * 2) = velocity;
            rotation[axis] = (u16)(rotation[axis] + velocity);
        }
    } else if (mode == 7 || mode == 8) {
        u32 delta[3], squares[3];
        for (u32 axis = 0; axis < 3; ++axis) {
            delta[axis] = (u32)(s32)*(s16*)(track + 0xA + axis * 2) - *(u32*)(node + 0x5C + axis * 4);
            squares[axis] = delta[axis] * delta[axis];
        }
        u32 distance = (u32)SquareRoot0((s32)(squares[0] + squares[1] + squares[2])) + 1;
        s32 angle = ratan2((s32)(0u - delta[0]), (s32)(0u - delta[2]));
        s32 difference = (s32)(((u32)angle - (u32)(s32)(s16)rotation[1]) & 0xFFF);
        if (difference >= 0x800) difference -= 0x1000;
        s32 product = (s32)((distance + (u32)(s32)*(s16*)(track + 0x10)) * (u32)(s32)*(s16*)(track + 8));
        s32 limit = (s32)((u32)(s32)*(s16*)(track + 6) + (u32)FieldTrackDivide(product, *(s16*)(track + 4)));
        s32 magnitude = difference < 0 ? -difference : difference;
        u32 step = magnitude < limit ? (u32)difference : difference < 0 ? 0u - (u32)limit : (u32)limit;
        rotation[1] = (u16)(rotation[1] + step);
        if (mode == 7) {
            s32 horizontal = SquareRoot0((s32)(squares[0] + squares[2]));
            angle = ratan2((s32)delta[1], horizontal);
            difference = (s32)(((u32)angle - (u32)(s32)(s16)rotation[0]) & 0xFFF);
            if (difference >= 0x800) difference -= 0x1000;
            magnitude = difference < 0 ? -difference : difference;
            step = magnitude < limit ? (u32)difference : difference < 0 ? 0u - (u32)limit : (u32)limit;
            rotation[0] = (u16)(rotation[0] + step);
        }
        if (*(s16*)(track + 0x10) < 32000)
            *(u16*)(track + 0x10) = (u16)(*(u16*)(track + 0x10) + *(u16*)(track + 0x12));
        node[5] = node[4] = 1;
        return status;
    }
    /* Retail modes 6 and 9..15 do no component writes but still take this
     * lifecycle path. They are not aliases for an invented decoder mode. */
    *(u16*)(track + 0x10) = (u16)(*(u16*)(track + 0x10) + 1);
    if (*(s16*)(track + 0x10) >= *(s16*)(track + 0x12)) {
        if (track[1] == 0) {
            if (track[3] == eventTag) status |= 2;
            status |= 0x200;
            func_801DF7A8(pool, track);
            *(u32*)(node + 0x70) = 0;
        } else {
            if (track[3] == eventTag) status |= 4;
            status |= 0x400;
            if (mode < 3) {
                *(u16*)(track + 0x10) = 0;
                *(u32*)(track + 8) = *(u32*)(track + 4);
            } else {
                *(u16*)(track + 0x10) = 0xFFFF;
                if (mode == 5) for (u32 axis = 0; axis < 3; ++axis) *(u16*)(track + 0xA + axis * 2) = 0;
            }
        }
    } else {
        if (track[3] == eventTag) status |= 1;
        status |= 0x100;
    }
    node[5] = node[4] = 1;
    return status;
}

/* Translation stage of DDBF8: retail 801DE3EC..801DEAD0. */
u32 FieldDecodeTranslationTrack(u8* node, u8* pool, u32 eventTag, u32 status, s32 scale) {
    u8* track = (u8*)(uintptr_t)*(u32*)(node + 0x74);
    u32* position = (u32*)(node + 0x5C);
    u32 mode, flags;
    if (track == NULL) return status;
    flags = track[2];
    mode = flags & 15;
    if (mode < 2) {
        for (u32 axis = 0; axis < 3; ++axis) {
            u8* stream;
            if (flags & (0x10u << axis)) continue;
            stream = (u8*)(uintptr_t)*(u32*)(track + 8);
            if (mode == 1) {
                s32 delta = *(s8*)stream;
                *(u32*)(track + 8) += 1;
                if (delta == -128) {
                    u16 absolute = (u16)stream[1] | ((u16)stream[2] << 8);
                    *(u32*)(track + 8) += 2;
                    position[axis] = (u32)(s32)(s16)absolute;
                } else {
                    position[axis] += (u32)delta;
                }
            } else {
                position[axis] = (u32)(s32)*(s16*)stream;
                *(u32*)(track + 8) += 2;
            }
        }
    } else if (mode == 2) {
        SVECTOR delta;
        VECTOR transformed;
        s16* component = &delta.vx;
        for (u32 axis = 0; axis < 3; ++axis) {
            if (flags & (0x10u << axis)) component[axis] = 0;
            else {
                u8* stream = (u8*)(uintptr_t)*(u32*)(track + 8);
                component[axis] = *(s16*)stream;
                *(u32*)(track + 8) += 2;
            }
        }
        for (u32 axis = 0; axis < 3; ++axis) {
            u32 product = (u32)(s32)component[axis] * (u32)(s32)*(s16*)(node + 0x4C + axis * 2);
            component[axis] = (s16)((s32)product >> 12);
        }
        ApplyMatrix((MATRIX*)(node + 0x2C), &delta, &transformed);
        for (u32 axis = 0; axis < 3; ++axis) {
            u32 product = (u32)scale * (u32)(&transformed.vx)[axis];
            position[axis] += (u32)((s32)product >> 12);
        }
    } else if (mode == 3) {
        s32 next = (s16)(*(u16*)(track + 0x10) + 1);
        for (u32 axis = 0; axis < 3; ++axis) {
            s32 product = (s32)((u32)(s32)*(s16*)(track + 0xA + axis * 2) * (u32)next);
            position[axis] = (u32)(s32)*(s16*)(track + 4 + axis * 2) +
                (u32)FieldTrackDivide(product, *(s16*)(track + 0x12));
        }
    } else if (mode == 4) {
        s32 delta[3];
        s32 duration = *(s16*)(track + 0x12);
        for (u32 axis = 0; axis < 3; ++axis)
            delta[axis] = FieldTrackDivide((s32)((u32)(s32)*(s16*)(track + 0xA + axis * 2) - position[axis]), duration);
        if (((u16)delta[0] | (u16)delta[1] | (u16)delta[2]) == 0) {
            *(u16*)(track + 0x10) = (u16)duration;
            for (u32 axis = 0; axis < 3; ++axis) position[axis] = (u32)(s32)*(s16*)(track + 0xA + axis * 2);
        } else {
            for (u32 axis = 0; axis < 3; ++axis) position[axis] += (u32)(s32)(s16)delta[axis];
            *(u16*)(track + 0x10) = 0;
        }
    } else if (mode == 5) {
        for (u32 axis = 0; axis < 3; ++axis) {
            u16 velocity = (u16)(*(u16*)(track + 4 + axis * 2) + *(u16*)(track + 0xA + axis * 2));
            *(u16*)(track + 4 + axis * 2) = velocity;
            position[axis] += (u32)(s32)(s16)velocity;
        }
    }
    *(u16*)(track + 0x10) = (u16)(*(u16*)(track + 0x10) + 1);
    if (*(s16*)(track + 0x10) >= *(s16*)(track + 0x12)) {
        if (track[1] == 0) {
            if (track[3] == eventTag) status |= 2;
            status |= 0x200;
            func_801DF7A8(pool, track);
            *(u32*)(node + 0x74) = 0;
        } else {
            if (track[3] == eventTag) status |= 4;
            status |= 0x400;
            if (mode < 3) {
                *(u16*)(track + 0x10) = 0;
                *(u32*)(track + 8) = *(u32*)(track + 4);
            } else {
                *(u16*)(track + 0x10) = 0xFFFF;
                if (mode == 5) for (u32 axis = 0; axis < 3; ++axis) *(u16*)(track + 0xA + axis * 2) = 0;
            }
        }
    } else {
        if (track[3] == eventTag) status |= 1;
        status |= 0x100;
    }
    node[4] = 1;
    return status;
}

/* Scale stage of DDBF8: retail 801DEAD0..801DEEB0. */
u32 FieldDecodeScaleTrack(u8* node, u8* pool, u32 eventTag, u32 status) {
    u8* track = (u8*)(uintptr_t)*(u32*)(node + 0x78);
    u16* scaling = (u16*)(node + 0x4C);
    u32 mode, flags;
    if (track == NULL) return status;
    flags = track[2];
    mode = flags & 15;
    if (mode == 3) {
        s32 next = (s16)(*(u16*)(track + 0x10) + 1);
        for (u32 axis = 0; axis < 3; ++axis) {
            s32 product = (s32)((u32)(s32)*(s16*)(track + 0xA + axis * 2) * (u32)next);
            scaling[axis] = (u16)(*(u16*)(track + 4 + axis * 2) +
                FieldTrackDivide(product, *(s16*)(track + 0x12)));
        }
    } else if (mode == 4) {
        s32 delta[3];
        s32 duration = *(s16*)(track + 0x12);
        for (u32 axis = 0; axis < 3; ++axis)
            delta[axis] = FieldTrackDivide((s32)*(s16*)(track + 0xA + axis * 2) -
                                          *(s16*)(track + 4 + axis * 2), duration);
        if (((u16)delta[0] | (u16)delta[1] | (u16)delta[2]) == 0) {
            *(u16*)(track + 0x10) = (u16)duration;
            for (u32 axis = 0; axis < 3; ++axis) scaling[axis] = *(u16*)(track + 0xA + axis * 2);
        } else {
            for (u32 axis = 0; axis < 3; ++axis) {
                u16 value = (u16)(*(u16*)(track + 4 + axis * 2) + delta[axis]);
                *(u16*)(track + 4 + axis * 2) = value;
                scaling[axis] = value;
            }
            *(u16*)(track + 0x10) = 0;
        }
    } else if (mode == 5) {
        for (u32 axis = 0; axis < 3; ++axis) {
            u16 velocity = (u16)(*(u16*)(track + 4 + axis * 2) + *(u16*)(track + 0xA + axis * 2));
            *(u16*)(track + 4 + axis * 2) = velocity;
            scaling[axis] = (u16)(scaling[axis] + velocity);
        }
    }
    /* Only 3/4/5 write components; all other modes retain lifecycle behavior. */
    *(u16*)(track + 0x10) = (u16)(*(u16*)(track + 0x10) + 1);
    if (*(s16*)(track + 0x10) >= *(s16*)(track + 0x12)) {
        if (track[1] == 0) {
            if (track[3] == eventTag) status |= 2;
            status |= 0x200;
            func_801DF7A8(pool, track);
            *(u32*)(node + 0x78) = 0;
        } else {
            if (track[3] == eventTag) status |= 4;
            status |= 0x400;
            *(u16*)(track + 0x10) = 0xFFFF;
            if (mode == 5) for (u32 axis = 0; axis < 3; ++axis) *(u16*)(track + 0xA + axis * 2) = 0;
        }
    } else {
        if (track[3] == eventTag) status |= 1;
        status |= 0x100;
    }
    node[5] = node[4] = 1;
    return status;
}

/* Archive 6B9, retail 801DDBF8..801DEF10: all three track stages, in
 * retail order for each node. The count is sampled once before the loop. */
u32 func_801DDBF8(u8* pool, u8* node, u32 eventTag, s32 scale) {
    u32 count = *(u16*)(node + 0x0A);
    u32 status = 0;
    for (u32 i = 0; i < count; ++i, node += OVLY_NODE_STRIDE) {
        status = FieldDecodeRotationTrack(node, pool, eventTag, status);
        status = FieldDecodeTranslationTrack(node, pool, eventTag, status, scale);
        status = FieldDecodeScaleTrack(node, pool, eventTag, status);
    }
    return status;
}

/* Archive 6B9, retail 801DC5C0..801DC848. Matrix SDK calls retain their
 * hardware-adapter boundary; node layout and pass ordering follow retail. */
s32 func_801DC5C0(u8* root, s32 scale) {
    u32 count = *(u16*)(root + 0x0A);
    MATRIX* diagonal = (MATRIX*)g_PsxScratchpad;
    u32 i;
    memcpy(root + 0x40, root + 0x5C, 12);
    if (root[6] != 0) RotMatrixYXZ((SVECTOR*)(root + 0x54), (MATRIX*)(root + 0x2C));
    else RotMatrix((SVECTOR*)(root + 0x54), (MATRIX*)(root + 0x2C));
    /* MULT/MFLO truncates to 32 bits before SRA and SH. Do not widen the
     * multiply and shift before truncation, even for extreme scale inputs. */
    for (i = 0; i < 3; ++i) {
        u32 product = (u32)scale * (u32)(s32)*(s16*)(root + 0x4C + i * 2);
        diagonal->m[i][0] = diagonal->m[i][1] = diagonal->m[i][2] = 0;
        diagonal->m[i][i] = (s16)((s32)product >> 12);
    }
    MulMatrix0((MATRIX*)(root + 0x2C), diagonal, (MATRIX*)(root + 0x0C));
    memcpy(root + 0x20, root + 0x40, 12);
    for (i = 1; i < count; ++i) {
        u8* node = root + i * OVLY_NODE_STRIDE;
        u8* parent;
        if (node[5] != 0) {
            if (node[6] != 0) RotMatrixYXZ((SVECTOR*)(node + 0x54), (MATRIX*)(node + 0x0C));
            else RotMatrix((SVECTOR*)(node + 0x54), (MATRIX*)(node + 0x0C));
            node[5] = 0;
        }
        parent = (u8*)(uintptr_t)*(u32*)node;
        if (parent != NULL && parent[4] == 1) node[4] = 1;
        if (node[4] != 0) {
            memcpy(node + 0x20, node + 0x5C, 12);
            parent = (u8*)(uintptr_t)*(u32*)node;
            if (parent != NULL) {
                CompMatrix((MATRIX*)(parent + 0x2C), (MATRIX*)(node + 0x0C), (MATRIX*)(node + 0x2C));
            } else {
                memcpy(node + 0x2C, node + 0x0C, sizeof(MATRIX));
            }
        }
    }
    for (i = 1; i < count; ++i) root[i * OVLY_NODE_STRIDE + 4] = 0;
    return (s32)count;
}

/* Archive 6B9, retail 801DC848..801DCC34: scale-compensated hierarchy. */
s32 func_801DC848(u8* root, s32 scale) {
    u32 count = *(u16*)(root + 0x0A);
    MATRIX* diagonal = (MATRIX*)g_PsxScratchpad;
    u32 i;
    memcpy(root + 0x40, root + 0x5C, 12);
    if (root[6] != 0) RotMatrixYXZ((SVECTOR*)(root + 0x54), (MATRIX*)(root + 0x2C));
    else RotMatrix((SVECTOR*)(root + 0x54), (MATRIX*)(root + 0x2C));
    /* MULT/MFLO truncates to 32 bits before SRA and SH. Do not widen the
     * multiply and shift before truncation, even for extreme scale inputs. */
    for (i = 0; i < 3; ++i) {
        u32 product = (u32)scale * (u32)(s32)*(s16*)(root + 0x4C + i * 2);
        diagonal->m[i][0] = diagonal->m[i][1] = diagonal->m[i][2] = 0;
        diagonal->m[i][i] = (s16)((s32)product >> 12);
    }
    MulMatrix0((MATRIX*)(root + 0x2C), diagonal, (MATRIX*)(root + 0x0C));
    memcpy(root + 0x20, root + 0x40, 12);
    for (i = 1; i < count; ++i) {
        u8* node = root + i * OVLY_NODE_STRIDE;
        u8* parent;
        parent = (u8*)(uintptr_t)*(u32*)node;
        if (parent != NULL && parent[5] == 1) node[5] = 1;
        parent = (u8*)(uintptr_t)*(u32*)node;
        if (parent != NULL && parent[4] == 1) node[4] = 1;
        if (node[5] != 0) {
            if (node[6] != 0) RotMatrixYXZ((SVECTOR*)(node + 0x54), (MATRIX*)(node + 0x0C));
            else RotMatrix((SVECTOR*)(node + 0x54), (MATRIX*)(node + 0x0C));
            for (u32 axis = 0; axis < 3; ++axis) {
                diagonal->m[axis][0] = diagonal->m[axis][1] = diagonal->m[axis][2] = 0;
                diagonal->m[axis][axis] = *(s16*)(node + 0x4C + axis * 2);
            }
            MulMatrix0((MATRIX*)(node + 0x0C), diagonal, (MATRIX*)(node + 0x0C));
            parent = (u8*)(uintptr_t)*(u32*)node;
            if (parent != NULL) {
                for (u32 axis = 0; axis < 3; ++axis) {
                    s32 divisor;
                    parent = (u8*)(uintptr_t)*(u32*)node;
                    divisor = *(s16*)(parent + 0x4C + axis * 2);
                    /* Retail DIV is followed by BREAK 7 for zero. The host
                     * trap is an explicit exception adapter, not a fallback. */
                    if (divisor == 0) __builtin_trap();
                    diagonal->m[axis][0] = diagonal->m[axis][1] = diagonal->m[axis][2] = 0;
                    diagonal->m[axis][axis] = (s16)(0x1000000 / divisor);
                }
                MulMatrix0(diagonal, (MATRIX*)(node + 0x0C), (MATRIX*)(node + 0x0C));
            }
        }
        if (node[4] != 0) {
            memcpy(node + 0x20, node + 0x5C, 12);
            parent = (u8*)(uintptr_t)*(u32*)node;
            if (parent != NULL) {
                CompMatrix((MATRIX*)(parent + 0x2C), (MATRIX*)(node + 0x0C), (MATRIX*)(node + 0x2C));
            } else {
                memcpy(node + 0x2C, node + 0x0C, sizeof(MATRIX));
            }
        }
    }
    for (i = 1; i < count; ++i) {
        root[i * OVLY_NODE_STRIDE + 5] = 0;
        root[i * OVLY_NODE_STRIDE + 4] = 0;
    }
    return (s32)count;
}

/* Archive 6B9, retail 801E7298..801E72CC: root-height preparation. */
void func_801E7298(u8* obj) {
    s16 height = *(s16*)(obj + 0x60);
    if (obj[0x36] == 0) {
        u8* root = (u8*)(uintptr_t)*(u32*)(obj + 4);
        *(s32*)(root + 0x60) = height;
    }
}

/* Retail archive6B9, 801E37D0-801E39EC. Linked-object pass; the parent
 * selector is validated by the outer caller (FF means unlinked). */
void func_801E37D0(u8* obj) {
    u8* parent = (u8*)(uintptr_t)D_801E8670[obj[0x5C]];
    MATRIX* transform = (MATRIX*)g_PsxScratchpad;
    u8* parentRoot;
    u8* root;
    s32 joint;
    SVECTOR offset;
    if (parent == NULL) {
        obj[0x5C] = 0xFF;
        return;
    }
    if ((*(u16*)(obj + 0x4A) & 0x10) == 0) obj[0x34] = parent[0x34];
    if (obj[0x34] == 0) return;
    if (obj[0x5D] != 0) {
        joint = *(s16*)(obj + 0x5E);
        parentRoot = (u8*)(uintptr_t)*(u32*)(parent + 4);
        if (joint != 0) {
            MulMatrix0((MATRIX*)(parentRoot + 0x2C),
                       (MATRIX*)(parentRoot + joint * OVLY_NODE_STRIDE + 0x2C),
                       (MATRIX*)g_PsxScratchpad);
        } else {
            transform = (MATRIX*)(parentRoot + 0x2C);
        }
        root = (u8*)(uintptr_t)*(u32*)(obj + 4);
        MulMatrix2(transform, (MATRIX*)(root + 0x0C));
        root = (u8*)(uintptr_t)*(u32*)(obj + 4);
        MulMatrix2(transform, (MATRIX*)(root + 0x2C));
    }
    joint = *(s16*)(obj + 0x5E);
    parent = (u8*)(uintptr_t)D_801E8670[obj[0x5C]];
    parentRoot = (u8*)(uintptr_t)*(u32*)(parent + 4);
    if (joint != 0) {
        /* Destination is the current transform pointer, matching retail
         * s1. It remains scratch for this nonzero-joint path. */
        CompMatrix((MATRIX*)(parentRoot + 0x0C),
                   (MATRIX*)(parentRoot + joint * OVLY_NODE_STRIDE + 0x2C), transform);
    } else {
        transform = (MATRIX*)(parentRoot + 0x0C);
    }
    SetRotMatrix(transform);
    SetTransMatrix(transform);
    offset.vx = *(s16*)(obj + 0x6A);
    offset.vy = *(s16*)(obj + 0x6C);
    offset.vz = *(s16*)(obj + 0x6E);
    gte_ldv0(&offset);
    gte_rt();
    root = (u8*)(uintptr_t)*(u32*)(obj + 4);
    gte_stlvnl(root + 0x20);
    root = (u8*)(uintptr_t)*(u32*)(obj + 4);
    gte_stlvnl(root + 0x5C);
}

/* Retail [801E5D44,801E632C), archive 6B9: per-tick object event step.
 * With an event list (obj+0x98 >= 0) the entries at obj+0xA0 (frame s16,
 * opcode u8, arguments) are consumed while obj+0x9C < obj+0x9E and the
 * entry frame equals the object's frame counter; opcodes 1..9 dispatch
 * through the 801E5DE4 jump table into 801E0844/0A00/165C/34BC/8330/8394,
 * an unknown opcode just advances obj+0x9C. The frame counter then
 * advances and, once it reaches obj+0x9A, the list is rewound from
 * obj+0xA4 with both counters cleared. The prologue, list walk, counter
 * advance and rewind are transcribed; the nine opcode bodies have no native
 * owners yet, so an event that reaches the dispatch stops the walk loudly
 * rather than inventing record sizes. MAP16's objects carry obj+0x98 = -1. */
void func_801E5D44(u8* obj, void* ctx, s32 renderContextIndex)
{
    u16 frame;
    (void)ctx;
    (void)renderContextIndex;
    if (*(s16*)(obj + 0x98) < 0) {
        return;
    }
    while (*(s16*)(obj + 0x9C) < *(s16*)(obj + 0x9E)) {
        u8* e = (u8*)(uintptr_t)*(u32*)(obj + 0xA0);
        if (*(s16*)e != *(s16*)(obj + 0x98)) {
            break;
        }
        if ((u32)e[2] - 1u < 9u) {
            static int s_eventLogs;
            if (s_eventLogs < 8) {
                fprintf(stderr,
                        "[obj-ovly] E5D44 unported event op=%u obj=%p frame=%d\n",
                        (unsigned)e[2], (void*)obj, (int)*(s16*)(obj + 0x98));
                s_eventLogs++;
            }
            return;
        }
        *(u16*)(obj + 0x9C) = (u16)(*(u16*)(obj + 0x9C) + 1);
    }
    frame = (u16)(*(u16*)(obj + 0x98) + 1);
    *(u16*)(obj + 0x98) = frame;
    if (*(s16*)(obj + 0x9A) < 0) {
        return;
    }
    if ((s16)frame < *(s16*)(obj + 0x9A)) {
        return;
    }
    *(u32*)(obj + 0xA0) = *(u32*)(obj + 0xA4);
    *(u16*)(obj + 0x98) = 0;
    *(u16*)(obj + 0x9C) = 0;
}

/* Retail [801E36BC,801E37D0), archive 6B9, SHA-256
 * 9277585ee654c29ae0768e3551fcab8ef33c94b168c7eedd832f38cbffc71fe3:
 * per-frame object update. An object without a model header returns at
 * once. A visible object first prepares the root height (func_801E7298),
 * rebuilds its hierarchy with the update selected by obj+0x37
 * (func_801DC848 scale-compensated, else func_801DC5C0) at scale obj+0x1C,
 * then for every positive tick decodes the animation tracks
 * (func_801DDBF8, statuses OR'd) and steps the event list (func_801E5D44).
 * The clip VM (func_801E39F0) runs afterwards whether or not the object is
 * visible or the tick count is positive, receiving the OR'd track status,
 * the tick count and the caller's fifth argument. Returns the OR'd status. */
s32 func_801E36BC(u8* obj, void* ctx, s32 ticks, s32 renderContextIndex, s32 arg5)
{
    s32 status = 0;
    s32 i;

    if (*(u32*)obj == 0) {
        return status;
    }
    if (obj[0x34] != 0) {
        func_801E7298(obj);
        if (obj[0x37] != 0) {
            func_801DC848((u8*)(uintptr_t)*(u32*)(obj + 4), *(s16*)(obj + 0x1C));
        } else {
            func_801DC5C0((u8*)(uintptr_t)*(u32*)(obj + 4), *(s16*)(obj + 0x1C));
        }
        for (i = 0; i < ticks; ++i) {
            status |= (s32)func_801DDBF8((u8*)ctx, (u8*)(uintptr_t)*(u32*)(obj + 4),
                                         *(u16*)(obj + 0x3C), *(s16*)(obj + 0x1C));
            func_801E5D44(obj, ctx, renderContextIndex);
        }
    }
    func_801E39F0(obj, ctx, status, ticks, arg5);
    return status;
}

static void OvlyDumpVerts(u8* model, unsigned idx) {
    s16* v;
    u8* cmds;
    if (!OvlyDiag() || model == NULL) {
        return;
    }
    v = (s16*)(uintptr_t)*(u32*)(model + 8);
    cmds = (u8*)(uintptr_t)*(u32*)(model + 0x10);
    if (v == NULL) {
        return;
    }
    fprintf(stderr,
            "[obj-ovly] verts idx=%u v0=(%d,%d,%d) v1=(%d,%d,%d) v2=(%d,%d,%d) "
            "cmd0=%02x nprim=%d\n",
            idx, (int)v[0], (int)v[1], (int)v[2],
            (int)v[4], (int)v[5], (int)v[6],
            (int)v[8], (int)v[9], (int)v[10],
            cmds != NULL ? cmds[0] : 0xFF,
            cmds != NULL ? (int)*(s16*)(cmds + 2) : -1);
}

static int OvlySanityCount(s32 n) {
    return n > 0 && n < 0x400;
}

static int OvlySanitySize(s32 n) {
    return n > 0 && n < 0x200000;
}

/* After C8CC, untextured GPU codes (0x20/0x30) or a zero tpage make PsyX
 * shade vertex color only. Force the textured bit when missing, but keep
 * C8CC's merged tpage/CLUT: CC74/CD24 already OR the file palette nibble
 * onto the dest (e.g. 3f05 = x=80,y=252). Overwriting with GetClut(0,252)
 * drops that nibble and samples empty VRAM in the FB gap. */
static void OvlyStampTpageClut(u8* work, s32 payload, u16 tpage, u16 clut) {
    u8* p;
    u8* end;
    s32 n = 0;
    s32 nTex = 0;
    s32 nForced = 0;

    if (work == NULL || payload < 0x10) {
        return;
    }
    p = work;
    end = work + payload;
    while (p + 8 < end) {
        u8 len = p[3];
        u8 code;
        u16 pktClut;
        u16 pktTpage;
        s32 step;
        if (len < 4 || len > 12) {
            break;
        }
        step = ((s32)len + 1) * 4;
        if (p + step > end) {
            break;
        }
        code = p[7];
        pktClut = *(u16*)(p + 0x0E);
        pktTpage = *(u16*)(p + 0x16);
        if (OvlyDiag() && n < 6) {
            fprintf(stderr,
                    "[obj-ovly] pkt%d len=%u code=%02x rgb=%02x%02x%02x "
                    "clut=%04x tpage=%04x uv0=%02x%02x uv1=%02x%02x uv2=%02x%02x\n",
                    (int)n, (unsigned)len, code, p[4], p[5], p[6],
                    (unsigned)pktClut, (unsigned)pktTpage,
                    p[0x0C], p[0x0D], p[0x14], p[0x15], p[0x1C], p[0x1D]);
        }
        if (len == 7 || len == 9) {
            u8 textured = (u8)((code & 0x04) != 0);
            if (!textured) {
                /* Keep gouraud/semi bits; set textured + poly. */
                p[7] = (u8)((code & 0x13) | ((len == 9) ? 0x2C : 0x24));
                nForced++;
            }
            /* Native renderer adapter: neutral RGB 808080 object packets need
             * raw texture sampling at this boundary. This SetShadeTex rewrite
             * is not instruction-exact field code and remains audit-visible. */
            if (p[4] == 0x80 && p[5] == 0x80 && p[6] == 0x80) {
                p[7] = (u8)(p[7] | 0x01);
            }
            if (pktTpage == 0) {
                *(u16*)(p + 0x16) = tpage;
            }
            if (pktClut == 0) {
                *(u16*)(p + 0x0E) = clut;
            }
            nTex++;
        }
        p += step;
        n++;
        if (n > 0x800) {
            break;
        }
    }
    if (OvlyDiag()) {
        fprintf(stderr,
                "[obj-ovly] stamp n=%d tex=%d forced=%d latch tpage=%04x clut=%04x\n",
                (int)n, (int)nTex, (int)nForced,
                (unsigned)tpage, (unsigned)clut);
    }
}

static void OvlyResolve(void* p) {
    u32* file = (u32*)p;
    if (file == NULL) {
        return;
    }
    if (!OvlySanityCount((s32)file[0])) {
        if (OvlyDiag()) {
            fprintf(stderr, "[obj-ovly] skip Resolve count=%d p=%p\n", (int)file[0], p);
        }
        return;
    }
    ResolveArchiveEntryPointers(file);
}

/* Retail 801DC22C..801DC2D0, SHA-256
 * ed37d38e4361912726b17fd6aeb8902f96e13650ba9caf105aa5347b7aa4e971.
 * The registry entry is two PSX words, not a host-pointer-sized structure.
 * Relocation and allocation remain their existing game/heap boundaries. */
u8* func_801DC22C(u8* model, u8* out)
{
    HeapChangeCurrentUser(4, NULL);
    u32 count = (u32)func_8002C3E8(model);
    u32 pointers = (u32)(uintptr_t)HeapAlloc(count * 4u, 0);
    memcpy(out, &pointers, sizeof(pointers));
    memcpy(out + 4, &count, sizeof(count));
    if (pointers != 0) {
        for (u32 i = 0, offset = 0x10; i < count; ++i, offset += 0x38) {
            u32 base;
            memcpy(&base, out, sizeof(base));
            u32 group = (u32)(uintptr_t)model + offset;
            memcpy((void*)(uintptr_t)(base + i * 4u), &group, sizeof(group));
        }
    }
    return out;
}

/* Transparent host-pointer-width adapter for the packed retail registry entry.
 * Object ownership is still slot-indexed here; converting registry selection
 * and all its consumers remains separate from this constructor translation. */
static OvlyPtrTab* OvlyBuildPtrTab(u8* pModel, OvlyPtrTab* out) {
    u32 packed[2];
    func_801DC22C(pModel, (u8*)packed);
    out->ptrs = (u32*)(uintptr_t)packed[0];
    out->count = (s32)packed[1];
    return out;
}

void func_801DCD8C(u8* root);

/* Retail 801DCC3C..DCD8C, SHA-256
 * 093893dc891eb7d18aaadd3b381a442ca54da0c363d5d69ea041a83567b0d508.
 * DCC34 is a separate return-only entry, not this function's prologue. */
void func_801DCC3C(u8* table, u8* root, MATRIX* view, MATRIX* light,
                  s32 drawArg, s32 orderingTable, s32 buffer)
{
    MATRIX* local = (MATRIX*)g_PsxScratchpad;
    MATRIX* rootLight = (MATRIX*)(g_PsxScratchpad + 0x20);
    MATRIX* rootView = (MATRIX*)(g_PsxScratchpad + 0x40);
    MulMatrix0(light, (MATRIX*)(root + 0x2C), rootLight);
    CompMatrix(view, (MATRIX*)(root + 0x0C), rootView);
    u16 count = *(u16*)(root + 0x0A);
    u8* node = root + 0x7C;
    for (u32 i = 1; i < count; ++i, node += 0x7C) {
        if (*(u16*)(node + 8) == 0xFFFF) continue;
        MulMatrix0(rootLight, (MATRIX*)(node + 0x2C), local);
        SetLightMatrix(local);
        CompMatrix(rootView, (MATRIX*)(node + 0x2C), local);
        SetRotMatrix(local);
        SetTransMatrix(local);
        u32* pointers = (u32*)(uintptr_t)*(u32*)table;
        u32 packet = *(u32*)(uintptr_t)((u32)(uintptr_t)node + 0x68u + (u32)buffer * 4u);
        func_8002C700((void*)(uintptr_t)pointers[*(u16*)(node + 8)],
                     (void*)(uintptr_t)packet, (void*)(uintptr_t)(u32)orderingTable, drawArg);
    }
}


u8* func_801E0248(u8* pool, s32 semiTrans);
s32 func_801E0354(u8* pool, u8* entry);
s32 func_801E1258(u8* effect, s32 ticks);
void func_801E22F8(u8* owner, s16* offset, MATRIX* matrix, u32* ot,
                  s32 buffer, s32 scale, s32 height);
extern s32 D_80050100;

/* Retail [801DCEC8,801DDBF8), archive 6B9: draw one registered object.
 * Scratchpad use follows retail: +0x00 the per-node/temp matrix, +0x20 the
 * ground-quad local matrix and then the root light matrix, +0x40 the root
 * view matrix. Passes:
 *   1. rootView = view x root.local (root+0x0C);
 *   2. unless obj+0x4A bit 0: the ground quad. The object's forward vector
 *      (root.local x node1.world applied to (0,0,0x1000) and (0,0,0)) gives
 *      a yaw, RotMatrix(0,-yaw,0) is placed at (p0.x, obj+0x60, p0.z),
 *      composed through the view and scaled by obj+0x1C minus a quarter of
 *      the height gap (root+0x60 above obj+0x60), and the four corners
 *      (+-obj+0x26, 0, +-obj+0x28) project into the per-context POLY_FT4 at
 *      obj+0xB8+ctx*0x28, linked at the unclamped OT index of the nearest
 *      SZ; obj+0x39 = 0x6B - clamp(index, 0x2D8)/8;
 *   3. rootLight = light x root.world (root+0x2C); for every node with a
 *      mesh (node+8 != FFFF) and node[7] set: light = rootLight x
 *      node.world -> SetLightMatrix, view = rootView x node.world, with the
 *      node+0x52 billboard override (rows scaled by obj+0x1C, column 1 taken
 *      from rootView when node+0x52 == 1), then func_8002C700(model,
 *      node+0x68+ctx*4, ot, drawArg);
 *   4. the obj+0x114 anchor/chain list (count obj[0x10D], stride 0x24):
 *      anchors and chain points are transformed by root.local x
 *      node.world and handed to func_801E22F8 with the sway offset
 *      (D_801E8698 rotated by root yaw + 0x400, y = obj+0x3E);
 *   5. the obj+0x118 list (count obj[0x10E], stride 0x30): func_801E1258;
 *   6. the obj+0x110 trail list (count obj[0x10C], stride 0x70): two points
 *      per entry projected into an eight-slot ring, aux-pool packets
 *      allocated through func_801E0248 when the ring count exceeds
 *      entry+0x60 or no packet exists; a wrapped ring count leaves the entry
 *      pointer in place, as retail does. */
void func_801DCEC8(u8* obj, MATRIX* view, MATRIX* light, s32 drawArg, s32 arg5,
                  u32* ot, s32 renderContextIndex)
{
    MATRIX* temp = (MATRIX*)g_PsxScratchpad;
    MATRIX* local = (MATRIX*)(g_PsxScratchpad + 0x20);
    MATRIX* rootView = (MATRIX*)(g_PsxScratchpad + 0x40);
    OvlyPtrTab* tab;
    u8* root;
    u32 count;
    u16 scale;
    u32 i;

    if (obj[0x34] == 0) {
        return;
    }
    root = (u8*)(uintptr_t)*(u32*)(obj + 4);
    tab = (OvlyPtrTab*)(uintptr_t)*(u32*)obj;
    scale = *(u16*)(obj + 0x1C);
    count = *(u16*)(root + 0xA);
    CompMatrix(view, (MATRIX*)(root + 0x0C), rootView);

    if ((*(u16*)(obj + 0x4A) & 1) == 0) {
        SVECTOR sv;
        s32 p1[3];
        s32 p0[3];
        s32 angle;
        s32 v;
        s32 minZ;
        s32 z;
        s32 otIndex;
        u8* pkt;

        CompMatrix((MATRIX*)(root + 0x0C), (MATRIX*)(root + 0xA8), temp);
        SetRotMatrix(temp);
        SetTransMatrix(temp);
        sv.vx = 0;
        sv.vy = 0;
        sv.vz = 0x1000;
        gte_ldv0(&sv);
        gte_rt();
        p1[0] = (s32)MFC2(25);
        p1[1] = (s32)MFC2(26);
        p1[2] = (s32)MFC2(27);
        sv.vz = 0;
        gte_ldv0(&sv);
        gte_rt();
        p0[0] = (s32)MFC2(25);
        p0[1] = (s32)MFC2(26);
        p0[2] = (s32)MFC2(27);
        angle = ratan2(p1[2] - p0[2], p1[0] - p0[0]);
        sv.vx = 0;
        sv.vy = (s16)(-angle);
        sv.vz = 0;
        RotMatrix(&sv, local);
        local->t[0] = p0[0];
        local->t[1] = *(s16*)(obj + 0x60);
        local->t[2] = p0[2];
        CompMatrix(view, local, local);
        v = *(s16*)(obj + 0x60) - *(s32*)(root + 0x60);
        if (v < 0) {
            v += 3;
        }
        v >>= 2;
        v = *(s16*)(obj + 0x1C) - v;
        if (v < 0) {
            v = 0;
        }
        temp->m[0][0] = (s16)v;
        temp->m[0][1] = 0;
        temp->m[0][2] = 0;
        temp->m[1][0] = 0;
        temp->m[1][1] = (s16)v;
        temp->m[1][2] = 0;
        temp->m[2][0] = 0;
        temp->m[2][1] = 0;
        temp->m[2][2] = (s16)v;
        MulMatrix(local, temp);
        SetRotMatrix(local);
        SetTransMatrix(local);

        pkt = obj + 0xB8 + renderContextIndex * 0x28;
        sv.vx = (s16)*(u16*)(obj + 0x26);
        sv.vy = 0;
        sv.vz = (s16)*(u16*)(obj + 0x28);
        gte_ldv0(&sv);
        gte_rtps();
        *(u32*)(pkt + 0x08) = MFC2(14);
        minZ = (s32)MFC2(19) >> 2;
        sv.vx = (s16)(-(s32)*(u16*)(obj + 0x26));
        gte_ldv0(&sv);
        gte_rtps();
        *(u32*)(pkt + 0x10) = MFC2(14);
        z = (s32)MFC2(19) >> 2;
        if (z < minZ) {
            minZ = z;
        }
        sv.vx = (s16)*(u16*)(obj + 0x26);
        sv.vz = (s16)(-(s32)*(u16*)(obj + 0x28));
        gte_ldv0(&sv);
        gte_rtps();
        *(u32*)(pkt + 0x18) = MFC2(14);
        z = (s32)MFC2(19) >> 2;
        if (z < minZ) {
            minZ = z;
        }
        sv.vx = (s16)(-(s32)*(u16*)(obj + 0x26));
        gte_ldv0(&sv);
        gte_rtps();
        *(u32*)(pkt + 0x20) = MFC2(14);
        z = (s32)MFC2(19) >> 2;
        if (z < minZ) {
            minZ = z;
        }
        otIndex = minZ >> (D_80050100 & 31);
        /* Retail links at the unclamped index and only clamps the copy that
         * feeds the shade byte. */
        PcPort_LinkModelPrim(ot, otIndex, pkt, *(u32*)pkt & 0xFF000000u);
        if (otIndex >= 0x2D9) {
            otIndex = 0x2D8;
        }
        if (otIndex < 0) {
            otIndex += 7;
        }
        obj[0x39] = (u8)(0x6B - (otIndex >> 3));
    }

    MulMatrix0(light, (MATRIX*)(root + 0x2C), local);
    for (i = 1; i < count; ++i) {
        u8* node = root + i * OVLY_NODE_STRIDE;
        u16 mesh = *(u16*)(node + 8);
        if (mesh == 0xFFFF || node[7] == 0) {
            continue;
        }
        MulMatrix0(local, (MATRIX*)(node + 0x2C), temp);
        SetLightMatrix(temp);
        CompMatrix(rootView, (MATRIX*)(node + 0x2C), temp);
        if (*(s16*)(node + 0x52) > 0) {
            temp->m[0][2] = 0;
            temp->m[1][0] = 0;
            temp->m[1][2] = 0;
            temp->m[2][0] = 0;
            temp->m[0][0] = (s16)scale;
            temp->m[2][2] = (s16)scale;
            if (*(s16*)(node + 0x52) == 1) {
                temp->m[0][1] = rootView->m[0][1];
                temp->m[1][1] = rootView->m[1][1];
                temp->m[2][1] = rootView->m[2][1];
            } else {
                temp->m[0][1] = 0;
                temp->m[2][1] = 0;
                temp->m[1][1] = (s16)scale;
            }
        }
        SetRotMatrix(temp);
        SetTransMatrix(temp);
        func_8002C700((void*)(uintptr_t)tab->ptrs[mesh],
                     (void*)(uintptr_t)*(u32*)(node + 0x68 + renderContextIndex * 4),
                     (void*)ot, drawArg);
    }

    {
        u32 n = obj[0x10D];
        u8* e = (u8*)(uintptr_t)*(u32*)(obj + 0x114);
        u32 k;
        for (k = 0; k < n; ++k, e += 0x24) {
            s16 off[3];
            s32 yaw;
            s32 s;
            s32 j;
            if (*(s16*)e < 0) {
                continue;
            }
            yaw = *(s16*)(root + 0x56) + 0x400;
            s = -(s32)D_801E8698 * rsin(yaw);
            if (s < 0) {
                s += 0xFFF;
            }
            off[0] = (s16)(s >> 12);
            s = (s32)D_801E8698 * rcos(yaw);
            if (s < 0) {
                s += 0xFFF;
            }
            off[2] = (s16)(s >> 12);
            off[1] = (s16)*(u16*)(obj + 0x3E);
            CompMatrix((MATRIX*)(root + 0x0C),
                       (MATRIX*)(root + *(s16*)e * OVLY_NODE_STRIDE + 0x2C), temp);
            SetRotMatrix(temp);
            SetTransMatrix(temp);
            for (j = 0; j < *(s16*)(e + 4); ++j) {
                u8* vertex = (u8*)(uintptr_t)*(u32*)(e + 0x14) + j * 8;
                s16* out;
                gte_ldv0(vertex);
                gte_rt();
                out = (s16*)(uintptr_t)((u32*)(uintptr_t)*(u32*)(e + 0x1C))[j];
                out[2] = (s16)MFC2(25);
                out[3] = (s16)MFC2(26);
                out[4] = (s16)MFC2(27);
            }
            {
                s32 m = *(s16*)(e + 0xA);
                u8* item = (u8*)(uintptr_t)*(u32*)(e + 0x18);
                for (j = 0; j < m; ++j, item += 0x10) {
                    CompMatrix((MATRIX*)(root + 0x0C),
                               (MATRIX*)(root + *(s16*)(item + 6) * OVLY_NODE_STRIDE + 0x2C),
                               temp);
                    SetRotMatrix(temp);
                    SetTransMatrix(temp);
                    gte_ldv0(item);
                    gte_rt();
                    *(s16*)(item + 0x8) = (s16)MFC2(25);
                    *(s16*)(item + 0xA) = (s16)MFC2(26);
                    *(s16*)(item + 0xC) = (s16)MFC2(27);
                }
            }
            func_801E22F8(e, off, view, ot, renderContextIndex, (s32)scale,
                          *(s16*)(obj + 0x60));
        }
    }

    {
        u32 n = obj[0x10E];
        u8* e = (u8*)(uintptr_t)*(u32*)(obj + 0x118);
        u32 k;
        for (k = 0; k < n; ++k, e += 0x30) {
            func_801E1258(e, arg5);
        }
    }

    {
        u32 n = obj[0x10C];
        u8* entry = (u8*)(uintptr_t)*(u32*)(obj + 0x110);
        u32 k;
        for (k = 0; k < n; ++k) {
            u32 idx;
            s16 ring;
            u8* p;
            if (*(s16*)entry < 0) {
                entry += 0x70;
                continue;
            }
            idx = (u32)(*(u16*)(entry + 0x5C) - 1u) & 7u;
            *(u16*)(entry + 0x5C) = (u16)idx;
            if (entry[2] == 0) {
                CompMatrix(rootView,
                           (MATRIX*)(root + *(s16*)entry * OVLY_NODE_STRIDE + 0x2C), temp);
                SetRotMatrix(temp);
                SetTransMatrix(temp);
                gte_ldv0(entry + 0xC);
                gte_rtps();
                *(u32*)(entry + 0x1C + idx * 4) = MFC2(14);
                gte_ldv0(entry + 0x14);
                gte_rtps();
                *(u32*)(entry + 0x3C + idx * 4) = MFC2(14);
                ring = (s16)(*(u16*)(entry + 0x5E) + 1);
                *(u16*)(entry + 0x5E) = (u16)ring;
                if (ring == 0) {
                    continue;
                }
                if (*(s16*)(entry + 0x60) < ring || *(u32*)(entry + 8) == 0) {
                    u32 kk;
                    p = func_801E0248((u8*)(uintptr_t)*(u32*)(entry + 4), entry[3]);
                    *(u16*)(entry + 0x5E) = 1;
                    *(u32*)(entry + 8) = (u32)(uintptr_t)p;
                    *(u16*)(p + 0x0E) = 0;
                    *(u16*)(p + 0x16) = 0;
                    *(u16*)(p + 0x1E) = *(u16*)(entry + 0x62);
                    *(u16*)(p + 0x20) = *(u16*)(entry + 0x64);
                    *(u16*)(p + 0x22) = *(u16*)(entry + 0x66);
                    *(u16*)(p + 0x24) = *(u16*)(entry + 0x68);
                    *(u16*)(p + 0x26) = *(u16*)(entry + 0x6A);
                    *(u16*)(p + 0x28) = *(u16*)(entry + 0x6C);
                    *(u16*)(p + 0x2A) = *(u16*)(entry + 0x6E);
                    kk = (u32)(*(s16*)(entry + 0x5C) + *(s16*)(entry + 0x5E)) & 7u;
                    *(u16*)(p + 0x00) = *(u16*)(entry + 0x1C + kk * 4);
                    *(u16*)(p + 0x02) = *(u16*)(entry + 0x1E + kk * 4);
                    *(u16*)(p + 0x10) = *(u16*)(entry + 0x3C + kk * 4);
                    *(u16*)(p + 0x12) = *(u16*)(entry + 0x3E + kk * 4);
                }
                p = (u8*)(uintptr_t)*(u32*)(entry + 8);
                *(u16*)(p + 0x08) = *(u16*)(entry + 0x1C + idx * 4);
                *(u16*)(p + 0x0A) = *(u16*)(entry + 0x1E + idx * 4);
                *(u16*)(p + 0x18) = *(u16*)(entry + 0x3C + idx * 4);
                *(u16*)(p + 0x1A) = *(u16*)(entry + 0x3E + idx * 4);
            } else {
                u32 half = idx & 1u;
                u8* a = entry + 0x1C + half * 16;
                u8* b = entry + 0x3C + half * 16;
                CompMatrix((MATRIX*)(root + 0x0C),
                           (MATRIX*)(root + *(s16*)entry * OVLY_NODE_STRIDE + 0x2C), temp);
                SetRotMatrix(temp);
                SetTransMatrix(temp);
                gte_ldv0(entry + 0xC);
                gte_rt();
                ((u32*)a)[0] = MFC2(25);
                ((u32*)a)[1] = MFC2(26);
                ((u32*)a)[2] = MFC2(27);
                gte_ldv0(entry + 0x14);
                gte_rt();
                ((u32*)b)[0] = MFC2(25);
                ((u32*)b)[1] = MFC2(26);
                ((u32*)b)[2] = MFC2(27);
                ring = (s16)(*(u16*)(entry + 0x5E) + 1);
                *(u16*)(entry + 0x5E) = (u16)ring;
                if (ring == 0) {
                    continue;
                }
                if (*(s16*)(entry + 0x60) < ring || *(u32*)(entry + 8) == 0) {
                    u8* src = entry + 0x1C + (1u - half) * 16;
                    u8* src2 = entry + 0x3C + (1u - half) * 16;
                    p = func_801E0248((u8*)(uintptr_t)*(u32*)(entry + 4), entry[3]);
                    *(u16*)(entry + 0x5E) = 1;
                    *(u32*)(entry + 8) = (u32)(uintptr_t)p;
                    *(u16*)(p + 0x0E) = 1;
                    *(u16*)(p + 0x16) = 0;
                    *(u16*)(p + 0x1E) = *(u16*)(entry + 0x62);
                    *(u16*)(p + 0x20) = *(u16*)(entry + 0x64);
                    *(u16*)(p + 0x22) = *(u16*)(entry + 0x66);
                    *(u16*)(p + 0x24) = *(u16*)(entry + 0x68);
                    *(u16*)(p + 0x26) = *(u16*)(entry + 0x6A);
                    *(u16*)(p + 0x28) = *(u16*)(entry + 0x6C);
                    *(u16*)(p + 0x2A) = *(u16*)(entry + 0x6E);
                    *(u16*)(p + 0x00) = *(u16*)(src + 0);
                    *(u16*)(p + 0x02) = *(u16*)(src + 4);
                    *(u16*)(p + 0x04) = *(u16*)(src + 8);
                    *(u16*)(p + 0x10) = *(u16*)(src2 + 0);
                    *(u16*)(p + 0x12) = *(u16*)(src2 + 4);
                    *(u16*)(p + 0x14) = *(u16*)(src2 + 8);
                }
                p = (u8*)(uintptr_t)*(u32*)(entry + 8);
                *(u16*)(p + 0x08) = *(u16*)(a + 0);
                *(u16*)(p + 0x0A) = *(u16*)(a + 4);
                *(u16*)(p + 0x0C) = *(u16*)(a + 8);
                *(u16*)(p + 0x18) = *(u16*)(b + 0);
                *(u16*)(p + 0x1A) = *(u16*)(b + 4);
                *(u16*)(p + 0x1C) = *(u16*)(b + 8);
            }
            entry += 0x70;
        }
    }
}

/* Retail [801E0398,801E0698), archive 6B9: auxiliary sprite pool draw.
 * The pool header is (entries, capacity, next); every live entry
 * (entry+0x16 != -1) whose age is below its life (entry+0x1E) is drawn into
 * its per-context packet (entry + ctx*0x28: tag +0x2C, rgb +0x30, xy
 * +0x34/3C/44/4C) -- screen-space entries (entry+0xE == 0) copy their four
 * xy pairs and link at OT[0], world-space entries RTPT/RTPS their four
 * vertices and link at the SZ3-derived depth -- then ages by the tick
 * count and fades its colour channels by entry+0x26/28/2A per tick. Entries
 * past their life are retired through func_801E0354. */
void func_801E0398(u8* pool, MATRIX* view, s32 ticks, u32* ot, s32 renderContextIndex)
{
    s32 cap;
    u8* entry;
    s32 k;

    SetRotMatrix(view);
    SetTransMatrix(view);
    cap = *(s16*)(pool + 4);
    entry = (u8*)(uintptr_t)*(u32*)pool;
    for (k = 0; k < cap; ++k, entry += 0x7C) {
        u8* base;
        if (*(s16*)(entry + 0x16) == -1) {
            continue;
        }
        if (*(s16*)(entry + 0x16) >= *(s16*)(entry + 0x1E)) {
            func_801E0354(pool, entry);
            continue;
        }
        base = entry + renderContextIndex * 0x28;
        base[0x30] = (u8)(*(u16*)(entry + 0x20) >> 6);
        base[0x31] = (u8)(*(u16*)(entry + 0x22) >> 6);
        base[0x32] = (u8)(*(u16*)(entry + 0x24) >> 6);
        if (*(s16*)(entry + 0xE) == 0) {
            *(u16*)(base + 0x34) = *(u16*)(entry + 0x00);
            *(u16*)(base + 0x36) = *(u16*)(entry + 0x02);
            *(u16*)(base + 0x3C) = *(u16*)(entry + 0x08);
            *(u16*)(base + 0x3E) = *(u16*)(entry + 0x0A);
            *(u16*)(base + 0x44) = *(u16*)(entry + 0x10);
            *(u16*)(base + 0x46) = *(u16*)(entry + 0x12);
            *(u16*)(base + 0x4C) = *(u16*)(entry + 0x18);
            *(u16*)(base + 0x4E) = *(u16*)(entry + 0x1A);
            PcPort_LinkModelPrim(ot, 0, base + 0x2C, *(u32*)(base + 0x2C) & 0xFF000000u);
        } else {
            s32 otz;
            MTC2(*(u32*)(entry + 0x00), 0);
            MTC2(*(u32*)(entry + 0x04), 1);
            MTC2(*(u32*)(entry + 0x08), 2);
            MTC2(*(u32*)(entry + 0x0C), 3);
            MTC2(*(u32*)(entry + 0x10), 4);
            MTC2(*(u32*)(entry + 0x14), 5);
            gte_rtpt();
            *(u32*)(base + 0x34) = MFC2(12);
            *(u32*)(base + 0x3C) = MFC2(13);
            *(u32*)(base + 0x44) = MFC2(14);
            otz = ((s32)MFC2(19) >> 2) >> (D_80050100 & 31);
            gte_ldv0(entry + 0x18);
            gte_rtps();
            *(u32*)(base + 0x4C) = MFC2(14);
            PcPort_LinkModelPrim(ot, otz, base + 0x2C, *(u32*)(base + 0x2C) & 0xFF000000u);
        }
        *(u16*)(entry + 0x16) = (u16)(*(u16*)(entry + 0x16) + ticks);
        *(u16*)(entry + 0x22) = (u16)(*(u16*)(entry + 0x22) - *(s16*)(entry + 0x28) * ticks);
        *(u16*)(entry + 0x20) = (u16)(*(u16*)(entry + 0x20) - *(s16*)(entry + 0x26) * ticks);
        *(u16*)(entry + 0x24) = (u16)(*(u16*)(entry + 0x24) - *(s16*)(entry + 0x2A) * ticks);
    }
}

/* Retail 801DC2D0..C5C0:
 * d10a984d37284d898fcea03ff7360ef9d0cb0a0e0e41046775aa7020c1681e6a.
 * Matrix bytes are not initialized here; the hierarchy update owns them. */
u8* func_801DC2D0(u8* table, u16* list, s32 mode, s32 setup,
                 s16 texX, s16 texY, s16 clutX, s16 clutY)
{
    HeapChangeCurrentUser(4, NULL);
    u32 records = 0;
    while ((u32)list[records * 2] < *(u32*)(table + 4) || list[records * 2] == 0xFFFF)
        ++records;
    if (records == 0) return NULL;
    u8* root = HeapAlloc((records + 1u) * 0x7Cu, 0);
    if (root == NULL) return NULL;
    *(u32*)root = 0;
    root[4] = root[5] = root[6] = 1;
    root[7] = 0;
    *(u16*)(root + 8) = 0xFFFF;
    *(u16*)(root + 10) = (u16)(records + 1u);
    *(u16*)(root + 0x4C) = *(u16*)(root + 0x4E) = *(u16*)(root + 0x50) = 0x1000;
    *(u32*)(root + 0x68) = *(u32*)(root + 0x6C) = 0;
    *(u16*)(root + 0x54) = *(u16*)(root + 0x56) = *(u16*)(root + 0x58) = 0;
    *(u32*)(root + 0x5C) = *(u32*)(root + 0x60) = *(u32*)(root + 0x64) = 0;
    *(u32*)(root + 0x70) = *(u32*)(root + 0x74) = *(u32*)(root + 0x78) = 0;
    u8* node = root + 0x7C;
    u32 index = 1;
    while ((u32)list[0] < *(u32*)(table + 4) || list[0] == 0xFFFF) {
        u16 modelIndex = list[0], parentIndex = list[1];
        *(u32*)node = parentIndex == 0xFFFF ? 0 : (u32)(uintptr_t)root + ((u32)parentIndex + 1u) * 0x7Cu;
        *(u16*)(node + 10) = (u16)index++;
        node[4] = node[5] = node[7] = 1;
        node[6] = 0;
        *(u16*)(node + 0x4C) = *(u16*)(node + 0x4E) = *(u16*)(node + 0x50) = 0x1000;
        *(u16*)(node + 0x52) = 0;
        *(u16*)(node + 8) = modelIndex;
        if (modelIndex != 0xFFFF) {
            u32* pointers = (u32*)(uintptr_t)*(u32*)table;
            func_8002CB54((u8*)(uintptr_t)pointers[modelIndex], (u32*)(node + 0x68), (u32*)(node + 0x6C));
            if (*(u32*)(node + 0x68) == 0) {
                func_801DCD8C(root);
                return NULL;
            }
            if (setup != 0) {
                func_8002CC10((u16)texX, (u16)texY);
                func_8002CC74((u16)clutX, (u16)clutY);
            }
            pointers = (u32*)(uintptr_t)*(u32*)table;
            func_8002C8CC((u8*)(uintptr_t)pointers[modelIndex], (void*)(uintptr_t)*(u32*)(node + 0x68), mode);
            pointers = (u32*)(uintptr_t)*(u32*)table;
            u8* model = (u8*)(uintptr_t)pointers[modelIndex];
            memcpy((void*)(uintptr_t)*(u32*)(node + 0x6C), (void*)(uintptr_t)*(u32*)(node + 0x68), *(u32*)(model + 0x34));
        } else {
            *(u32*)(node + 0x68) = *(u32*)(node + 0x6C) = 0;
        }
        *(u16*)(node + 0x54) = *(u16*)(node + 0x56) = *(u16*)(node + 0x58) = 0;
        *(u32*)(node + 0x5C) = *(u32*)(node + 0x60) = *(u32*)(node + 0x64) = 0;
        *(u32*)(node + 0x70) = *(u32*)(node + 0x74) = *(u32*)(node + 0x78) = 0;
        node += 0x7C;
        list += 2;
    }
    return root;
}

/* Pointer-width bridge only; the retail constructor owns list semantics,
 * node initialization, packet building, and failure cleanup. */
static u8* OvlyBuildNodes(OvlyPtrTab* tab, u16* list, s32 buildMode, s32 setup,
                          s16 texX, s16 texY, s16 clutX, s16 clutY) {
    u32 packed[2] = {(u32)(uintptr_t)tab->ptrs, (u32)tab->count};
    return func_801DC2D0((u8*)packed, list, buildMode, setup,
                        texX, texY, clutX, clutY);
}

/* DCCC8..DCD40 selects the model by node+8, leaving node+70 to the track VM. */
static u8* OvlyNodeModel(OvlyPtrTab* tab, const u8* node) {
    u16 index = *(const u16*)(node + 8);
    return index == 0xFFFF ? NULL : (u8*)(uintptr_t)tab->ptrs[index];
}

/* Native owner adapter for retail func_801E8030.
 *
 * Retail authority: disc1 archive entry 0x860 (field-local 0x6B9), loaded at
 * 0x801DC000. The function is 0x801E8030..0x801E8330, SHA-256
 * e15a7ea45c5adf850542501ced1817d2f6bda71e2b355251dd2381cb3c3d95c7.
 * Retail first releases obj+0xA8, calls 0x801DCE18 on obj+0 (including
 * func_8002CBBC for every model), releases the node packet allocations and
 * node block through 0x801DCD8C, releases auxiliary owners, then frees the
 * object and clears D_801E8670[slot].
 *
 * The native overlay deliberately represents only the owners constructed by
 * OvlyBuildPtrTab/OvlyBuildNodes/func_801E742C. Retail-only auxiliary arrays
 * at +0xAC/+0xB0/+0x110/+0x114/+0x118 are never constructed here. Refuse a
 * foreign/extended object before freeing anything rather than pretending its
 * internal destructors are available. Node attachment fields also differ in
 * the host representation, so cleanup follows the allocations this adapter
 * actually owns while retaining retail's model-buffer cleanup boundary. */
static int OvlyObjectHasUnsupportedOwners(const u8* obj)
{
    return obj[0x62] != 0 || *(u32*)(obj + 0xAC) != 0 ||
           obj[0x10C] != 0 || obj[0x10D] != 0 || obj[0x10E] != 0 ||
           *(u32*)(obj + 0x110) != 0 || *(u32*)(obj + 0x114) != 0 ||
           *(u32*)(obj + 0x118) != 0;
}

static void OvlyFreeOwnedPtrTab(OvlyPtrTab* tab)
{
    s32 i;

    if (tab->ptrs == NULL) {
        tab->count = 0;
        return;
    }
    if (OvlySanityCount(tab->count)) {
        for (i = 0; i < tab->count; i++) {
            u8* model = (u8*)(uintptr_t)tab->ptrs[i];
            if (model != NULL) {
#ifndef XENO_TEST_MUTATE_FIELD_OBJECT_SKIP_MODEL_CLEANUP
                func_8002CBBC(model);
#endif
            }
        }
    } else if (OvlyDiag()) {
        fprintf(stderr, "[obj-ovly] 8030 invalid ptrtab count=%d\n",
                (int)tab->count);
    }
    HeapFree(tab->ptrs);
    tab->ptrs = NULL;
    tab->count = 0;
}

/* Retail 801DCD8C..801DCE18, SHA-256
 * 4b9ad7ffc3d4742466f4ef70efbc8e210d78f0b8ce6dcec9ece683933a4bfcba. */
void func_801DCD8C(u8* root)
{
    if (root == NULL) return;
    u8* node = root;
    for (s32 i = 0; i < *(u16*)(root + 0x0A); ++i, node += 0x7C) {
        void* packet = (void*)(uintptr_t)*(u32*)(node + 0x68);
        if (packet != NULL) {
            HeapFree(packet);
            *(u32*)(node + 0x68) = 0;
            *(u32*)(node + 0x6C) = 0;
        }
    }
    *(u16*)(root + 0x0A) = 0;
    HeapFree(root);
}

static void OvlyFreeOwnedNodes(u8* root)
{
    func_801DCD8C(root);
}

void func_801E8030(s32 slot)
{
    u8* obj;
    OvlyPtrTab* ownedTab;
    u32 expectedTab;

    if ((u32)slot >= OVLY_SLOT_MAX) {
        return;
    }
    obj = (u8*)(uintptr_t)D_801E8670[slot];
    if (obj == NULL) {
        return;
    }

    ownedTab = &s_ptrTab[slot];
    expectedTab = (u32)(uintptr_t)ownedTab;
    if (*(u32*)obj != expectedTab || OvlyObjectHasUnsupportedOwners(obj)) {
        if (OvlyDiag()) {
            fprintf(stderr,
                    "[obj-ovly] 8030 refuse foreign/extended slot=%d obj=%p "
                    "tab=%08x expected=%08x\n",
                    (int)slot, (void*)obj, (unsigned)*(u32*)obj,
                    (unsigned)expectedTab);
        }
        return;
    }

    if (*(u32*)(obj + 0xA8) != 0) {
        HeapFree((void*)(uintptr_t)*(u32*)(obj + 0xA8));
        *(u32*)(obj + 0xA8) = 0;
    }
    OvlyFreeOwnedPtrTab(ownedTab);
    OvlyFreeOwnedNodes((u8*)(uintptr_t)*(u32*)(obj + 4));
    *(u32*)(obj + 4) = 0;
    HeapFree(obj);
#ifndef XENO_TEST_MUTATE_FIELD_OBJECT_SKIP_SLOT_CLEAR
    D_801E8670[slot] = 0;
#endif
}

/* Retail 0x801E7FD4..0x801E8030, SHA-256
 * 0fc1564d18b625500a97b6545bd868c46d1dd8aa93937d392b5e60fbbc2d1218.
 * It destroys slots 0..9, then applies the simple retail owner destructors
 * 0x801DF668 and 0x801E00DC to workspaces 0x801E86A8 and 0x801E86A0. */
void func_801E00DC(u8* pool);

void func_801E7FD4(void)
{
    s32 slot;

    for (slot = 0; slot < OVLY_SLOT_MAX; slot++) {
#ifndef XENO_TEST_MUTATE_FIELD_OBJECT_SKIP_DESTROY_ALL_SLOT
        func_801E8030(slot);
#endif
    }

    func_801DF668(D_801E86A8);
    func_801E00DC(D_801E86A0);
}

/* Retail auxiliary pool 0x801E0064..0x801E0248, SHA-256
 * 587c6c424093b1ce08f561d48477b1176b36a89236f27f7dbd1bb8e3aecdc621.
 * Unlike the track pool, reset includes the extra record and uses a signed
 * capacity. Other record fields intentionally retain their prior contents. */
void func_801E011C(u8* pool)
{
    u8* entry = (u8*)(uintptr_t)*(u32*)pool;
    for (s32 i = 0; i < (s32)*(s16*)(pool + 4) + 1; ++i, entry += 0x7C) {
        *(s16*)(entry + 0x16) = -1;
        *(u16*)(entry + 0x1E) = 0;
        for (s32 j = 0; j < 2; ++j) {
            u8* p = entry + 0x2C + j * 0x28;
            SetPolyFT4((POLY_FT4*)p);
            SetSemiTrans(p, 1);
            *(u16*)(p + 0x0E) = GetClut(0, 0x1CD);
            *(u16*)(p + 0x16) = GetTPage(0, 1, 0x340, 0x100);
            p[0x0C] = 0; p[0x0D] = 0xBD;
            p[0x14] = 0; p[0x15] = 0xBD;
            p[0x1C] = 0x0F; p[0x1D] = 0xBD;
            p[0x24] = 0x0F; p[0x25] = 0xBD;
        }
    }
}

u8* func_801E0064(u8* pool, s32 count)
{
    HeapChangeCurrentUser(4, NULL);
    /* Byte copies preserve the packed halfword updates even when a host heap
     * adapter observes this header through a word-sized owner. */
    u16 capacity = (u16)count, next = 0;
    memcpy(pool + 4, &capacity, sizeof(capacity));
    memcpy(pool + 6, &next, sizeof(next));
    u8* entries = HeapAlloc(((u32)count + 1u) * 124u, 0);
    *(u32*)pool = (u32)(uintptr_t)entries;
    if (entries == NULL) return NULL;
    func_801E011C(pool);
    return pool;
}

void func_801E00DC(u8* pool)
{
    void* entries = (void*)(uintptr_t)*(u32*)pool;
    u32 emptyIndices = 0;
    memcpy(pool + 4, &emptyIndices, sizeof(emptyIndices));
    if (entries != NULL) HeapFree(entries);
    *(u32*)pool = 0;
}

/* Retail 801E0248..E0398:
 * 5ff4f732dc907fb123d331f6d44b62452489a74bf5dd11f8aae6c5de175fb5b5. */
u8* func_801E0248(u8* pool, s32 semiTrans)
{
    s32 next = *(s16*)(pool + 6);
    if (next < *(s16*)(pool + 4)) {
        u8* entry = (u8*)(uintptr_t)(*(u32*)pool + (u32)next * 124u);
        if (*(s16*)(entry + 0x16) == -1) {
            *(u16*)(pool + 6) = (u16)(next + 1);
            s32 capacity = *(s16*)(pool + 4);
            u32 base = *(u32*)pool;
            while (*(s16*)(pool + 6) < capacity) {
                next = *(s16*)(pool + 6);
                u8* candidate = (u8*)(uintptr_t)(base + (u32)next * 124u);
                if (*(s16*)(candidate + 0x16) == -1) break;
                *(u16*)(pool + 6) = (u16)(next + 1);
            }
            SetSemiTrans(entry + 0x2C, (s16)semiTrans);
            SetSemiTrans(entry + 0x54, (s16)semiTrans);
            return entry;
        }
    }
    return (u8*)(uintptr_t)(*(u32*)pool + (u32)(s32)*(s16*)(pool + 4) * 124u);
}

s32 func_801E0354(u8* pool, u8* entry)
{
    u32 difference = (u32)(uintptr_t)entry - *(u32*)pool;
    u32 index = difference / 124u;
    if (*(s16*)(pool + 6) >= (s32)index) *(u16*)(pool + 6) = (u16)index;
    *(s16*)(entry + 0x16) = -1;
    return (s32)index;
}

/* Retail 801E165C..1708:
 * 5ef30c514c52e541640ab7ab25be5e59c48a14278a899750b4056e9bfedff91d. */
void func_801E165C(u8* effect)
{
    if (*(u16*)(effect + 0x1A) == 0) return;
    if (*(u32*)(effect + 4) != 0) {
        if (effect[0x10] < 4)
            LoadImage((RECT16*)(effect + 0x28), (u_long*)(uintptr_t)*(u32*)(effect + 4));
        HeapFree((void*)(uintptr_t)*(u32*)(effect + 4));
        *(u32*)(effect + 4) = 0;
    }
    if (*(u32*)(effect + 8) != 0) {
        HeapFree((void*)(uintptr_t)*(u32*)(effect + 8));
        *(u32*)(effect + 8) = 0;
    }
    if (*(u32*)(effect + 0xC) != 0) {
        HeapFree((void*)(uintptr_t)*(u32*)(effect + 0xC));
        *(u32*)(effect + 0xC) = 0;
    }
    *(u16*)(effect + 0x1A) = 0;
}

/* Retail 801E1708..1880:
 * 930c868225e060b0e36f6166c70e8fe3caaac7e9ce348ea7b573ab2c5bcafeaf.
 * The signed factor and unsigned source product fit s32; /32 matches the
 * retail negative-product bias followed by arithmetic shift. */
void func_801E1708(u8* effect, s32 factor)
{
    u16* source = (u16*)(uintptr_t)*(u32*)(effect + 4);
    s32 scale = (s16)factor;
    for (s32 y = 0; y < *(s16*)(effect + 0x2E); ++y) {
        for (s32 x = 0; x < *(s16*)(effect + 0x2C); ++x) {
            s32 product = (s32)*source * scale;
            u32 destination = *(u32*)(effect + 0x1C)
                + (u32)(*(s16*)(effect + 0x2A) + y) * 6u
                + (u32)(*(s16*)(effect + 0x28) + x) * 2u;
            *(u16*)(uintptr_t)destination = (u16)(product / 32);
            ++source;
        }
    }
}

void func_801E17B8(u8* effect, s32 factor)
{
    u16* source = (u16*)(uintptr_t)*(u32*)(effect + 4);
    u16* base = (u16*)(uintptr_t)*(u32*)(effect + 8);
    s32 scale = (s16)factor;
    for (s32 y = 0; y < *(s16*)(effect + 0x2E); ++y) {
        for (s32 x = 0; x < *(s16*)(effect + 0x2C); ++x) {
            s32 product = ((s32)*source - (s32)*base) * scale;
            u32 destination = *(u32*)(effect + 0x1C)
                + (u32)(*(s16*)(effect + 0x2A) + y) * 6u
                + (u32)(*(s16*)(effect + 0x28) + x) * 2u;
            ++source;
            *(u16*)(uintptr_t)destination = (u16)(*base + product / 32);
            ++base;
        }
    }
}

/* Packed effect+24 contains retail code addresses supplied by E34BC.
 * Resolve only the proven field-overlay entries; an unknown address is
 * unresolved execution, never an implied default callback/frame. */
static s32 FieldEffectCallback(u32 address, s32 phase, s32 divisor, s32 offset)
{
    switch (address) {
    case 0x801E0850: return func_801E0850(phase, divisor, offset);
    case 0x801E08D4: return func_801E08D4(phase, divisor, offset);
    case 0x801E0938: return func_801E0938(phase, divisor, offset);
    case 0x801E0988: return func_801E0988(phase, divisor, offset);
    default:
        fprintf(stderr, "[field-effect] unresolved retail callback %08x\n", address);
        __builtin_trap();
    }
}

extern void func_80026F44(s32, s32, u16*, const u16*);
extern void func_80026FE8(s32, s32, u16*, const u16*, const u16*);

/* Retail [801E0A00,801E1258), SHA-256
 * 4ede271950970cf43e61e2864660eaa9a27bbd4145f191c81330dab80d6ec32c.
 * incomingS1 is explicit CPU provenance, not a fabricated effect field. */
u8* PcPort_FieldEffectConstructWithS1(u8* effect, u8* parent,
    s32 typeArg, s32 flagsArg, u8* source,
    s32 ax, s32 ay, s32 az, s32 bx, s32 by, s32 bz,
    s32 x, s32 y, s32 widthArg, s32 heightArg,
    s32 period, s32 parameter0, s32 parameter1, u32 callback, u32 incomingS1)
{
    u16 type = (u16)typeArg, flags = (u16)flagsArg;
    u16 values[2][3] = {{(u16)ax,(u16)ay,(u16)az}, {(u16)bx,(u16)by,(u16)bz}};
    u16 width = (u16)widthArg, height = (u16)heightArg;
    u16 outputX = (u16)x, outputY = (u16)y;
    u16 savedPeriod = (u16)period, p0 = (u16)parameter0, p1 = (u16)parameter1;
    u16 carry = (u16)incomingS1;
    if (*(u16*)(effect + 0x1a) != 0) return NULL;
    HeapChangeCurrentUser(4, NULL);
    *(u16*)(effect + 0x1a) = 1;
    effect[0x11] = 0;
    effect[0x10] = (u8)type;
    *(u32*)effect = (u32)(uintptr_t)parent;
    *(u16*)(effect + 0x14) = 0;
    *(u16*)(effect + 0x18) = 0xffff;
    *(u16*)(effect + 0x16) = savedPeriod;
    *(u16*)(effect + 0x20) = p0;
    *(u16*)(effect + 0x22) = p1;
    *(u32*)(effect + 0x24) = callback;
    *(u32*)(effect + 0x1c) = (u32)(uintptr_t)source;
    if (!(typeArg & 1)) flags &= 0xfd0f;
    if (typeArg & 4) flags &= 0xfeff;
    if (width == 0) width = 0x100;
    if (height == 0) height = 0x100;
    if (type >= 6 || type == 2 || type == 3) return effect;
    if (type < 2) {
        s32 half = ((s32)(s16)width + 1) / 2;
        width = (u16)(half * 2);
        carry = (u16)half;
    }
    *(u16*)(effect + 0x28) = outputX;
    *(u16*)(effect + 0x2a) = outputY;
    *(u16*)(effect + 0x2c) = width;
    *(u16*)(effect + 0x2e) = height;
    *(u16*)(effect + 0x12) = (u16)((u32)width * height);
    u32 size = ((u32)(s32)(s16)width * (u32)(s32)(s16)height) << 1;
    for (u32 i = 0; i < 3; ++i) {
        if (flags & (0x100u << i))
            *(u32*)(effect + 0xc - i * 4) = (u32)(uintptr_t)HeapAlloc(size, 0);
    }
    for (u32 plane = 0; plane < 2; ++plane) {
        u32 mode = (flags >> (plane * 4)) & 0xf;
        if (type < 2) {
            if (mode == 1) {
                RECT rect = {(s16)values[plane][0], (s16)values[plane][1], (s16)width, (s16)height};
                StoreImage(&rect, (u_long*)(uintptr_t)*(u32*)(effect + 4 + plane * 4));
                DrawSync(0);
            } else if (mode == 2) {
                u16 color = (u16)((values[plane][0] & 0x1f)
                    + ((values[plane][1] & 0x1f) << 5)
                    + ((values[plane][2] & 0x3f) << 10));
                s32 count = (s32)((u32)(s32)(s16)width * (u32)(s32)(s16)height);
                for (s32 i = 0; i < count; ++i)
                    *(u16*)(uintptr_t)(*(u32*)(effect + 4 + plane * 4) + (u32)i * 2) = color;
            }
        } else if (mode == 1 || mode == 2) {
            for (s32 row = 0; row < (s16)height; ++row) {
                for (s32 column = 0; column < (s16)width; ++column) {
                    u16 value;
                    if (mode == 1) {
                        u32 sourceOffset = ((u32)(s32)(s16)values[plane][1] + (u32)row) * 6u
                            + ((u32)(s32)(s16)values[plane][0] + (u32)column) * 2u;
                        value = *(u16*)(uintptr_t)((u32)(uintptr_t)source + sourceOffset);
                    } else {
                        s32 component = ((s32)(s16)outputY + row) % 3;
                        if (component >= 0) carry = values[plane][component];
                        value = carry;
                    }
                    u32 destination = *(u32*)(effect + 4 + plane * 4);
                    u32 offset = ((u32)row * (u32)(s32)(s16)width + (u32)column) * 2u;
                    *(u16*)(uintptr_t)(destination + offset) = value;
                }
            }
        }
    }
    return effect;
}

/* Retail [801E1258,801E165C), SHA-256
 * bde5842f0f7b095691ed3a3be6284607384e7883073524e62cefd7bdfb2ebeff. */
s32 func_801E1258(u8* effect, s32 ticks)
{
    if (*(u16*)(effect + 0x1A) == 0) return -1;
    u32 advance = *(u16*)(effect + 0x16) * ((u32)ticks + 1u);
    *(u16*)(effect + 0x14) = (u16)(*(u16*)(effect + 0x14) + advance);
    s32 frame = (s16)FieldEffectCallback(*(u32*)(effect + 0x24),
        *(u16*)(effect + 0x14), *(s16*)(effect + 0x20), *(s16*)(effect + 0x22));
    if (frame < 0) {
        func_801E165C(effect);
        return frame;
    }
    if (frame == *(u16*)(effect + 0x18)) return frame;
    u8 type = effect[0x10];
    *(u16*)(effect + 0x18) = (u16)frame;
    if (type == 0 || type == 1) {
        if (type == 0)
            func_80026F44(*(s16*)(effect + 0x12), frame,
                (u16*)(uintptr_t)*(u32*)(effect + 0xC), (u16*)(uintptr_t)*(u32*)(effect + 4));
        else
            func_80026FE8(*(s16*)(effect + 0x12), frame,
                (u16*)(uintptr_t)*(u32*)(effect + 0xC),
                (u16*)(uintptr_t)*(u32*)(effect + 8), (u16*)(uintptr_t)*(u32*)(effect + 4));
        if (*(u32*)effect == 0)
            LoadImage((RECT16*)(effect + 0x28), (u_long*)(uintptr_t)*(u32*)(effect + 0xC));
    } else if (type == 4) {
        func_801E1708(effect, frame);
    } else if (type == 5) {
        func_801E17B8(effect, frame);
    }
    u8* linked = (u8*)(uintptr_t)*(u32*)effect;
    if (linked == NULL || *(u16*)(linked + 0x1A) == 0) return frame;
    s16 sourceOffset[2], destinationOffset[2], extent[2];
    for (unsigned axis = 0; axis < 2; ++axis) {
        unsigned position = 0x28 + axis * 2;
        unsigned size = position + 4;
        if (*(s16*)(linked + position) < *(s16*)(effect + position)) {
            destinationOffset[axis] = (s16)(*(s16*)(effect + position) - *(s16*)(linked + position));
            sourceOffset[axis] = 0;
            extent[axis] = (s16)(*(u16*)(linked + position) + *(u16*)(linked + size) - *(u16*)(effect + position));
        } else {
            destinationOffset[axis] = 0;
            sourceOffset[axis] = (s16)(*(u16*)(linked + position) - *(u16*)(effect + position));
            extent[axis] = (s16)(*(u16*)(effect + position) + *(u16*)(effect + size) - *(u16*)(linked + position));
        }
    }
    if (extent[0] <= 0 || extent[1] <= 0) return frame;
    u8 linkedType = linked[0x10];
    u32 destinationBase = *(u32*)(linked + 4);
    linked[0x11] = 1;
    u32 sourceBase = linkedType < 4 ? *(u32*)(effect + 0xC) : 0;
    for (s32 y = 0; y < extent[1]; ++y) {
        for (s32 x = 0; x < extent[0]; ++x) {
            u32 destination = destinationBase +
                ((u32)(destinationOffset[1] + y) * (u32)(s32)*(s16*)(linked + 0x2C)
                 + (u32)(destinationOffset[0] + x)) * 2u;
            u32 source;
            if (linkedType < 4)
                source = sourceBase + ((u32)(sourceOffset[1] + y) *
                    (u32)(s32)*(s16*)(effect + 0x2C) + (u32)(sourceOffset[0] + x)) * 2u;
            else
                source = *(u32*)(effect + 0x1C) + (u32)(sourceOffset[1] + y) * 6u
                    + (u32)(sourceOffset[0] + x) * 2u;
            *(u16*)(uintptr_t)destination = *(u16*)(uintptr_t)source;
        }
    }
    return frame;
}

/* Retail [801E1880,801E1A14), SHA-256
 * 66166223611c2503d903567f301fe314e8a346c3124b4a4cf1f3856589fa2be9.
 * Two packed light/effect records; slot and child indices stay signed. */
void func_801E1880(u32* objects)
{
    for (unsigned index = 0; index < 2; ++index) {
        u16* record = D_801E8648 + index * 10;
        if ((s16)record[3] == 0) continue;
        s32 slot = (s16)record[8];
        u8* object = slot < 0 ? NULL : (u8*)(uintptr_t)objects[slot];
        if (object != NULL) {
            u8* root = (u8*)(uintptr_t)*(u32*)(object + 4);
            u32 childMatrix = (u32)(uintptr_t)root + 0xA8u + (u32)(s32)(s16)record[9] * 0x7Cu;
            MATRIX* transform = (MATRIX*)g_PsxScratchpad;
            CompMatrix((MATRIX*)(root + 0xC), (MATRIX*)(uintptr_t)childMatrix, transform);
            SetRotMatrix(transform);
            SetTransMatrix(transform);
            u32 xy, zpad;
            memcpy(&xy, record + 4, 4);
            memcpy(&zpad, record + 6, 4);
            MTC2(xy, 0);
            MTC2(zpad, 1);
            doCOP2(0x00480012);
            record[0] = (u16)MFC2(25);
            record[1] = (u16)MFC2(26);
            record[2] = (u16)MFC2(27);
        } else {
            record[0] = record[4];
            record[1] = record[5];
            record[2] = record[6];
        }
    }
}

/* Retail [801E1A14,801E22F8), SHA-256
 * d2f25c7c9728b4929a214cf8dbc4db756e45d4d0ca75d405356eae1a130f3f03.
 * Null frees on allocation failure are intentional retail error dispatch. */
void func_801E1A14(u8* owner, u16* source, s32 bias, s32 scale,
    s32 x, s32 y, s32 z, s32 constraints, s32 texX, s32 texY,
    s32 width, s32 height, s32 clutX, s32 clutY,
    s32 r, s32 g, s32 b, s32 backR, s32 backG, s32 backB)
{
    *(u16*)(owner + 4) = *source++;
    *(u16*)(owner + 6) = (u16)(*source++ * 2u);
    HeapChangeCurrentUser(4, NULL);
    u8* anchors = HeapAlloc((u32)*(s16*)(owner + 4) * 8u, 0);
    *(u32*)(owner + 0x14) = (u32)(uintptr_t)anchors;
    if (anchors == NULL) return;
    s16 offsets[3] = {(s16)x, (s16)y, (s16)z};
    for (s32 i = 0; i < *(s16*)(owner + 4); ++i)
        for (unsigned axis = 0; axis < 3; ++axis) {
            s32 product = (s32)(((u32)*source++ + (u32)(s32)offsets[axis]) * (u32)scale);
            *(s16*)(anchors + i * 8 + axis * 2) = (s16)(product / 4096);
        }
    u16* counts = source;
    u32 total = source[*(s16*)(owner + 4)];
    *(u16*)(owner + 8) = (u16)(total + *(s16*)(owner + 4));
    u32* table = HeapAlloc((u32)*(s16*)(owner + 4) * 4u, 0);
    if (table == NULL) {
        *(u32*)(owner + 0x14) = 0;
        HeapFree(NULL);
        return;
    }
    *(u32*)(owner + 0x1C) = (u32)(uintptr_t)table;
    u16* lengths = counts + *(s16*)(owner + 4) + 1;
    u8* biases = (u8*)(lengths + total);
    u8* vertices = HeapAlloc((total + (u32)*(s16*)(owner + 4)) * 24u, 0);
    if (vertices == NULL) {
        *(u32*)(owner + 0x14) = 0;
        HeapFree(NULL);
        HeapFree((void*)(uintptr_t)*(u32*)(owner + 0x1C));
        return;
    }
    u8* vertex = vertices;
    anchors = (u8*)(uintptr_t)*(u32*)(owner + 0x14);
    for (s32 i = 0; i < *(s16*)(owner + 4); ++i) {
        table[i] = (u32)(uintptr_t)vertex;
        for (u32 j = 0; j < counts[i]; ++j) {
            *(s16*)vertex = (s16)((s32)((u32)*lengths++ * (u32)scale) / 4096);
            *(u16*)(vertex + 2) = (u16)((u32)*biases++ + (u32)bias);
            for (unsigned axis = 0; axis < 3; ++axis)
                *(u16*)(vertex + 4 + axis * 2) = *(u16*)(anchors + i * 8 + axis * 2);
            vertex += 24;
        }
        *(u16*)vertex = 0;
        *(u16*)(vertex + 2) = 0;
        for (unsigned axis = 0; axis < 3; ++axis)
            *(u16*)(vertex + 4 + axis * 2) = *(u16*)(anchors + i * 8 + axis * 2);
        vertex += 24;
    }
    u8* face = HeapAlloc((u32)*(s16*)(owner + 6) * 88u, 0);
    if (face == NULL) {
        *(u32*)(owner + 0x14) = 0;
        HeapFree(NULL);
        HeapFree((void*)(uintptr_t)*(u32*)(owner + 0x1C));
        HeapFree(vertices);
        return;
    }
    *(u32*)(owner + 0x20) = (u32)(uintptr_t)face;
    s32 pageX = (s16)(((s16)texX / 64) * 64);
    s32 pageY = (s16)(((s16)texY / 256) * 256);
    u16 tpage = GetTPage(0, 1, pageX, pageY);
    u16 clut = GetClut((s16)clutX, (s16)clutY);
    s32 originU = ((s16)texX - pageX) * 4;
    s32 originV = (u16)texY - pageY;
    s32 step = (s16)FieldTrackDivide((s16)width, *(s16*)(owner + 4) - 1);
    u32 base = 0;
    for (s32 i = 0; i < *(s16*)(owner + 4) - 1; ++i) {
        u32 pairs = counts[i] < counts[i + 1] ? counts[i] : counts[i + 1];
        u32 vstep = (u16)FieldTrackDivide((s16)height, (s32)pairs);
        u32 left = (u32)originU + (u32)i * (u32)step;
        u32 right = left + (u32)step;
        for (u32 j = 0; j < pairs; ++j) {
            u32 at = base + j, next = at + counts[i] + 1;
            for (unsigned tri = 0; tri < 2; ++tri) {
                *(u16*)face = (u16)(tri ? next : at);
                *(u16*)(face + 2) = (u16)(tri ? next + 1 : next);
                *(u16*)(face + 4) = (u16)(at + 1);
                for (unsigned buffer = 0; buffer < 2; ++buffer) {
                    u8* p = face + buffer * 40;
                    SetPolyGT3((POLY_GT3*)(p + 8));
                    *(u16*)(p + 0x22) = tpage;
                    *(u16*)(p + 0x16) = clut;
                    p[0x14] = (u8)(tri ? right : left);
                    p[0x15] = (u8)((u32)originV + vstep * j);
                    p[0x20] = (u8)right;
                    p[0x21] = (u8)((u32)originV + vstep * (j + tri));
                    p[0x2C] = (u8)left;
                    p[0x2D] = (u8)((u32)originV + vstep * (j + 1));
                }
                face += 88;
            }
        }
        base += counts[i] + 1u;
    }
    owner[0xC] = (u8)r; owner[0xD] = (u8)g; owner[0xE] = (u8)b;
    owner[0xF] = (u8)backR; owner[0x10] = (u8)backG; owner[0x11] = (u8)backB;
    *(s16*)(owner + 0xA) = (s16)constraints;
    if ((s16)constraints > 0) {
        u16* entries = HeapAlloc((u32)(s32)(s16)constraints * 16u, 0);
        if (entries == NULL) *(u16*)(owner + 0xA) = 0;
        *(u32*)(owner + 0x18) = (u32)(uintptr_t)entries;
        for (s32 i = 0; i < *(s16*)(owner + 0xA); ++i)
            for (unsigned j = 0; j < 8; ++j) entries[i * 8 + j] = 0;
    } else {
        *(u32*)(owner + 0x18) = 0;
    }
}

extern long VectorNormal(VECTOR*, VECTOR*);
extern s32 D_80050100;

static s32 FieldGeometryDistance(s32* delta)
{
    u32 square = 0;
    for (unsigned axis = 0; axis < 3; ++axis)
        square += (u32)delta[axis] * (u32)delta[axis];
    return SquareRoot0((s32)square);
}

static void FieldGeometryNormalize(s32* delta, s32 distance)
{
    for (unsigned axis = 0; axis < 3; ++axis)
        delta[axis] = distance == 0 ? 0 : FieldTrackDivide((s32)((u32)delta[axis] << 8), distance);
}

static void FieldGeometryNext(u8* vertex, s32* delta, s32 scale)
{
    for (unsigned axis = 0; axis < 3; ++axis) {
        s32 product = (s32)((u32)(s32)*(s16*)vertex * (u32)delta[axis] * (u32)scale);
        *(u16*)(vertex + 0x1C + axis * 2) = (u16)(*(u16*)(vertex + 4 + axis * 2) + product / 0x100000);
    }
}

static void FieldGeometryLoadVector(const u8* vector, unsigned reg)
{
    u32 xy, z;
    memcpy(&xy, vector, 4);
    memcpy(&z, vector + 4, 4);
    MTC2(xy, reg);
    MTC2(z, reg + 1);
}

/* Complete retail [801E22F8,801E3438), SHA-256
 * 93902b9a120ae364d9084a84c16cc079481eb89acc84a62ec13e8f94f3e43445.
 * Linked geometry constraints, normal averaging, and double-buffered packets. */
void func_801E22F8(u8* owner, s16* offset, MATRIX* matrix, u32* ot,
                  s32 buffer, s32 scale, s32 height)
{
    if (*(u32*)(owner + 0x14) == 0) return;
    u8 code = ((u8*)(uintptr_t)*(u32*)(owner + 0x20))[0xF];
    for (s32 chain = 0; chain < *(s16*)(owner + 4); ++chain) {
        u8* vertex = (u8*)(uintptr_t)((u32*)(uintptr_t)*(u32*)(owner + 0x1C))[chain];
        while (*(s16*)vertex != 0) {
            s32 delta[3];
            for (unsigned axis = 0; axis < 3; ++axis)
                delta[axis] = *(s16*)(vertex + 0x1C + axis * 2) - *(s16*)(vertex + 4 + axis * 2) + offset[axis];
            delta[1] += *(s16*)(vertex + 2);
            FieldGeometryNormalize(delta, FieldGeometryDistance(delta));
            FieldGeometryNext(vertex, delta, scale);
            if ((s16)height < *(s16*)(vertex + 0x1E)) *(u16*)(vertex + 0x1E) = (u16)height;
            u8* constraint = (u8*)(uintptr_t)*(u32*)(owner + 0x18);
            for (s32 index = 0; index < *(s16*)(owner + 0xA); ++index, constraint += 0x10) {
                for (unsigned axis = 0; axis < 3; ++axis)
                    delta[axis] = *(s16*)(vertex + 0x1C + axis * 2) - *(s16*)(constraint + 8 + axis * 2);
                s32 distance = FieldGeometryDistance(delta);
                s32 product = (s32)((u32)(s32)*(s16*)(constraint + 0xE) * (u32)scale);
                s32 radius = (s16)(product / 4096);
                if (distance < radius) {
                    for (unsigned axis = 0; axis < 3; ++axis) {
                        s32 component = distance == 0 ? 0 : FieldTrackDivide((s32)((u32)radius * (u32)delta[axis]), distance);
                        delta[axis] = (s32)((u32)component + (u32)(s32)*(s16*)(constraint + 8 + axis * 2)
                            - (u32)(s32)*(s16*)(vertex + 4 + axis * 2));
                    }
                    FieldGeometryNormalize(delta, FieldGeometryDistance(delta));
                    FieldGeometryNext(vertex, delta, scale);
                }
            }
            vertex += 0x18;
        }
    }
    u8* vertices = (u8*)(uintptr_t)*(u32*)(uintptr_t)*(u32*)(owner + 0x1C);
    for (s32 index = 0; index < *(s16*)(owner + 8); ++index) {
        u8* vertex = vertices + index * 0x18;
        *(u16*)(vertex + 0xA) = 0;
        *(u32*)(vertex + 0xC) = *(u32*)(vertex + 0x10) = *(u32*)(vertex + 0x14) = 0;
    }
    u8* face = (u8*)(uintptr_t)*(u32*)(owner + 0x20);
    for (s32 index = 0; index < *(s16*)(owner + 6); ++index, face += 0x58) {
        VECTOR edgeA = {0}, edgeB = {0}, normal = {0}, normalized = {0};
        s32* a = &edgeA.vx;
        s32* b = &edgeB.vx;
        for (unsigned axis = 0; axis < 3; ++axis) {
            s32 first = *(s16*)(vertices + *(s16*)face * 0x18 + 4 + axis * 2);
            a[axis] = first - *(s16*)(vertices + *(s16*)(face + 2) * 0x18 + 4 + axis * 2);
            b[axis] = first - *(s16*)(vertices + *(s16*)(face + 4) * 0x18 + 4 + axis * 2);
        }
        CTC2(a[0], 0); CTC2(a[1], 2); CTC2(a[2], 4);
        MTC2(b[2], 11); MTC2(b[0], 9); MTC2(b[1], 10);
        doCOP2(0x0170000C);
        normal.vx = (s32)MFC2(25) / 8;
        normal.vy = (s32)MFC2(26) / 8;
        normal.vz = (s32)MFC2(27) / 8;
        VectorNormal(&normal, &normalized);
        for (unsigned corner = 0; corner < 3; ++corner) {
            u8* vertex = vertices + *(s16*)(face + corner * 2) * 0x18;
            *(u32*)(vertex + 0xC) += (u32)normalized.vx;
            *(u32*)(vertex + 0x10) += (u32)normalized.vy;
            *(u32*)(vertex + 0x14) += (u32)normalized.vz;
            ++*(u16*)(vertex + 0xA);
        }
    }
    vertices = (u8*)(uintptr_t)*(u32*)(uintptr_t)*(u32*)(owner + 0x1C);
    for (s32 index = 0; index < *(s16*)(owner + 8); ++index) {
        u8* vertex = vertices + index * 0x18;
        s32 x = FieldTrackDivide(*(s32*)(vertex + 0xC), *(s16*)(vertex + 0xA));
        s32 y = FieldTrackDivide(*(s32*)(vertex + 0x10), *(s16*)(vertex + 0xA));
        s32 z = FieldTrackDivide(*(s32*)(vertex + 0x14), *(s16*)(vertex + 0xA));
        *(s32*)(vertex + 0xC) = x; *(s32*)(vertex + 0x10) = y; *(s32*)(vertex + 0x14) = z;
    }
    SetRotMatrix(matrix); SetTransMatrix(matrix);
    face = (u8*)(uintptr_t)*(u32*)(owner + 0x20);
    vertices = (u8*)(uintptr_t)*(u32*)(uintptr_t)*(u32*)(owner + 0x1C);
    u8* packet = (u8*)(uintptr_t)((u32)(uintptr_t)face + 8u + (u32)buffer * 40u);
    for (s32 index = 0; index < *(s16*)(owner + 6); ++index) {
        for (unsigned corner = 0; corner < 3; ++corner)
            FieldGeometryLoadVector(vertices + *(s16*)(face + corner * 2) * 0x18 + 4, corner * 2);
        doCOP2(0x00280030);
        if (CFC2(31) & 0x40000u) continue;
        doCOP2(0x01400006);
        s32 orientation = (s32)MFC2(24);
        *(u32*)(packet + 8) = MFC2(12); *(u32*)(packet + 0x14) = MFC2(13); *(u32*)(packet + 0x20) = MFC2(14);
        doCOP2(0x0158002D);
        s32 depth = (s32)MFC2(7) >> ((u32)D_80050100 & 31);
        unsigned colorOffset = orientation < 0 ? 0xC : 0xF;
        u32 color = (u32)owner[colorOffset] | (u32)owner[colorOffset + 1] << 8
            | (u32)owner[colorOffset + 2] << 16 | (u32)code << 24;
        for (unsigned corner = 0; corner < 3; ++corner) {
            u8* vertex = vertices + *(s16*)(face + corner * 2) * 0x18;
            SVECTOR normal = {0};
            s16* components = &normal.vx;
            for (unsigned axis = 0; axis < 3; ++axis) {
                u16 value = *(u16*)(vertex + 0xC + axis * 4);
                components[axis] = (s16)(orientation < 0 ? value : (u16)(0u - value));
            }
            FieldGeometryLoadVector((u8*)&normal, 0);
            if (corner == 0) MTC2(color, 6);
            doCOP2(0x0108041B);
            *(u32*)(packet + 4 + corner * 12) = MFC2(22);
        }
        u32* bucket = (u32*)(uintptr_t)((u32)(uintptr_t)ot + (u32)depth * 4u);
        *(u32*)packet = (*(u32*)packet & 0xFF000000u) | (*bucket & 0xFFFFFFu);
        *bucket = (*bucket & 0xFF000000u) | ((u32)(uintptr_t)packet & 0xFFFFFFu);
        face += 0x58; packet += 0x58;
    }
}

/* Retail [801E3438,801E34BC), SHA-256
 * 40f036f5def72dd96f6fdf1357db7bc47c35e26655b956a79bdbba78fd3794a5.
 * Only +14 is cleared; the other retained words are not independent owners
 * once that guard is zero. Reload pointers after each heap boundary. */
void func_801E3438(u8* owner)
{
    if (*(u32*)(owner + 0x14) == 0) return;
    HeapFree((void*)(uintptr_t)*(u32*)(owner + 0x14));
    HeapFree((void*)(uintptr_t)*(u32*)(uintptr_t)*(u32*)(owner + 0x1C));
    HeapFree((void*)(uintptr_t)*(u32*)(owner + 0x1C));
    HeapFree((void*)(uintptr_t)*(u32*)(owner + 0x20));
    if (*(u32*)(owner + 0x18) != 0)
        HeapFree((void*)(uintptr_t)*(u32*)(owner + 0x18));
    *(u32*)(owner + 0x14) = 0;
}

/* Retail [801E7378,801E738C), SHA-256
 * f23ba5e9842ef3fc55b69891235391be5c40c4ba98b7453588d0f63bd3576666. */
void func_801E7378(s32 value)
{
    D_801E85CC = (u32)value & 1u;
}

void func_801E738C(s32 arg0) {
    /* Retail 801E738C..742C:
     * 5287e22284932db7e892ba7743bf4f2de91317d355b3392ec5aa96a27395f728. */
    D_801E8640 = 0;
    D_801E869C = 0;
    func_801DF5F4(D_801E86A8, arg0);
    func_801E0064(D_801E86A0, 16);
    for (s32 i = 9; i >= 0; --i) D_801E8670[i] = 0;
    for (s32 i = 7; i >= 0; --i) D_801E85F4[i][0] = 0;
    D_801E8648[13] = 0; /* 8662: second record's active halfword */
    D_801E8648[3] = 0;  /* 864E: first record's active halfword */

    /* Host-only pointer-width bookkeeping for the remaining slot adapter.
     * This is not a substitute for the packed registry reset above. */
    memset(s_ptrTab, 0, sizeof(s_ptrTab));
}

/* Retail [801E8480,801E8510), SHA-256
 * e7efebddd8c4f81a23326459143c016db776a73c057c3e65cb2b5a29872defd3. */
s32 func_801E8480(s32 slot)
{
    u32* entry = (u32*)(uintptr_t)((u32)(uintptr_t)D_801E8670 + (u32)slot * 4u);
    u8* object = (u8*)(uintptr_t)*entry;
    u8* node;
    s32 factor, component, scale;
    if (object == NULL) return 0;
    node = (u8*)(uintptr_t)*(u32*)(object + 4);
    if (*(u16*)(object + 0x4A) & 8) {
        factor = *(s16*)(object + 0x26);
        component = *(s16*)(node + 0x4C);
    } else {
        factor = *(s16*)(object + 0x28);
        component = *(s16*)(node + 0x50);
    }
    object = (u8*)(uintptr_t)*entry;
    scale = (s32)((u32)*(s16*)(object + 0x1C) * (u32)component) >> 12;
    return (s32)((u32)factor * (u32)scale) >> 12;
}

/* Retail [801E8510,801E8590), SHA-256
 * 0764da3a5f31586321a67a62de035066a3c1694ce28e81ecec92308e09dc580d.
 * No allocation-null recovery exists in this body. */
void func_801E8510(u8* parent)
{
    if (parent[0x10C] != 0) {
        u8* entries = HeapAlloc((u32)parent[0x10C] * 0x70u, 0);
        for (s32 i = 0; i < parent[0x10C]; ++i) {
            *(s16*)(entries + i * 0x70) = -1;
            *(u32*)(entries + i * 0x70 + 8) = 0;
        }
        *(u32*)(parent + 0x110) = (u32)(uintptr_t)entries;
    }
}

static void OvlyFinalizeModelCopy(u8* obj, u8* copy, u32 flags) {
    /* Retail 801E7C40..801E7CDC. The temporary copy was allocated from
     * the high end. Restore offsets before moving it into a low allocation;
     * preserve packet owners while replacing the model pointer table. */
    if (flags & 2) {
        *(u32*)(obj + 0xA8) = 0;
        return;
    }
    (void)func_8002C644(copy);
    (void)func_8002C4BC(copy);
    u32 size = (u32)(uintptr_t)HeapGetNextBlockHeader((HeapBlock*)copy);
    u8* relocated = HeapAlloc(size, 0);
    memcpy(relocated, copy, size);
    HeapFree(copy);
    /* 801DCE18(registry,0) frees only its pointer array, leaving model
     * packet buffers alive. OvlyFreeOwnedPtrTab also frees those packets. */
    OvlyPtrTab* tab = (OvlyPtrTab*)(uintptr_t)*(u32*)obj;
    if (tab != NULL && tab->ptrs != NULL) {
        HeapFree(tab->ptrs);
        tab->ptrs = NULL;
    }
    OvlyBuildPtrTab(relocated, (OvlyPtrTab*)(uintptr_t)*(u32*)obj);
    *(u32*)(obj + 0xA8) = (u32)(uintptr_t)relocated;
}

void func_801E742C(s32 slot, s32 flags, void* pModelArc, void* pTexArc,
                  s32 x, s32 y, s32 z, s32 w, s16* vec) {
    u8* obj;
    u8* tex = (u8*)pTexArc;
    u8* modelArc = (u8*)pModelArc;
    u8* slice = NULL;
    u8* sliceEnd = NULL;
    u8* copy = NULL;
    u16* list = NULL;
    u8* nodes;
    s32 copySize;
    s32 texMode;
    OvlyPtrTab* tab;

    HeapChangeCurrentUser(HEAP_USER_MASA, NULL);
    if (slot < 0 || slot >= OVLY_SLOT_MAX) {
        return;
    }
    if (D_801E8670[slot] != 0) {
        return;
    }

    if (OvlyDiag()) {
        fprintf(stderr, "[obj-ovly] 742C slot=%d flags=%d model=%p tex=%p xy=(%d,%d) zw=(%d,%d) vec=%p\n",
               (int)slot, (int)flags, pModelArc, pTexArc,
               (int)x, (int)y, (int)z, (int)w, (void*)vec);
        if (modelArc != NULL) {
            fprintf(stderr, "[obj-ovly]  model[0..3]=%08x %08x %08x %08x\n",
                   ((u32*)modelArc)[0], ((u32*)modelArc)[1],
                   ((u32*)modelArc)[2], ((u32*)modelArc)[3]);
        }
        if (tex != NULL) {
            fprintf(stderr, "[obj-ovly]  tex[0..3]=%08x %08x %08x %08x\n",
                   ((u32*)tex)[0], ((u32*)tex)[1],
                   ((u32*)tex)[2], ((u32*)tex)[3]);
        }
    }

    /* Relocate archive pointer tables (ResolveArchiveEntryPointers). */
    if (!(flags & 4) && tex != NULL) {
        OvlyResolve(tex);
        if (*(u32*)(tex + 0x10) != 0) {
            OvlyResolve((void*)(uintptr_t)*(u32*)(tex + 0x10));
        }
    }
    if (!(flags & 1) && modelArc != NULL) {
        OvlyResolve(modelArc);
        if (*(u32*)(modelArc + 8) != 0) {
            OvlyResolve((void*)(uintptr_t)*(u32*)(modelArc + 8));
        }
        if (*(u32*)(modelArc + 4) != 0) {
            u8* sub = (u8*)(uintptr_t)*(u32*)(modelArc + 4);
            OvlyResolve(sub);
            if (*(u32*)(sub + 4) != 0) {
                OvlyResolve((void*)(uintptr_t)*(u32*)(sub + 4));
            }
        }
    }

    obj = (u8*)HeapAlloc(OVLY_OBJ_SIZE, 0);
    if (obj == NULL) {
        return;
    }
    memset(obj, 0, OVLY_OBJ_SIZE);
    D_801E8670[slot] = (u32)(uintptr_t)obj;

    /* Retail E75F4 loads object flags before the constructor chooses setup. */
    if (tex != NULL && *(u32*)(tex + 0x10) != 0) {
        u8* hdr = (u8*)(uintptr_t)*(u32*)(tex + 0x10);
        u8* s2 = (u8*)(uintptr_t)*(u32*)(hdr + 4);
        if (s2 != NULL) {
            *(u16*)(obj + 0x1C) = *(u16*)(s2 + 8);
            *(u16*)(obj + 0x24) = *(u16*)(s2 + 2);
            *(u16*)(obj + 0x26) = *(u16*)(s2 + 4);
            *(u16*)(obj + 0x28) = *(u16*)(s2 + 6);
            obj[0x2A] = s2[0xA];
            *(u16*)(obj + 0x4A) = *(u16*)(s2 + 0xC);
        }
    }

    /* Texture upload from tex archive +4. Field calls this with flags==0. */
    texMode = (flags & 0x40) ? 0 : 1;
    if (!(flags & 1) && tex != NULL && *(u32*)(tex + 4) != 0) {
        void* image = (void*)(uintptr_t)*(u32*)(tex + 4);
        if (OvlyDiag()) {
            fprintf(stderr, "[obj-ovly] DDE4 image=%p mode=%d xy=(%d,%d)\n",
                   image, texMode, (int)x, (int)y);
        }
        func_8002DDE4(image, texMode, (s16)x, (s16)y, texMode, (s16)z, (s16)w);
    }

    /* Copy the model slice out of the tex archive: *(tex+8) .. *(tex+0xC).
     * func_80077AB4 HeapFree's D_8005A450 (pTexArc) after this returns. */
    if (tex != NULL) {
        slice = (u8*)(uintptr_t)*(u32*)(tex + 8);
        sliceEnd = (u8*)(uintptr_t)*(u32*)(tex + 0xC);
        if (slice != NULL && sliceEnd > slice &&
            OvlySanitySize((s32)(sliceEnd - slice))) {
            copySize = (s32)(sliceEnd - slice);
            copy = (u8*)HeapAlloc((u32)copySize, 1);
            if (copy != NULL) {
                memcpy(copy, slice, (size_t)copySize);
            }
        }
    }
    if (copy == NULL && modelArc != NULL) {
        /* Fallback: treat the model archive itself as the blob. */
        copy = modelArc;
        copySize = 0;
        if (OvlyDiag()) {
            fprintf(stderr, "[obj-ovly] fallback model-arc as blob %p\n", (void*)modelArc);
        }
    }
    if (copy == NULL) {
        if (OvlyDiag()) {
            fprintf(stderr, "[obj-ovly] no model blob slot=%d\n", (int)slot);
        }
        return;
    }

    tab = OvlyBuildPtrTab(copy, &s_ptrTab[slot]);
    /* Retail 801E75A8 keeps *(pTexArc + 0xC) in s0; 801E775C/77B8
     * passes that skeleton list to 801DC2D0. modelArc+8 is a pointer table. */
    list = (u16*)sliceEnd;
    nodes = OvlyBuildNodes(tab, list, (flags & 0x40) ? 0 : 2,
                           !(flags & 0x40) && !(*(u16*)(obj + 0x4A) & 4),
                           (s16)x, (s16)y, (s16)z, (s16)w);
    *(u32*)(obj + 4) = (u32)(uintptr_t)nodes;
    *(u32*)obj = (u32)(uintptr_t)tab;
    *(u8*)(obj + 0x34) = 1;
    *(u8*)(obj + 0x20) = (u8)slot;
    if (nodes != NULL && vec != NULL) {
        *(s32*)(nodes + 0x5C) = (s32)vec[0];
        *(s32*)(nodes + 0x60) = (s32)vec[1];
        *(s32*)(nodes + 0x64) = (s32)vec[2];
        OvlyIdentMatrix((MATRIX*)(nodes + 0xC),
                        (s32)vec[0], (s32)vec[1], (s32)vec[2]);
    }
    {
        u8* clipTab = NULL;
        u32 clipAux = 0;
        if (modelArc != NULL && *(u32*)(modelArc + 4) != 0) {
            u8* sub = (u8*)(uintptr_t)*(u32*)(modelArc + 4);
            clipTab = sub + 8;
            clipAux = *(u32*)(sub + 4);
        }
        OvlyClipInit(obj, clipTab, clipAux);
        if (!(flags & 0x40)) {
            OvlyClipBind(obj, obj, 0);
        }
    }
    if (copy != modelArc) {
        OvlyFinalizeModelCopy(obj, copy, (u32)flags);
    }
    if (OvlyDiag()) {
        fprintf(stderr, "[obj-ovly] 742C done slot=%d obj=%p nodes=%p groups=%d vec=(%d,%d,%d)\n",
               (int)slot, (void*)obj, (void*)nodes,
               tab != NULL ? (int)tab->count : 0,
               vec ? (int)vec[0] : 0, vec ? (int)vec[1] : 0,
               vec ? (int)vec[2] : 0);
    }
}

/* Retail [801E7D14,801E7FD4), archive 6B9, SHA-256
 * 2e84156fa452d4cfcb0c90c98f11650394193b0bd2403d10da52c4950140d419.
 * Field object frame. misc2.c func_8007520C passes the world-to-screen
 * matrix (g_Scene+0xD4), the field light-direction matrix (&D_800B221C),
 * the render context OT (+0xCC), the render context index and 1. Six
 * retail passes, in order:
 *   1. tick budget: D_801E8640 += 1 + arg4, clamped to 6, consumed two per
 *      tick; the sway phase D_801E869C advances 56 per tick and
 *      D_801E8698 = (rsin(phase) + 0x1000) / 800 + 4 (signed, truncating);
 *   2. every registered object: snapshot the root translation into
 *      obj+0x11C/120/124, then func_801E36BC with the tick count and 1;
 *   3. every registered object with an attachment selector (obj+0x5C != FF):
 *      func_801E37D0;
 *   4. func_801E1880 over the slot table, then SetColorMatrix(D_801E8644)
 *      (= &D_800B223C, the field's light-colour table);
 *   5. every registered object: the per-frame root delta into
 *      obj+0x128/12C/130 (misc8.c func_80081F80 reads it as the actor step),
 *      then func_801DCEC8(obj, view, light, 1, 1, ot, ctx);
 *   6. func_801E0398 over the auxiliary sprite pool.
 * Retail neither rebinds actor positions here (misc2.c FieldUpdateObjectActor
 * wrote the actor into the root earlier in the frame) nor gates on node
 * counts or visibility in the outer passes. */
void func_801E7D14(void* sceneData, void* arg1, void* prim, s32 renderContextIndex,
                   s32 arg4)
{
    MATRIX* view = (MATRIX*)sceneData;
    MATRIX* light = (MATRIX*)arg1;
    u32* ot = (u32*)prim;
    s32 ticks = 0;
    s32 slot;

    D_801E8640 = (u32)((s32)D_801E8640 + 1 + arg4);
    if ((s32)D_801E8640 >= 7) {
        D_801E8640 = 6;
    }
    while ((s32)D_801E8640 >= 2) {
        D_801E8640 = (u32)((s32)D_801E8640 - 2);
        ticks++;
    }
    D_801E869C = (u16)(D_801E869C + ticks * 56);
    D_801E8698 = (s16)(((rsin((s16)D_801E869C) + 0x1000) / 800) + 4);

    for (slot = 0; slot < OVLY_SLOT_MAX; slot++) {
        u8* obj = (u8*)(uintptr_t)D_801E8670[slot];
        u8* root;
        if (obj == NULL) {
            continue;
        }
        root = (u8*)(uintptr_t)*(u32*)(obj + 4);
        *(s32*)(obj + 0x11C) = *(s32*)(root + 0x5C);
        *(s32*)(obj + 0x120) = *(s32*)(root + 0x60);
        *(s32*)(obj + 0x124) = *(s32*)(root + 0x64);
        func_801E36BC(obj, D_801E86A8, ticks, renderContextIndex, 1);
    }
    for (slot = 0; slot < OVLY_SLOT_MAX; slot++) {
        u8* obj = (u8*)(uintptr_t)D_801E8670[slot];
        if (obj != NULL && obj[0x5C] < 0xFF) {
            func_801E37D0(obj);
        }
    }
    func_801E1880(D_801E8670);
    SetColorMatrix((MATRIX*)D_801E8644);
    for (slot = 0; slot < OVLY_SLOT_MAX; slot++) {
        u8* obj = (u8*)(uintptr_t)D_801E8670[slot];
        u8* root;
        if (obj == NULL) {
            continue;
        }
        root = (u8*)(uintptr_t)*(u32*)(obj + 4);
        *(s32*)(obj + 0x128) = *(s32*)(obj + 0x11C) - *(s32*)(root + 0x5C);
        *(s32*)(obj + 0x12C) = *(s32*)(obj + 0x120) - *(s32*)(root + 0x60);
        *(s32*)(obj + 0x130) = *(s32*)(obj + 0x124) - *(s32*)(root + 0x64);
        func_801DCEC8(obj, view, light, 1, 1, ot, renderContextIndex);
    }
    func_801E0398(D_801E86A0, view, ticks, ot, renderContextIndex);
}

/* Retail [801E8394,801E8430), SHA-256
 * 52f01bdbf34869a3e7cf2cd2704d57bee4a84dfcd0376e1ee33ef71eff93ee1f.
 * Cross-object clip binding. Retail clears byte 35 before its null check;
 * it neither clamps the slot nor substitutes a missing registry entry. */
void func_801E8394(u8* source, s32 slot, s32 mask, s32 animation)
{
    u32 index = (u16)slot;
    u32* entry = (u32*)((u8*)D_801E8670 + index * 4u);
    u8* object = (u8*)(uintptr_t)*entry;
    D_801E86B0 = (s16)slot;
    D_801E863C = (s16)mask;
    object[0x35] = 0;
    object = (u8*)(uintptr_t)*entry;
    if (object != NULL && index != (u32)func_801E67F8()) {
        object = (u8*)(uintptr_t)*entry;
        func_801E35D0(object, source, D_801E86A8, animation);
    }
}

/* Retail [801E8330,801E8394), SHA-256
 * 338db1f71d21b868721a7760e971ef414800b2eb601df4ccc0f34f8f28da8763.
 * No slot clamp, pre-written phase or provisional interpreter fallback. */
void func_801E8330(s32 slot, s32 mask, s32 anim)
{
    u32* entry = (u32*)((u8*)D_801E8670 + (u32)(u16)slot * 4u);
    u8* obj = (u8*)(uintptr_t)*entry;
    D_801E86B0 = (s16)slot;
    D_801E863C = (s16)mask;
    obj[0x35] = 0;
    obj = (u8*)(uintptr_t)*entry;
    if (obj != NULL) func_801E35D0(obj, obj, D_801E86A8, anim);
}
