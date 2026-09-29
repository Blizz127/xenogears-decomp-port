/* Reuse the low-address fixture and projection recorder, not its expected
 * clipping logic. Execute the shadow leaf's own retail instructions. */
#define main split_fixture_main
#define SPRITE_TEST_SHADOW_PROJECTION
#include "sprite_split_retail_test.c"
#undef main
MATRIX D_8004FBB8;
static struct MatrixEvent {u32 kind;u8 matrix[32];s32 vector[3];} matrixEvents[4],expectedMatrices[4];
static unsigned nMatrices;
static void matrixRecord(u32 kind,MATRIX* m,s32 x,s32 y,s32 z) {
    assert(nMatrices<4);struct MatrixEvent* e=&matrixEvents[nMatrices++];
    e->kind=kind;memcpy(e->matrix,m,32);e->vector[0]=x;e->vector[1]=y;e->vector[2]=z;
}
MATRIX* ScaleMatrixL(MATRIX* m,VECTOR* v) {
    matrixRecord(0x8004974c,m,v->vx,v->vy,v->vz);
    /* Deliberately distinctive boundary output, not a substitute GTE. */
    for(unsigned row=0;row<3;++row)for(unsigned col=0;col<3;++col)
        m->m[row][col]=(s16)((u32)m->m[row][col]+(u32)v->vx+row*17+col);
    return m;
}
VECTOR* ApplyMatrix(MATRIX* m,SVECTOR* v,VECTOR* out) {
    matrixRecord(0x80049cec,m,v->vx,v->vy,v->vz);
    out->vx=(s32)((u32)(s32)v->vx+0x7ffffffeu);
    out->vy=(s32)((u32)(s32)v->vy+0x80000001u);
    out->vz=v->vz+99;return out;
}
void SetRotMatrix(MATRIX* m) {matrixRecord(0x80049efc,m,0,0,0);}
void SetTransMatrix(MATRIX* m) {matrixRecord(0x80049f8c,m,0,0,0);}
static int shadowBridge(void* u,PcPortMipsCpu* cpu,u32 t) {
    u32* a=cpu->gpr+4;MATRIX m;SVECTOR v;VECTOR scale,out;
    if(t==0x8004a7bc)return bridge(u,cpu,t);
    if(t!=0x8004974c && t!=0x80049cec && t!=0x80049efc && t!=0x80049f8c)return 0;
    memcpy(&m,address(a[0],32),32);
    switch(t) {
    case 0x8004974c:
        memcpy(&scale,address(a[1],12),12);ScaleMatrixL(&m,&scale);
        memcpy(address(a[0],32),&m,32);cpu->gpr[2]=a[0];break;
    case 0x80049cec:
        memcpy(&v,address(a[1],8),8);ApplyMatrix(&m,&v,&out);
        memcpy(address(a[2],12),&out,12);cpu->gpr[2]=a[2];break;
    case 0x80049efc:SetRotMatrix(&m);break;
    case 0x80049f8c:SetTransMatrix(&m);break;
    }
    return 1;
}
int main(void) {
    FILE* file=fopen("disc/SLUS_006.64","rb");assert(file);
    assert(!fseek(file,0x800+0xe9bc,SEEK_SET));
    assert(fread(ram+0x1e9bc,1,0x4ac,file)==0x4ac);
    assert(!fseek(file,0x800+0x3faf8,SEEK_SET));
    assert(fread(ram+0x4faf8,1,16,file)==16);assert(!fclose(file));
    assert(!memcmp(ram+0x4faf8,s_DirectionMask8004FAF8,16));
    static const unsigned counts[]={0,1,8,63},shifts[]={0,1,15,31};
    static const s16 scales[]={-32768,-3,0,1,4096,32767};
    static const u8 masks[]={0,1,0x55,0xaa,0xef,0xf7,0xff};
    unsigned cases=0;
    for(unsigned variant=0;variant<8;++variant)for(unsigned ci=0;ci<4;++ci)
    for(unsigned si=0;si<4;++si)for(unsigned sc=0;sc<6;++sc)
    for(unsigned mi=0;mi<7;++mi)for(unsigned capacity=0;capacity<3;++capacity) {
        unsigned count=counts[ci];projectionMode=variant&1;
        memset(&fixture,0xa5,sizeof(fixture));
        fixture.sprite[8]=(u32)(uintptr_t)fixture.base;
        fixture.base[12]=(u32)(uintptr_t)fixture.prims;
        fixture.sprite[16]=(count<<2)|(shifts[si]<<8);
        fixture.sprite[15]=((variant&3)<<3)|((u32)masks[mi]<<8);
        u8* sprite=(u8*)fixture.sprite;
        *(s16*)(sprite+2)=-32768;*(s16*)(sprite+6)=321;*(s16*)(sprite+10)=32767;
        *(s16*)(sprite+0x84)=-1234;*(s16*)(sprite+0x2c)=scales[sc];
        fixture.ot=0xcd123456;
        for(unsigned i=0;i<63;++i) {
            u8* p=(u8*)fixture.prims+i*24;
            *(s16*)p=(i&1)?-32768:7;*(s16*)(p+2)=i*11-16;
            p[4]=(i&1)?0:255;p[5]=250;p[6]=(i&1)?16:17;p[7]=32;
            p[8]=(variant&4)?0xec:0;p[9]=(variant&4)?0xd8:0;
            *(u32*)(p+20)=((i/2)&7)|((i&3)<<4);
        }
        memset(&D_8004FBB8,0x3b,32);
        D_8004FBB8.t[0]=12345;D_8004FBB8.t[1]=-12345;D_8004FBB8.t[2]=0x7fffffff;
        memcpy(ram+0x4fbb8,&D_8004FBB8,32);
        memset(s_ShadowQuad8004FAD8,0x27,32);memcpy(ram+0x4fad8,s_ShadowQuad8004FAD8,32);
        initial=fixture;u32 work=(u32)(uintptr_t)fixture.packets;
        u32 end=work+count*40+(capacity==0?0:capacity==1?1:(u32)-1);
        assert(!wr(NULL,0x80059580,4,work));assert(!wr(NULL,0x80059534,4,end));
        nCalls=nMatrices=0;memset(calls,0,sizeof(calls));memset(matrixEvents,0,sizeof(matrixEvents));
        PcPortMipsBus bus={.read=rd,.write=wr,.bridge=shadowBridge};
        PcPortMipsCpu cpu;PcPortMipsCpuInit(&cpu,&bus);
        cpu.gpr[4]=(u32)(uintptr_t)fixture.sprite;cpu.gpr[5]=(u32)(uintptr_t)&fixture.ot;
        cpu.gpr[29]=0x801ff000;cpu.gpr[31]=0xfffffffcu;
        if(PcPortMipsRun(&cpu,0x8001e9bc,0xfffffffcu,100000)!=PC_PORT_MIPS_HALTED) {
            fprintf(stderr,"SPRITE SHADOW FAIL oracle %s\n",cpu.error);return 1;
        }
        expected=fixture;memcpy(expectedCalls,calls,sizeof(calls));
        memcpy(expectedMatrices,matrixEvents,sizeof(matrixEvents));
        unsigned expectedCount=nCalls;assert(nMatrices==4);
        fixture=initial;g_GfxCurWorkBuffer=(void*)(uintptr_t)work;g_GfxCurWorkBufferEnd=(void*)(uintptr_t)end;
        nCalls=nMatrices=0;memset(calls,0,sizeof(calls));memset(matrixEvents,0,sizeof(matrixEvents));
        func_8001E9BC(fixture.sprite,&fixture.ot);
        if(memcmp(&fixture,&expected,sizeof(fixture)) || memcmp(calls,expectedCalls,sizeof(calls)) ||
           memcmp(matrixEvents,expectedMatrices,sizeof(matrixEvents)) || nMatrices!=4 ||
           memcmp(&D_8004FBB8,ram+0x4fbb8,32) || memcmp(s_ShadowQuad8004FAD8,ram+0x4fad8,32) ||
           nCalls!=expectedCount || (uintptr_t)g_GfxCurWorkBuffer!=read32(0x80059580)) {
            fprintf(stderr,"SPRITE SHADOW FAIL variant=%u count=%u shift=%u scale=%d mask=%x capacity=%u\n",
                    variant,count,shifts[si],scales[sc],masks[mi],capacity);return 1;
        }
        ++cases;
    }
    printf("SPRITE SHADOW PASS %u cases: matrix boundaries/quad scratch/packets/OT/cursor\n",cases);
}
