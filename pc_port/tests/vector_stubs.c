/* Minimal stubs for OuterProduct0 and VectorNormal. */
#include <stdint.h>
#include "../src/gte_normalize_table.h"
#include <string.h>
typedef struct { int vx, vy, vz, pad; } VECTOR;
static int32_t GteClampS32(int64_t v) {
    if (v > INT64_C(0x7FFFFFFF))  return INT32_MAX;
    if (v < INT64_C(-0x80000000)) return INT32_MIN;
    return (int32_t)v;
}
static int16_t s_InvSqrtTable[XENO_GTE_NORM_TABLE_LEN];
__attribute__((constructor)) static void s_InvSqrtTable_Init(void)
{
    XenoGteNormTableFill(s_InvSqrtTable);
}
void OuterProduct0(VECTOR* v0, VECTOR* v1, VECTOR* out) {
    out->vx = v0->vy * v1->vz - v0->vz * v1->vy;
    out->vy = v0->vz * v1->vx - v0->vx * v1->vz;
    out->vz = v0->vx * v1->vy - v0->vy * v1->vx;
}
static int32_t VectorNormalWork(int32_t x, int32_t y, int32_t z,
                                int32_t* ox, int32_t* oy, int32_t* oz) {
    int32_t sx=(int16_t)x, sy=(int16_t)y, sz=(int16_t)z;
    uint32_t sq=(uint32_t)(sx*sx+sy*sy+sz*sz);
    if(!sq){*ox=0;*oy=0;*oz=0;return 0;}
    int lzc=__builtin_clz(sq), lze=lzc&~1, sh=(31-lze)>>1, idx;
    if(lze>=24) idx=(int)(sq<<(lze-24)); else idx=(int)(sq>>(24-lze));
    idx-=0x40; if(idx<0)idx=0;
    if(idx>=(int)(sizeof(s_InvSqrtTable)/sizeof(s_InvSqrtTable[0])))
        idx=(sizeof(s_InvSqrtTable)/sizeof(s_InvSqrtTable[0]))-1;
    int32_t sc=s_InvSqrtTable[idx];
    *ox=GteClampS32(((int64_t)sc*sx)>>sh);
    *oy=GteClampS32(((int64_t)sc*sy)>>sh);
    *oz=GteClampS32(((int64_t)sc*sz)>>sh);
    return (int32_t)sq;
}
long VectorNormal(VECTOR* v0, VECTOR* v1) {
    int32_t x,y,z;
    int32_t sq=VectorNormalWork(v0->vx,v0->vy,v0->vz,&x,&y,&z);
    v1->vx=x; v1->vy=y; v1->vz=z;
    return sq;
}
