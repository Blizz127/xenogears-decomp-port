/* Fixed-width retail ABI fixture for an extracted helper, not PsyCross headers. */
#include <stdint.h>
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int16_t s16;
typedef int32_t s32;
typedef unsigned long u_long;
typedef struct { s16 m[3][3]; s32 t[3]; } MATRIX;
typedef struct { s32 vx,vy,vz,pad; } VECTOR;
typedef struct { s16 vx,vy,vz,pad; } SVECTOR;
extern MATRIX D_8004FBB8;
extern s32 g_GfxCurContext;
extern u_long *g_GfxCurOT;
void func_80022038(void*);
MATRIX *TransMatrix(MATRIX*,VECTOR*);
MATRIX *CompMatrix(MATRIX*,MATRIX*,MATRIX*);
void SetRotMatrix(MATRIX*);
void SetTransMatrix(MATRIX*);
void func_8002C700(void*,void*,u_long*,s32);
void func_80025718(u8*);
