#ifndef _XENO_3D_MODEL_H
#define _XENO_3D_MODEL_H

#include "psyq/libgpu.h"
#include "psyq/libgte.h"

/* Model-file and packet semantics recovered by the upstream model work.  The
 * numeric func_* identities remain authoritative in this tree; these names
 * document the protocol without forcing a temp2.c translation-unit split. */
#define MODEL_FLAG_HAS_LIGHT_DATA 0x1
#define PRIM_HAS_TEX_DATA 0x0
#define PRIM_NO_TEX_DATA 0x1

typedef struct {
    /* 0x0 */ u16 texpageAndClut;
    /* 0x2 */ u8 unk2;
    /* 0x3 */ u8 code;
} PrimitiveHeader;

typedef struct {
    /* 0x0 */ u8 r;
    /* 0x1 */ u8 g;
    /* 0x2 */ u8 b;
    /* 0x3 */ u8 code;
} UntexturedPrimitive;

typedef struct {
    /* 0x0 */ u8 u2;
    /* 0x1 */ u8 v2;
    /* 0x2 */ u8 unk2;
    /* 0x3 */ u8 code;
    /* 0x4 */ u8 u0;
    /* 0x5 */ u8 v0;
    /* 0x6 */ u8 u1;
    /* 0x7 */ u8 v1;
} TexturedTrianglePrimitive;

typedef struct {
    /* 0x0 */ u8 r;
    /* 0x1 */ u8 g;
    /* 0x2 */ u8 b;
    /* 0x3 */ u8 code;
    /* 0x4 */ u8 u0;
    /* 0x5 */ u8 v0;
    /* 0x6 */ u8 u1;
    /* 0x7 */ u8 v1;
    /* 0x8 */ u8 u2;
    /* 0x9 */ u8 v2;
    /* 0xA */ u8 u3;
    /* 0xB */ u8 v3;
} TexturedQuadPrimitive;

typedef struct {
    u32 unk0;
    u32 lightDataSize;
    u32 modelPacketSize;
} ModelPacketInfo;

typedef struct {
    /* 0x0  */ u_short flags;
    /* 0x2  */ u8 unk2[0x2];
    /* 0x4  */ u_short polyCount;
    /* 0x6  */ u16 unk6;
    /* 0x8  */ SVECTOR* pVertices;
    /* 0xC  */ u32 unkC;
    /* 0x10 */ u32 unk10;
    /* 0x14 */ CVECTOR* pColors;
    /* 0x18 */ void* pLightData;
    /* 0x1C */ u32* unk1C;
    /* 0x20 */ ModelPacketInfo unk20[2];
} ModelPart;

typedef struct {
    /* 0x0  */ int numModelParts;
    /* 0x4  */ u_int flags;
    /* 0x8  */ u8 unk8[0x8];
    /* 0x10 */ ModelPart modelParts[0];
} ModelFileHeader;

/* These structures describe serialized PSX model data.  Host pointers are
 * wider in XENO_PC_PORT, so retain the layout checks for the matching ABI and
 * avoid pretending the native pointer-bearing view is a serialized layout. */
#ifndef XENO_PC_PORT
typedef char xeno_model_assert_primitive_header[(sizeof(PrimitiveHeader) == 0x4) ? 1 : -1];
typedef char xeno_model_assert_untextured_primitive[(sizeof(UntexturedPrimitive) == 0x4) ? 1 : -1];
typedef char xeno_model_assert_textured_triangle[(sizeof(TexturedTrianglePrimitive) == 0x8) ? 1 : -1];
typedef char xeno_model_assert_textured_quad[(sizeof(TexturedQuadPrimitive) == 0xC) ? 1 : -1];
typedef char xeno_model_assert_packet_info[(sizeof(ModelPacketInfo) == 0xC) ? 1 : -1];
typedef char xeno_model_assert_model_part[(sizeof(ModelPart) == 0x38) ? 1 : -1];
typedef char xeno_model_assert_model_file_header[(sizeof(ModelFileHeader) == 0x10) ? 1 : -1];
#endif

#endif
