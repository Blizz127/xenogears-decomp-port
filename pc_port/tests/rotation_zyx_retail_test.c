/* Retail 8004ABBC..8004AE48, including original trig-table reads, versus
 * the production native owner. Neither path should execute a GTE command. */
#include "battle_mips_adapter.h"
#include <libgte.h>
#include <psx/gtereg.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
extern MATRIX *RotMatrixZYX(SVECTOR *,MATRIX *);
static uint8_t ram[0x200000];
static unsigned cases,stores;
typedef struct Fixture {
    uint32_t before[4]; MATRIX matrix; uint32_t gap[4];
    SVECTOR angles; uint32_t after[2];
} Fixture;
_Static_assert(offsetof(Fixture,matrix)==16 && offsetof(Fixture,angles)==64 &&
               sizeof(Fixture)==80 && sizeof(MATRIX)==32,"fixture ABI");
static int rd(void *p,uint32_t a,unsigned w,uint32_t *v) {
    (void)p;
    if(a<0x80000000u || a>0x80200000u-w) return -1;
    a-=0x80000000u; *v=0;
    for(unsigned i=0;i<w;i++) *v|=(uint32_t)ram[a+i]<<(i*8);
    return 0;
}
static int wr(void *p,uint32_t a,unsigned w,uint32_t v) {
    (void)p;
    /* Retail stores precisely the nine rotation halfwords, in its own order. */
    if(w!=2 || a<0x80180010u || a>0x80180020u || (a&1)) return -1;
    a-=0x80000000u;
    for(unsigned i=0;i<w;i++) ram[a+i]=(uint8_t)(v>>(i*8));
    stores++; return 0;
}
static void check(int ok,const char *what) {
    if(!ok) { fprintf(stderr,"ROT ZYX FAIL case=%u %s\n",cases,what); exit(1); }
}
static void run(SVECTOR angles) {
    Fixture native,expected;
    memset(&native,0xa5,sizeof(native)); native.angles=angles;
    memcpy(ram+0x180000,&native,sizeof(native));
    memset(&gteRegs,0x5a,sizeof(gteRegs));
    GTERegisters before=gteRegs;
    PcPortMipsBus bus={.read=rd,.write=wr};
    PcPortMipsCpu cpu; PcPortMipsCpuInit(&cpu,&bus);
    cpu.gpr[4]=0x80180040; cpu.gpr[5]=0x80180010;
    cpu.gpr[29]=0x801ff000; cpu.gpr[31]=0x801ffffc;
    for(unsigned i=16;i<=23;i++) cpu.gpr[i]=0x51ba0000u+i;
    stores=0;
    int rc=PcPortMipsRun(&cpu,0x8004abbc,0x801ffffc,300);
    if(rc) fprintf(stderr,"retail error %s\n",cpu.error);
    check(rc==0,"retail execution");
    check(stores==9 && cpu.gpr[2]==0x80180010,"retail stores/return");
    check(cpu.gpr[29]==0x801ff000 && cpu.gpr[31]==0x801ffffc,"SP/RA");
    for(unsigned i=16;i<=23;i++) check(cpu.gpr[i]==0x51ba0000u+i,"callee registers");
    memcpy(&expected,ram+0x180000,sizeof(expected));
    check(RotMatrixZYX(&native.angles,&native.matrix)==&native.matrix,"native return");
    if(memcmp(&native,&expected,sizeof(native))) {
        fprintf(stderr,"angles=(%d,%d,%d)\n",angles.vx,angles.vy,angles.vz);
        for(unsigned r=0;r<3;r++) for(unsigned c=0;c<3;c++)
            if(native.matrix.m[r][c]!=expected.matrix.m[r][c])
                fprintf(stderr,"m[%u][%u] retail=%d native=%d\n",r,c,expected.matrix.m[r][c],native.matrix.m[r][c]);
        check(0,"matrix/input/guards");
    }
    check(!memcmp(&before,&gteRegs,sizeof(before)),"GTE unchanged");
    cases++;
}
int main(void) {
    FILE *f=fopen("disc/SLUS_006.64","rb"); assert(f);
    assert(!fseek(f,0x800,SEEK_SET));
    assert(fread(ram+0x10000,1,0x49800,f)==0x49800); fclose(f);
    run((SVECTOR){113,271,509,0x1234});
    const int16_t peers[][2]={{0,0},{113,-271},{-1024,2048},{4095,4096}};
    for(unsigned n=0;n<65536;n++) for(unsigned axis=0;axis<3;axis++)
    for(unsigned p=0;p<4;p++) {
        SVECTOR v;
        v.vx=axis==0?(int16_t)n:peers[p][0];
        v.vy=axis==1?(int16_t)n:(axis==0?peers[p][0]:peers[p][1]);
        v.vz=axis==2?(int16_t)n:peers[p][1]; v.pad=(int16_t)0xbcde;
        run(v);
    }
    MATRIX m; SVECTOR zero={0,0,0,0}; memset(&m,0xa5,sizeof(m));
    RotMatrixZYX(&zero,&m);
    for(unsigned r=0;r<3;r++) for(unsigned c=0;c<3;c++)
        check(m.m[r][c]==(r==c?4096:0),"identity witness");
    check(cases==786433,"fixture count");
    printf("ROT ZYX native/retail PASS %u cases; original trig table, no GTE commands\n",cases);
}
