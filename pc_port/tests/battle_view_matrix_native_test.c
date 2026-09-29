/* Real retail BB844 and SDK instructions versus native BB844 and SDK owners.
 * Both use PsyCross GTE: this is not independent PS1 hardware validation. */
#include "battle_mips_adapter.h"
#include <libgte.h>
#include <psx/gtereg.h>
#include <inline_c.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

extern void func_800BB844(MATRIX *, SVECTOR *, SVECTOR *, SVECTOR *);
extern int matrixLevel;
extern MATRIX stack[];
extern MATRIX *currentMatrix;
static uint8_t ram[0x200000];
static uint32_t seed;
static unsigned cases;
#ifdef XBT_VIEW_ALIAS
enum { ALIAS_LAYOUTS=4 };
#else
enum { ALIAS_LAYOUTS=1 };
#endif
typedef struct NativeImage {
    uint32_t prefix[4];
    MATRIX matrix;
    uint32_t gap0[4];
    SVECTOR eye;
    uint32_t gap1[2];
    SVECTOR target;
    uint32_t gap2[2];
    SVECTOR up;
    uint32_t suffix[6];
} NativeImage;
_Static_assert(sizeof(NativeImage)==128 && offsetof(NativeImage,matrix)==16 &&
    offsetof(NativeImage,eye)==64 && offsetof(NativeImage,target)==80 &&
    offsetof(NativeImage,up)==96,"typed fixture layout");
enum { DATA=0x180000, STACK=0x1ff000, HALT=0x1ffffc };
static uint32_t word(unsigned a) {
    return (uint32_t)ram[a] | (uint32_t)ram[a+1]<<8 |
           (uint32_t)ram[a+2]<<16 | (uint32_t)ram[a+3]<<24;
}
static void put(unsigned a, unsigned w, uint32_t v) {
    for (unsigned i=0;i<w;i++) ram[a+i]=(uint8_t)(v>>(i*8));
}
static int rd(void *p,uint32_t a,unsigned w,uint32_t *v) {
    (void)p;
    if (a<0x80000000u || a>0x80200000u-w) return -1;
    a-=0x80000000u; *v=0;
    for (unsigned i=0;i<w;i++) *v|=(uint32_t)ram[a+i]<<(i*8);
    return 0;
}
static int wr(void *p,uint32_t a,unsigned w,uint32_t v) {
    (void)p;
    if (a<0x80000000u || a>0x80200000u-w) return -1;
    put(a-0x80000000u,w,v); return 0;
}
static uint32_t cr(void *p,int control,unsigned r) {
    (void)p; return control?CFC2(r):MFC2(r);
}
static void cw(void *p,int control,unsigned r,uint32_t v) {
    (void)p; if(control) CTC2(v,r); else MTC2(v,r);
}
static int op(void *p,uint32_t ins) { (void)p; doCOP2(ins); return 0; }
static uint32_t next(void) { seed=seed*1664525u+1013904223u; return seed; }
static void reset_gte(void) {
    memset(&gteRegs,0,sizeof(gteRegs));
    for(unsigned i=0;i<32;i++) { CTC2(next(),i); MTC2(next(),i); }
}
static void check(int condition,const char *what) {
    if(!condition) { fprintf(stderr,"VIEW MATRIX FAIL case=%u %s\n",cases,what); exit(1); }
}
static void run(const SVECTOR input[3],unsigned depth,unsigned alias) {
    /* Guarded local region; alias modes also cover the late eye load after
     * the rotation-row stores. No callback or SDK routine is simulated. */
    struct { uint8_t bytes[128]; } initial, expected;
    NativeImage native;
    unsigned eye=alias==1?16:64, target=alias==2?16:80, up=alias==3?16:96;
    memset(initial.bytes,0xa5,sizeof(initial));
    memcpy(initial.bytes+eye,&input[0],8);
    memcpy(initial.bytes+target,&input[1],8);
    memcpy(initial.bytes+up,&input[2],8);
    memcpy(ram+DATA,initial.bytes,sizeof(initial));
    memset(ram+STACK-0x100,0x5a,0x100);
    put(0x56d2c,4,depth*32);
    memset(ram+0x56d30,0x96,640);
    seed=0x921bd781u+cases; reset_gte();
    GTERegisters start=gteRegs;
    PcPortMipsBus bus={.read=rd,.write=wr,.cop2_read=cr,.cop2_write=cw,.cop2_command=op};
    PcPortMipsCpu cpu; PcPortMipsCpuInit(&cpu,&bus);
    cpu.gpr[4]=0x80000000u+DATA+16; cpu.gpr[5]=0x80000000u+DATA+eye;
    cpu.gpr[6]=0x80000000u+DATA+target; cpu.gpr[7]=0x80000000u+DATA+up;
    cpu.gpr[29]=0x80000000u+STACK; cpu.gpr[31]=0x80000000u+HALT;
    for(unsigned i=16;i<=23;i++) cpu.gpr[i]=0x71350000u+i;
    int rc=PcPortMipsRun(&cpu,0x800bb844u,0x80000000u+HALT,2000);
    if(rc) fprintf(stderr,"retail error: %s\n",cpu.error);
    check(rc==0,"retail execution");
    check(cpu.gpr[29]==0x80000000u+STACK && cpu.gpr[31]==0x80000000u+HALT,"SP/RA");
    for(unsigned i=16;i<=23;i++) check(cpu.gpr[i]==0x71350000u+i,"callee registers");
    check(word(0x56d2c)==depth*32,"retail matrix stack balance");
    memcpy(expected.bytes,ram+DATA,sizeof(expected));
    GTERegisters end=gteRegs;
    memcpy(&native,initial.bytes,sizeof(native));
    gteRegs=start; matrixLevel=(int)depth; currentMatrix=stack+depth;
    memset(stack,0x96,640);
    SVECTOR *native_eye=&native.eye, *native_target=&native.target, *native_up=&native.up;
#ifdef XBT_VIEW_ALIAS
    /* Overlapping SDK objects require the explicit GCC/Clang no-strict-aliasing
     * test build. Default builds use only correctly typed disjoint members. */
    if(alias==1) native_eye=(SVECTOR *)&native.matrix;
    if(alias==2) native_target=(SVECTOR *)&native.matrix;
    if(alias==3) native_up=(SVECTOR *)&native.matrix;
#else
    check(alias==0,"strict typed fixture must be disjoint");
#endif
    func_800BB844(&native.matrix,native_eye,native_target,native_up);
    /* Observation-only controls: output and stack depth remain intact for
     * the GTE/saved-stack controls, so their own assertions must detect them. */
#if XBT_OBSERVATION_CONTROL == 1
    ((uint8_t *)&gteRegs.CP2D)[0]^=1;
#elif XBT_OBSERVATION_CONTROL == 2
    ((uint8_t *)&gteRegs.CP2C)[0]^=1;
#elif XBT_OBSERVATION_CONTROL == 3
    ((uint8_t *)stack)[0]^=1;
#elif XBT_OBSERVATION_CONTROL == 4
    native.prefix[0]^=1;
#elif XBT_OBSERVATION_CONTROL == 5
    native.eye.pad^=1;
#endif
    check(!memcmp(&native,expected.bytes,sizeof(native)),"matrix/input/guards");
    check(matrixLevel==(int)depth && currentMatrix==stack+depth,"native matrix stack balance");
    check(!memcmp(stack,ram+0x56d30,640),"matrix stack saved bytes");
    check(!memcmp(&gteRegs.CP2D,&end.CP2D,sizeof(end.CP2D)),"GTE data state");
    check(!memcmp(&gteRegs.CP2C,&end.CP2C,sizeof(end.CP2C)),"GTE control state");
    cases++;
}
int main(void) {
    _Static_assert(sizeof(MATRIX)==32 && sizeof(VECTOR)==16 && sizeof(SVECTOR)==8,"SDK ABI");
    FILE *f=fopen("disc/SLUS_006.64","rb"); assert(f);
    assert(!fseek(f,0x800,SEEK_SET)); assert(fread(ram+0x10000,1,0x49800,f)==0x49800); fclose(f);
    f=fopen("disc/battle.bin","rb"); assert(f);
    assert(!fseek(f,0x4bd54,SEEK_SET)); assert(fread(ram+0xbb844,1,0x190,f)==0x190); fclose(f);
    const unsigned depths[]={0,1,19};
    for(unsigned n=0;n<4096;n++) for(unsigned d=0;d<3;d++) for(unsigned alias=0;alias<ALIAS_LAYOUTS;alias++) {
        SVECTOR v[3]; seed=n+71;
        for(unsigned i=0;i<3;i++) {
            v[i].vx=(int)(next()%8192)-4096;
            v[i].vy=(int)(next()%8192)-4096;
            v[i].vz=(int)(next()%8192)-4096; v[i].pad=(short)0xbcde;
        }
        if(n==0) memset(v,0,sizeof(v));
        run(v,depths[d],alias);
    }
    for(unsigned n=0;n<65536;n++) {
        SVECTOR v[3]={{(short)n,4,-5,0},{0,0,0,0},{0,4096,0,0}};
        run(v,0,0);
    }
    /* Independent axis-aligned witness, beyond pairwise agreement. */
    SVECTOR axis[3]={{0,0,-4096,0},{0,0,0,0},{0,4096,0,0}};
    run(axis,0,0);
    MATRIX m; memset(&m,0xa5,sizeof(m));
    func_800BB844(&m,&axis[0],&axis[1],&axis[2]);
    for(unsigned row=0;row<3;row++) for(unsigned col=0;col<3;col++)
        check(m.m[row][col]==(row==col?4096:0),"axis-aligned rotation witness");
    check(m.t[0]==0 && m.t[1]==0 && m.t[2]==4096,"axis-aligned translation witness");
    check(cases==4096*3*ALIAS_LAYOUTS+65536+1,"fixture count");
    printf("VIEW MATRIX native/retail PASS %u cases; real SDK, shared PsyCross GTE\n",cases);
}
