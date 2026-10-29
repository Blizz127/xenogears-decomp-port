#ifndef _XENO_FIELD_MAIN_H
#define _XENO_FIELD_MAIN_H

#include "psyq/libgpu.h"
#include "psyq/libgte.h"

#define setRGB(obj, _color) obj.r0 = obj.g0 = obj.b0 = _color

#define SCREEN_WIDTH 320U
#define SCREEN_HEIGHT 224U

#define FIELD_OT_MAX_SIZE 4096

// FieldScene.unk48 flags
#define FIELD_SCENE_INTERPOLATING_SCREEN_Z 0x10

typedef struct {
    // There could potentially be more fields to the struct here

    /* 0x0   */ MATRIX viewMatrix;
    /* 0x20  */ MATRIX camRotationMatrix;
    /* 0x40  */ SVECTOR camRotation;
    /* 0x48  */ s32 unk48; // Flags related to random encounters?
    /* 0x4C  */ undefined8 unk4C[0x8];
    /* 0x54  */ SVECTOR sceneAngle;
    /* 0x5C  */ undefined8 unk5C[0x8];
    /* 0x64  */ u_char dollySet;
    /* 0x65  */ u_char dollyStop;
    /* 0x66  */ undefined16 _pad66;
    /* 0x68  */ u_int sceneScrZ;
    /* 0x6C  */ u_short sceneDIP;
    /* 0x6E  */ u_short sceneScale;
    /* 0x70  */ undefined8 unk70[0xC];
    /* 0x7C  */ u_int unk7C;
    /* 0x80  */ undefined8 unk71[0x44];
    /* 0xC4  */ SVECTOR worldRotation;
    /* 0xCC  */ SVECTOR worldTranslation;
    /* 0xD4  */ MATRIX worldToScreenMatrix;
    /* 0xF4  */ MATRIX unkF4;
    /* 0x114 */ MATRIX worldRotationMatrix;
} FieldScene; // Size: 0x134

typedef struct {
    /* 0x0  */ s16 x0;
    /* 0x2  */ s16 y0; // Height?
    /* 0x4  */ s16 z0;
    /* 0x6  */ s16 x1;
    /* 0x8  */ s16 y1; // Unused?
    /* 0xA  */ s16 z1;
    /* 0xC  */ s16 x2;
    /* 0xE  */ s16 y2; // Unused?
    /* 0x10 */ s16 z2;
    /* 0x12 */ s16 x3;
    /* 0x14 */ s16 y3; // Unused?
    /* 0x16 */ s16 z3;
} FieldTriggerZone; // Size: 0x18

extern FieldTriggerZone* g_pFieldTriggerZones;

extern FieldScene g_Scene;
extern int g_WorldScale;

/* Model-resource overlay helper loaded from archive 0x6B9 at 0x801DC000. */
void func_801E72CC(MATRIX* dst, MATRIX* unused, s32 selector, s32 index);


typedef struct {
    DRAWENV drawEnvs[2];
    DISPENV dispEnv;
    u_long ot1[0x1000];
    int unk1;
    u_long ot2[0x1000];
    int unk2;
    u_long ot3[0x8];
} RenderContext;

/* The field's third ordering table (8 tags; DrawOTag starts at tag 7).
 * Retail code reaches it both as ctx->ot3 and as the raw byte offset
 * ctx+0x80D4 (fade tiles, text boxes, FieldAddPrimitives' link into tag 7,
 * the field frame's DrawOTag(ctx+0x80F0)) -- one list on the PSX.  On the
 * LP64 port u_long is 8 bytes, so the struct member moves away from +0x80D4
 * and the two spellings would name two different lists: prims linked through
 * ctx->ot3 (zoom-fade crossfade overlay, starfield dissolve, map sprites)
 * would never reach the frame the retail-offset DrawOTag walks, and the
 * FieldDisplay-drawn frames would lose the fade tile.  Every ot3 user goes
 * through these so both builds address the single retail list (4-byte tags
 * at +0x80D4 on the port, FieldClearOTagR4). */
#ifdef XENO_PC_PORT
#define FIELD_CTX_OT3(ctx) ((u_long*)((u8*)(ctx) + 0x80D4))
#define FIELD_CTX_OT3_HEAD(ctx) ((u_long*)((u8*)(ctx) + 0x80F0))
#else
#define FIELD_CTX_OT3(ctx) ((ctx)->ot3)
#define FIELD_CTX_OT3_HEAD(ctx) ((ctx)->ot3 + 7)
#endif

typedef struct {
    SVECTOR vertices[4];
    POLY_FT4 polys[2];
} Quad;

typedef struct {
    short x;
    short y;
    short z;
    short w;
} SVec4;

typedef struct {
    // 0x800b11ac
    DR_MODE drawModes[10];
    
    // 0x800b1224
    RECT rects[10];

    // 0x800b1274
    POLY_FT4 polygons[10];

    // 0x800b1404
    SVECTOR vectors[20]; // Vectors for RotAverage4
} ZoomFadeEffect;

// Random assortment of control-related things
typedef struct {
    short isRandomEncountersEnabled;
    short unkAngle;
    short controllerBtnMask;
} FieldControl;

extern FieldControl g_FieldControl;

extern ZoomFadeEffect g_FieldZoomFadeEffect;

extern int D_80059198; // Has initialized render contexts?

#define SYSTEM_MODE_PC_HDD 0x0
#define SYSTEM_MODE_CD_ROM 0x1

extern int g_FieldSystemMode;

extern int g_FieldCurRenderContextIndex;
extern int g_FieldRenderContextUseOT2;
extern RenderContext* g_FieldCurRenderContext;
extern RenderContext g_FieldRenderContexts[2];

/* Retail gear transitions consume a party slot, not an actor index. */
void func_800AD4D4(s32 partySlot);
void func_800ACFD0(s32 partySlot);

#endif
