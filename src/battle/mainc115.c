/* func_800BC460: battle target-chain camera/radius computation.
 *
 * CDK-provenance body (BattleCdk preset, gcc-2.7.2-cdk-psx +
 * `--dont-expand-li`): 401/401 equal-address words, size 0x644, byte-identical
 * to disc/battle.bin[0x4C970:0x4CFB4]. The configured 2.7.2-psx cc1 emits only
 * 11/401 for the same source, so this TU must stay on the CDK preset.
 *
 * This is 32-bit legacy compiler source, NOT portable native C: signed
 * shifts/overflow and the SDK ABI are judged by the emitted MIPS code. */
#include "common.h"
#include "psyq/libgte.h"
extern void *memset(void *, int, unsigned);
extern MATRIX *RotMatrixZYX(SVECTOR *, MATRIX *);
extern long ReadGeomScreen(void);
extern void func_800BB844(MATRIX *, SVECTOR *, SVECTOR *, SVECTOR *);
#ifndef XENO_PC_PORT
extern u8 D_800C3688[];
#endif
#ifndef XENO_PC_PORT
extern u32 D_800C3678[];
#endif
#ifndef XENO_PC_PORT
extern s32 D_800C3CDC[];
#endif
#ifndef XENO_PC_PORT
extern SVECTOR D_800C3740[], D_800C3730;
#endif
extern struct { SVECTOR eye,target; } D_800D30A0;
typedef struct Position {
    s32 x,y,z;
    u8 rest[0x2a];
    u16 height;
} Position;
typedef struct BattleTargets {
    struct { u8 bytes[0x1c]; } rows[11];
    u8 gap[0x8c8c-11*0x1c];
    Position *positions[11];
} BattleTargets;
#ifndef XENO_PC_PORT
extern BattleTargets D_800C3EB0;
#endif

void func_800BC460(u32 mask) {
    VECTOR center;
    SVECTOR eye,target;
    MATRIX view;
    VECTOR offset;
    union {
        struct { SVECTOR point; DVECTOR screen; } projection;
        VECTOR result;
    } work;
    SVECTOR distance;
    long depth,flags;
    Position *position;
    u32 remaining;
    s32 i, maximum, count;
    s32 minx,maxx,minz,maxz,miny,maxy;
    memset(&center,0,16);
    maximum=0;
    D_800C3678[0]=mask;
    i=0;
    count=0;
    remaining=mask;
    for(;i!=11;i++,remaining=(u16)remaining>>1) {
        if((remaining&1) && !D_800C3EB0.rows[i].bytes[7]) {
            position=D_800C3EB0.positions[i];
            if(position) {
                count++;
                center.vx+=position->x>>1;
                center.vy+=position->y>>1;
                center.vz+=position->z>>1;
            }
        }
    }
    i=0;
    if(count) {
        center.vx=(center.vx/count)*2;
        center.vy=(center.vy/count)*2;
        center.vz=(center.vz/count)*2;
        maxx=minx=center.vx;
        maxz=minz=center.vz;
        maxy=miny=center.vy;
        remaining=mask;
        for(;i!=11;i++,remaining=(u16)remaining>>1) {
            if((remaining&1) && !D_800C3EB0.rows[i].bytes[7]) {
                position=D_800C3EB0.positions[i];
                if(position) {
                    if(maxx<position->x) maxx=position->x;
                    if(position->x<minx) minx=position->x;
                    if(maxz<position->z) maxz=position->z;
                    if(position->z<minz) minz=position->z;
                    if(maxy<position->y) maxy=position->y;
                    if(position->y<miny) miny=position->y;
                }
            }
        }
        center.vx=(minx+maxx)/2;
        center.vy=(maxy+miny)/2;
        center.vz=(minz+maxz)/2;
        center.vx>>=16; center.vy>>=16; center.vz>>=16;
        RotMatrixZYX(D_800C3740,&view);
        distance.vx=0; distance.vy=0;
        distance.vz=ReadGeomScreen()<<3;
        ApplyMatrix(&view,&distance,&offset);
        eye.vx=center.vx; eye.vy=center.vy; eye.vz=center.vz;
        target.vx=eye.vx; target.vy=eye.vy; target.vz=eye.vz;
        eye.vx-=offset.vx; eye.vy+=offset.vy; eye.vz-=offset.vz;
        func_800BB844(&view,&eye,&target,&D_800C3730);
        SetRotMatrix(&view); SetTransMatrix(&view);
        remaining=mask;
        for(i=0;i!=11;i++,remaining=(u16)remaining>>1) {
            if((remaining&1) && !D_800C3EB0.rows[i].bytes[7]) {
                position=D_800C3EB0.positions[i];
                if(position) {
                    work.projection.point.vx=position->x>>16;
                    work.projection.point.vy=position->y>>16;
                    work.projection.point.vz=position->z>>16;
                    RotTransPers(&work.projection.point,(long *)&work.projection.screen,&depth,&flags);
                    work.projection.screen.vx-=160; work.projection.screen.vy-=164;
                    work.projection.screen.vx<<=2; work.projection.screen.vy<<=2;
                    { int squared=work.projection.screen.vx*work.projection.screen.vx; squared+=work.projection.screen.vy*work.projection.screen.vy; if(maximum<squared) maximum=squared; }
                    if(D_800C3EB0.rows[i].bytes[8] && !D_800C3688[0]) {
                        work.projection.point.vy-=position->height;
                        RotTransPers(&work.projection.point,(long *)&work.projection.screen,&depth,&flags);
                        work.projection.screen.vx-=160; work.projection.screen.vy-=164;
                        work.projection.screen.vx<<=2; work.projection.screen.vy<<=2;
                        { int squared=work.projection.screen.vx*work.projection.screen.vx; squared+=work.projection.screen.vy*work.projection.screen.vy; if(maximum<squared) maximum=squared; }
                    }
                }
            }
        }
        maximum=SquareRoot0(maximum);
        {
            MATRIX rotation;
            VECTOR result;
            SVECTOR input;
            SVECTOR *destination;
        if(maximum<120) {
            RotMatrixZYX(D_800C3740,&rotation);
            input.vx=0; input.vy=0;
            input.vz=ReadGeomScreen()<<1;
            D_800C3CDC[0]=ReadGeomScreen()<<1;
            ApplyMatrix(&rotation,&input,&work.result);
            eye.vx=center.vx; eye.vy=center.vy; eye.vz=center.vz;
            target.vx=eye.vx; target.vy=eye.vy; target.vz=eye.vz;
            eye.vx-=work.result.vx; eye.vy+=work.result.vy; eye.vz-=work.result.vz;
            D_800D30A0.eye.vx=eye.vx; D_800D30A0.eye.vy=eye.vy; D_800D30A0.eye.vz=eye.vz;
            D_800D30A0.target.vx=target.vx;
            destination=&D_800D30A0.target;
            destination->vy=target.vy; destination->vz=target.vz;
        } else {
            MATRIX rotation;
            VECTOR unused_result;
            SVECTOR input;
            SVECTOR *destination;
            s32 scaled=(maximum<<14)/120;
            scaled=(scaled<<1)*ReadGeomScreen();
            scaled>>=14;
            D_800C3CDC[0]=scaled;
            RotMatrixZYX(D_800C3740,&rotation);
            input.vx=0; input.vy=0; input.vz=scaled;
            ApplyMatrix(&rotation,&input,&result);
            eye.vx=center.vx; eye.vy=center.vy; eye.vz=center.vz;
            target.vx=eye.vx; target.vy=eye.vy; target.vz=eye.vz;
            eye.vx-=result.vx; eye.vy+=result.vy; eye.vz-=result.vz;
            D_800D30A0.eye.vx=eye.vx; D_800D30A0.eye.vy=eye.vy; D_800D30A0.eye.vz=eye.vz;
            D_800D30A0.target.vx=target.vx;
            destination=&D_800D30A0.target;
            destination->vy=target.vy; destination->vz=target.vz;
        }
        }
    }
}
