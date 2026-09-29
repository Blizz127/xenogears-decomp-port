#define main object_state_main
#include "actor_object_state_retail_test.c"
#undef main
FieldActor* volatile g_FieldActors;
s32 D_800ADBFC,D_8004F37C,D_80050100;
u8 D_80059598,D_80059599,D_8005959A;
GTERegisters gteRegs;
int FieldGetCameraDirection(void) {return 0;}
static void forbidden(void) {fputs("OBJECT LOOP FAIL entered sprite transform/draw path\n",stderr);exit(1);}
void MTC2(unsigned int value,int reg) {(void)value;(void)reg;forbidden();}
unsigned MFC2(int reg) {(void)reg;forbidden();return 0;}
unsigned CFC2(int reg) {(void)reg;forbidden();return 0;}
void CTC2(unsigned int value,int reg) {(void)value;(void)reg;forbidden();}
int doCOP2(int op) {(void)op;forbidden();return 0;}
void gte_OuterProduct12(VECTOR* v0,VECTOR* v1,VECTOR* v2) {(void)v0;(void)v1;(void)v2;forbidden();}
SVECTOR* gte_ApplyMatrixSV(MATRIX* m,SVECTOR* v0,SVECTOR* v1) {(void)m;(void)v0;forbidden();return v1;}
int gte_RotTransPers(SVECTOR* v0,int* sxy,long* p,long* flag,long* otz) {(void)v0;(void)sxy;(void)p;(void)flag;(void)otz;forbidden();return 0;}
long VectorNormalSS(SVECTOR* v0,SVECTOR* v1) {(void)v0;(void)v1;forbidden();return 0;}
long RotAverage4(SVECTOR* v0,SVECTOR* v1,SVECTOR* v2,SVECTOR* v3,long* sxy0,long* sxy1,long* sxy2,long* sxy3,long* p,long* flag) {(void)v0;(void)v1;(void)v2;(void)v3;(void)sxy0;(void)sxy1;(void)sxy2;(void)sxy3;(void)p;(void)flag;forbidden();return 0;}
void ReadGeomOffset(long* ofx,long* ofy) {(void)ofx;(void)ofy;forbidden();}
SVECTOR* ApplyMatrixSV(MATRIX* m,SVECTOR* v,SVECTOR* o) {(void)m;(void)v;forbidden();return o;}
VECTOR* ApplyMatrixLV(MATRIX* m,VECTOR* v,VECTOR* o) {(void)m;(void)v;forbidden();return o;}
void SetRotMatrix(MATRIX* m) {(void)m;forbidden();}
void SetTransMatrix(MATRIX* m) {(void)m;forbidden();}
MATRIX* ScaleMatrix(MATRIX* m,VECTOR* v) {(void)v;forbidden();return m;}
int RotTransPers(SVECTOR* v,int* xy,long* p,long* f) {(void)v;(void)xy;(void)p;(void)f;forbidden();return 0;}
void SpriteSetColor(void* p,u8 r,u8 g,u8 b) {(void)p;(void)r;(void)g;(void)b;forbidden();}
void func_8001E298(void* p,void* ot) {(void)p;(void)ot;forbidden();}
void func_8001E2F8(void* p,void* ot,s16 y) {(void)p;(void)ot;(void)y;forbidden();}
void func_8001E368(void* p,void* ot,s16 y) {(void)p;(void)ot;(void)y;forbidden();}
static int bridge(void* u,PcPortMipsCpu* cpu,u32 t) {
    (void)u;if(t!=0x8009a514)return 0;
    cpu->gpr[2]=0;return 1;
}
int main(void) {
    FILE* f=fopen("disc/field.bin","rb");assert(f);
    assert(fread(ram+0x6faf0,1,260862,f)==260862);assert(!fclose(f));
    unsigned cases=0;
    for(unsigned skip=0;skip<2;++skip)for(unsigned active=0;active<8;++active)
    for(unsigned invisible=0;invisible<8;++invisible) {
        memset(&fixture,0xa5,sizeof(fixture));memset(&g_Scene,0,sizeof(g_Scene));
        for(unsigned j=0;j<3;++j) {
            fixture.loopActors[j][0x58/4]=((active>>j)&1)?0x40:0;
            if((invisible>>j)&1)fixture.loopActors[j][0x58/4]|=0x20;
            fixture.loopActors[j][0x4c/4]=(u32)(uintptr_t)fixture.loopStates[j];
            fixture.loopActors[j][1]=0; /* Object actors never dereference the sprite. */
            fixture.loopStates[j][1]=0x2000|0x200;
            *(s16*)((u8*)fixture.loopStates[j]+0x22)=100+j;
            *(s16*)((u8*)fixture.loopStates[j]+0x26)=200+j;
            *(s16*)((u8*)fixture.loopStates[j]+0x2a)=300+j;
        }
        for(unsigned j=0;j<10;++j) {
            fixture.objects[j][1]=(u32)(uintptr_t)fixture.nodes[j];
            D_801E8670[j]=skip?0:(u32)(uintptr_t)fixture.objects[j];
            /* Block base is retail 0x800B2078; D_800B220C scales at +0x194. */
            *(u32*)(g_FieldBss_800B20A8+0x194+j*4)=0x1000+j;
        }
        D_8004F380=skip;D_800ADBFC=3;g_FieldActors=(FieldActor*)fixture.loopActors;
        memcpy(ram+0x1e8670,D_801E8670,40);
        memcpy(ram+0xb2078,g_FieldBss_800B20A8,0x424);
        memcpy(ram+0xaf990,&g_Scene,sizeof(g_Scene));
        assert(!wr(NULL,0x8004f380,4,skip));assert(!wr(NULL,0x800adbfc,4,3));
        assert(!wr(NULL,0x800afb10,4,(u32)(uintptr_t)g_FieldActors));
        for(unsigned frame=0;frame<2;++frame) {
            typeof(fixture) initial=fixture;
            PcPortMipsBus bus={.read=rd,.write=wr,.bridge=bridge};PcPortMipsCpu cpu;PcPortMipsCpuInit(&cpu,&bus);
            cpu.gpr[29]=0x801ff000;cpu.gpr[31]=0xfffffffcu;
            if(PcPortMipsRun(&cpu,0x80075b44,0xfffffffcu,20000)!=PC_PORT_MIPS_HALTED) {
                fprintf(stderr,"OBJECT LOOP FAIL oracle %s\n",cpu.error);return 1;
            }
            expected=fixture;fixture=initial;
            func_80075B44(NULL,0);
            if(memcmp(&fixture,&expected,sizeof(fixture))) {
                fprintf(stderr,"OBJECT LOOP FAIL skip=%u active=%x hidden=%x frame=%u\n",skip,active,invisible,frame);return 1;
            }
            ++cases;
        }
    }
    printf("OBJECT LOOP PASS %u cases: full actor loop/packed slots/reset/inactive/hidden/no sprite fallback\n",cases);
}
