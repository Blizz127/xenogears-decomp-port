#include "battle_target_camera.h"
#include "battle_target_bounds.h"
#include <libgte.h>
#include <stddef.h>
#include <limits.h>

extern MATRIX *RotMatrixZYX(SVECTOR *,MATRIX *);
extern int32_t ReadGeomScreen(void);
extern void func_800BB844(MATRIX *,SVECTOR *,SVECTOR *,SVECTOR *);

static int32_t signed32(uint32_t v) {
    return v<=INT32_MAX?(int32_t)v:(int32_t)((int64_t)v-INT64_C(4294967296));
}
static int16_t short_bits(uint32_t v) {
    v&=0xffffu; return (int16_t)(v<0x8000u?(int32_t)v:(int32_t)v-65536);
}
static int read_short_vector(const PcPortMipsBus *m,uint32_t a,SVECTOR *v) {
    uint32_t x,y,z;
    if(m->read(m->opaque,a,2,&x) || m->read(m->opaque,a+2,2,&y) ||
       m->read(m->opaque,a+4,2,&z)) return -1;
    v->vx=short_bits(x); v->vy=short_bits(y); v->vz=short_bits(z); v->pad=0;
    return 0;
}
/* Return 1 for an eligible resolved handle, 0 for an excluded slot, -1 for
 * a resolver failure. The handle itself remains in the bus address domain. */
static int position_handle(const PcPortMipsBus *m,uint32_t mask,unsigned i,uint32_t *p) {
    uint32_t suppressed;
    if(!(mask&(1u<<i))) return 0;
    if(m->read(m->opaque,0x800c3eb7u+i*0x1cu,1,&suppressed)) return -1;
    if(suppressed) return 0;
    if(m->read(m->opaque,0x800ccb3cu+i*4,4,p)) return -1;
    if(!*p) return 0;
    if(*p>UINT32_MAX-10) return -1;
    return 1;
}
static void offset_eye(SVECTOR *eye,const SVECTOR *center,const VECTOR *offset) {
    eye->vx=short_bits((uint32_t)(uint16_t)center->vx-(uint32_t)offset->vx);
    eye->vy=short_bits((uint32_t)(uint16_t)center->vy+(uint32_t)offset->vy);
    eye->vz=short_bits((uint32_t)(uint16_t)center->vz-(uint32_t)offset->vz);
    eye->pad=0;
}

int PcPortBattleUpdateTargetCamera(const PcPortMipsBus *m,uint32_t mask) {
    PcPortBattleTargetPoint points[11]={0};
    int32_t positions[11][3];
    PcPortBattleTargetBounds bounds;
    MATRIX rotation={0};
    SVECTOR angles,up,center,eye,offset_input={0};
    VECTOR offset;
    uint32_t maximum=0;
    if(!m || !m->read || !m->write) return -1;
    if(m->write(m->opaque,0x800c3678u,4,mask)) return -1;
    /* No library calls intervene between retail's two center scans. Stable
     * passive memory permits using the existing tested snapshot arithmetic. */
    for(unsigned i=0;i<11;i++) {
        uint32_t p,value;
        int eligible=position_handle(m,mask,i,&p);
        if(eligible<0) return -1;
        if(!eligible) continue;
        for(unsigned axis=0;axis<3;axis++) {
            if(m->read(m->opaque,p+axis*4,4,&value)) return -1;
            positions[i][axis]=signed32(value);
        }
        points[i].position=positions[i];
    }
    if(PcPortBattleComputeTargetBounds(mask,points,&bounds)) return -1;
    if(!bounds.count) return 0;
    if(read_short_vector(m,0x800c3740u,&angles)) return -1;
    RotMatrixZYX(&angles,&rotation);
    offset_input.vz=short_bits((uint32_t)ReadGeomScreen()<<3);
    ApplyMatrix(&rotation,&offset_input,&offset);
    center.vx=short_bits((uint32_t)bounds.center[0]);
    center.vy=short_bits((uint32_t)bounds.center[1]);
    center.vz=short_bits((uint32_t)bounds.center[2]); center.pad=0;
    offset_eye(&eye,&center,&offset);
    if(read_short_vector(m,0x800c3730u,&up)) return -1;
    func_800BB844(&rotation,&eye,&center,&up);
    SetRotMatrix(&rotation); SetTransMatrix(&rotation);
    for(unsigned i=0;i<11;i++) {
        uint32_t p,x,y,z,has_height,gate,height;
        int eligible=position_handle(m,mask,i,&p);
        SVECTOR point;
        int screen;
        long depth,flags;
        if(eligible<0) return -1;
        if(!eligible) continue;
        if(m->read(m->opaque,p+2,2,&x) || m->read(m->opaque,p+6,2,&y) ||
           m->read(m->opaque,p+10,2,&z)) return -1;
        point.vx=short_bits(x); point.vy=short_bits(y); point.vz=short_bits(z); point.pad=0;
        RotTransPers(&point,&screen,&depth,&flags);
        maximum=PcPortBattleAccumulateTargetRadius(maximum,(uint16_t)screen,(uint16_t)((uint32_t)screen>>16));
        if(m->read(m->opaque,0x800c3eb8u+i*0x1cu,1,&has_height)) return -1;
        if(!has_height) continue;
        if(m->read(m->opaque,0x800c3688u,1,&gate)) return -1;
        if(gate) continue;
        if(p>UINT32_MAX-0x36u || m->read(m->opaque,p+0x36u,2,&height)) return -1;
        point.vy=short_bits((uint32_t)(uint16_t)point.vy-height);
        RotTransPers(&point,&screen,&depth,&flags);
        maximum=PcPortBattleAccumulateTargetRadius(maximum,(uint16_t)screen,(uint16_t)((uint32_t)screen>>16));
    }
    int32_t radius=SquareRoot0(signed32(maximum));
    if(radius<120) {
        if(read_short_vector(m,0x800c3740u,&angles)) return -1;
        RotMatrixZYX(&angles,&rotation);
        offset_input.vz=short_bits((uint32_t)ReadGeomScreen()<<1);
        uint32_t distance=(uint32_t)ReadGeomScreen()<<1;
        if(m->write(m->opaque,0x800c3cdcu,4,distance)) return -1;
    } else {
        /* Retail signed /120 strength reduction, then low-word product and
         * arithmetic >>14. Preserve overflow in the product before shifting. */
        uint32_t scaled=(uint32_t)(signed32((uint32_t)radius<<14)/120);
        uint32_t product=(scaled<<1)*(uint32_t)ReadGeomScreen();
        int32_t distance=signed32((product>>14)|((0u-(product>>31))<<18));
        if(m->write(m->opaque,0x800c3cdcu,4,(uint32_t)distance)) return -1;
        if(read_short_vector(m,0x800c3740u,&angles)) return -1;
        RotMatrixZYX(&angles,&rotation);
        offset_input.vz=short_bits((uint32_t)distance);
    }
    ApplyMatrix(&rotation,&offset_input,&offset);
    offset_eye(&eye,&center,&offset);
    if(m->write(m->opaque,0x800d30a0u,2,(uint16_t)eye.vx) ||
       m->write(m->opaque,0x800d30a2u,2,(uint16_t)eye.vy) ||
       m->write(m->opaque,0x800d30a4u,2,(uint16_t)eye.vz) ||
       m->write(m->opaque,0x800d30a8u,2,(uint16_t)center.vx) ||
       m->write(m->opaque,0x800d30aau,2,(uint16_t)center.vy) ||
       m->write(m->opaque,0x800d30acu,2,(uint16_t)center.vz)) return -1;
    return 0;
}
