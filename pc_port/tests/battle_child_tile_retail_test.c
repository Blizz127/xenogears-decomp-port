/* Exact SLUS type-9 oracle. Controlled mode substitutes only the three GTE
 * SDK boundaries; TYPE9_REAL_GTE executes their actual retail instructions
 * with PsyCross COP2. AddPrim is always actual retail code on the guest side,
 * and the native side links the production guest_prim_link.c. */
#include "battle_mips_adapter.h"
#include "psx_memory.h"
#include "guest_prim_link.h"
#include "psyq/libgte.h"
#ifdef TYPE9_REAL_GTE
#include "psx/gtereg.h"
extern unsigned MFC2(int), CFC2(int);
extern void MTC2(unsigned, int), CTC2(unsigned, int);
extern int doCOP2(int);
#endif
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t D_800C3664;
int32_t D_80050100;
MATRIX D_8004FBB8;
unsigned long *g_GfxCurOT;
void *g_GfxCurWorkBuffer, *g_GfxCurWorkBufferEnd;
extern void func_80025544(uint8_t *);
extern void func_80025224(void *, int);
void func_80025258(void *p) { (void)p; }
void func_80025710(void *p) { (void)p; }
void func_80025718(void *p) { (void)p; }
void func_8002541C(void *p) { (void)p; }
void func_800257F0(void *p) { (void)p; }
static void (*bound)(void *);
void WorkListSetTaskCallback(void *p, void (*f)(void *)) { (void)p; bound=f; }

#define TASK 0x80170000u
#define SPRITE 0x80170100u
#define WORK 0x80180000u
#define OT 0x80190000u
#define OT_START (OT-0x8000u)
#define OT_BYTES 0x18000u
#define HALT 0x80000080u
static const struct { uint32_t address; unsigned size; } bodies[] = {
    {0x80025544,0x1cc}, {0x80049efc,0x30}, {0x80049f8c,0x20},
    {0x8004a67c,0x54}, {0x80043b48,0x3c}
};
static uint8_t image[0x50000];
static uint32_t rd(const void *p) { uint32_t v; memcpy(&v,p,4); return v; }
static void wr(void *p,uint32_t v) { memcpy(p,&v,4); }
static void half(uint32_t a,uint16_t v) { memcpy(PSX_ADDR(a),&v,2); }
static void word(uint32_t a,uint32_t v) { wr(PSX_ADDR(a),v); }
static uint32_t canonical(uintptr_t p) {
    if(p>=(uintptr_t)g_PsxRam && p<(uintptr_t)g_PsxRam+PSX_RAM_SIZE)
        return 0x80000000u+(uint32_t)(p-(uintptr_t)g_PsxRam);
    return ((uint32_t)p & 0x1fffffffu)|0x80000000u;
}

typedef struct {
    const char *name; uint16_t gate,size; uint32_t xy0,xy1;
    int32_t depth; uint32_t shift,flags; int spare,domain,matrix;
} Case;
static const Case *active;
static unsigned calls[4], plain;
static uint32_t link_ot[2];
static SVECTOR vectors[3];
static unsigned sequence[8], sequence_count;
static int alias_ok;
static volatile sig_atomic_t native_active;
static void event(unsigned e) { if(sequence_count<8)sequence[sequence_count++]=e; }
static void vector_copy(unsigned i,const SVECTOR *v) {
    vectors[i].vx=v->vx; vectors[i].vy=v->vy; vectors[i].vz=v->vz;
    vectors[i].pad=0; /* Padding is not a GTE coordinate. */
}
#ifndef TYPE9_REAL_GTE
void SetRotMatrix(MATRIX *m) { assert(!memcmp(m,&D_8004FBB8,32)); ++calls[0]; event(0); }
void SetTransMatrix(MATRIX *m) { assert(!memcmp(m,&D_8004FBB8,32)); ++calls[1]; event(1); }
int RotTransPers3(SVECTOR *a,SVECTOR *b,SVECTOR *c,long *x,long *y,long *z,long *p,long *f) {
    vector_copy(0,a); vector_copy(1,b); vector_copy(2,c);
    ++calls[2]; event(2); alias_ok=(p==f);
    *x=(int32_t)active->xy0; *y=(int32_t)active->xy1; *z=0x13572468;
    *p=123; *f=(int32_t)0x80000001u;
    return active->depth;
}
#endif
/* Count calls without replacing the production helper. */
extern void __real_PcPort_AddPrimDomainAware(void *,void *);
void __wrap_PcPort_AddPrimDomainAware(void *ot,void *prim) {
    if(calls[3]<2)link_ot[calls[3]]=canonical((uintptr_t)ot);
    ++calls[3]; event(3);
    __real_PcPort_AddPrimDomainAware(ot,prim);
}
/* The pre-repair draft and plain-link mutant can run, but must be rejected. */
void AddPrim(void *ot,void *prim) {
    ++plain;
    wr(prim,(rd(prim)&0xff000000u)|(rd(ot)&0xffffffu));
    wr(ot,(rd(ot)&0xff000000u)|((uintptr_t)prim&0xffffffu));
}
static int bus_read(void *p,uint32_t a,unsigned n,uint32_t *v) {
    (void)p; uint32_t off=a&0x1fffffffu;
    if(off>PSX_RAM_SIZE-n)return -1;
    *v=0; memcpy(v,g_PsxRam+off,n); return 0;
}
static int bus_write(void *p,uint32_t a,unsigned n,uint32_t v) {
    (void)p; uint32_t off=a&0x1fffffffu;
    if(off>PSX_RAM_SIZE-n)return -1;
    memcpy(g_PsxRam+off,&v,n); return 0;
}
#ifdef TYPE9_REAL_GTE
static uint32_t cop_read(void *p,int c,unsigned r) { (void)p; return c?CFC2(r):MFC2(r); }
static void cop_write(void *p,int c,unsigned r,uint32_t v) { (void)p; if(c)CTC2(v,r);else MTC2(v,r); }
static int cop_command(void *p,uint32_t i) { (void)p; doCOP2((int)i); return 0; }
#endif
static int bridge(void *p,PcPortMipsCpu *cpu,uint32_t target) {
    (void)p;
    if(target==0x80043b48) {
        if(calls[3]<2)link_ot[calls[3]]=canonical(cpu->gpr[4]);
        ++calls[3]; event(3);
        return 0; /* Execute the actual two-tag retail link operation. */
    }
    if(target==0x80049efc || target==0x80049f8c) {
        unsigned i=target==0x80049efc?0:1;
        ++calls[i]; event(i);
        assert(!memcmp(PSX_ADDR(cpu->gpr[4]),&D_8004FBB8,32));
#ifndef TYPE9_REAL_GTE
        return 1;
#endif
    }
    if(target==0x8004a67c) {
        ++calls[2]; event(2);
        for(unsigned i=0;i<3;i++)vector_copy(i,PSX_ADDR(cpu->gpr[4+i]));
        uint32_t sp=cpu->gpr[29], dp=rd(PSX_ADDR(sp+24)), flag=rd(PSX_ADDR(sp+28));
        alias_ok=dp==flag;
#ifndef TYPE9_REAL_GTE
        word(cpu->gpr[7],active->xy0);
        word(rd(PSX_ADDR(sp+16)),active->xy1);
        word(rd(PSX_ADDR(sp+20)),0x13572468);
        word(dp,123); word(flag,0x80000001u);
        cpu->gpr[2]=(uint32_t)active->depth;
        return 1;
#endif
    }
    return 0;
}
static void reset(const Case *t,int native) {
    active=t;
    memset(g_PsxRam,0xcc,sizeof(g_PsxRam));
    for(unsigned i=0;i<sizeof(bodies)/sizeof(bodies[0]);i++)
        memcpy(PSX_ADDR(bodies[i].address),image+(bodies[i].address-0x8000f800u),bodies[i].size);
    uint32_t alias=t->domain==1?0x20000000u:0;
    uint32_t sprite=SPRITE|alias,work=WORK|alias,ot=OT|alias;
    if(native && t->domain==2) {
        sprite=(uint32_t)(uintptr_t)PSX_ADDR(SPRITE);
        work=(uint32_t)(uintptr_t)PSX_ADDR(WORK);
        ot=(uint32_t)(uintptr_t)PSX_ADDR(OT);
    }
    word(TASK+4,sprite); half(SPRITE+0x34,t->gate); half(SPRITE+0x36,t->size);
    half(SPRITE+2,strcmp(t->name,"signed-x-wrap")==0?0x7ff0:(uint16_t)-123);
    half(SPRITE+6,77); half(SPRITE+10,250);
    word(SPRITE+0x28,0x7a563412); word(SPRITE+0x3c,t->flags);
    word(0x80050100,t->shift); word(0x80059580,work);
    word(0x80059534,work+16+t->spare); word(0x8005956c,ot);
    /* Every OT word has a nonzero length and prior link. Full surrounding
     * range comparison detects wrong stride and signed-depth addressing. */
    for(unsigned i=0;i<OT_BYTES;i+=4)word(OT_START+i,0xd5001234);
    g_GfxCurWorkBuffer=(void *)(uintptr_t)work;
    g_GfxCurWorkBufferEnd=(void *)(uintptr_t)(work+16+t->spare);
    g_GfxCurOT=(void *)(uintptr_t)ot; D_80050100=t->shift;
    memset(calls,0,sizeof(calls)); memset(link_ot,0,sizeof(link_ot));
    memset(vectors,0,sizeof(vectors)); memset(sequence,0,sizeof(sequence));
    plain=sequence_count=0; alias_ok=0; PcPort_PrimLinkReset();
    memset(&D_8004FBB8,0,sizeof(D_8004FBB8));
    D_8004FBB8.m[0][0]=4096; D_8004FBB8.m[1][1]=4096; D_8004FBB8.m[2][2]=4096;
    D_8004FBB8.t[2]=4096;
    if(t->matrix==1) {
        D_8004FBB8.m[0][0]=2896; D_8004FBB8.m[0][2]=2896;
        D_8004FBB8.m[2][0]=-2896; D_8004FBB8.m[2][2]=2896;
        D_8004FBB8.t[0]=-137; D_8004FBB8.t[1]=83; D_8004FBB8.t[2]=700;
    } else if(t->matrix==2) {
        D_8004FBB8.m[0][0]=-4096; D_8004FBB8.t[2]=-245;
    }
    /* Retail CTC2 reg4 discards this matrix padding halfword. */
    uint16_t matrix_pad=0x5aa5;
    memcpy((uint8_t *)&D_8004FBB8+18,&matrix_pad,2);
    memcpy(PSX_ADDR(0x8004fbb8),&D_8004FBB8,32);
#ifdef TYPE9_REAL_GTE
    memset(&gteRegs,0,sizeof(gteRegs));
    CTC2(160u<<16,24); CTC2(112u<<16,25); CTC2(512,26);
    CTC2(0xfffffff0u,27); CTC2(0x100000,28);
    CTC2(0x155,29); CTC2(0x100,30);
#endif
}
typedef struct {
    uint8_t sprite[0xe0],work[0x80],ot[OT_BYTES],task[0x30];
    uint32_t cursor,link[2],gte[64]; unsigned calls[4],seq[8],nseq,plain;
    SVECTOR vectors[3]; int alias;
} Snapshot;
static void capture(Snapshot *s,int native) {
    memset(s,0,sizeof(*s));
    memcpy(s->sprite,PSX_ADDR(SPRITE-16),sizeof(s->sprite));
    memcpy(s->work,PSX_ADDR(WORK-16),sizeof(s->work));
    memcpy(s->ot,PSX_ADDR(OT_START),sizeof(s->ot));
    memcpy(s->task,PSX_ADDR(TASK),sizeof(s->task));
    wr(s->task+4,canonical(rd(s->task+4)));
    s->cursor=canonical(native?(uintptr_t)g_GfxCurWorkBuffer:rd(PSX_ADDR(0x80059580)));
    memcpy(s->link,link_ot,sizeof(link_ot)); memcpy(s->calls,calls,sizeof(calls));
    memcpy(s->seq,sequence,sizeof(sequence)); s->nseq=sequence_count;
    memcpy(s->vectors,vectors,sizeof(vectors)); s->alias=alias_ok; s->plain=plain;
#ifdef TYPE9_REAL_GTE
    /* Canonical register reads compare all architecturally visible data and
     * control registers, including IR/MAC/SXY/SZ/FLAG and untouched state. */
    for(unsigned i=0;i<32;i++) {s->gte[i]=MFC2(i);s->gte[32+i]=CFC2(i);}
#endif
}
static int equal(const char *name,const char *what,const void *a,const void *b,size_t n) {
    if(!memcmp(a,b,n))return 1;
    size_t i=0;while(i<n && ((const uint8_t *)a)[i]==((const uint8_t *)b)[i])i++;
    fprintf(stderr,"TYPE9 FAIL case=%s %s byte=%zu native=%02x retail=%02x\n",name,what,i,((const uint8_t *)a)[i],((const uint8_t *)b)[i]);
    return 0;
}
static int run_case(const Case *t) {
    Snapshot native,guest; PcPortMipsCpu cpu;
    PcPortMipsBus bus={.read=bus_read,.write=bus_write,.bridge=bridge};
#ifdef TYPE9_REAL_GTE
    bus.cop2_read=cop_read;bus.cop2_write=cop_write;bus.cop2_command=cop_command;
#endif
    reset(t,0);PcPortMipsCpuInit(&cpu,&bus);
    cpu.gpr[4]=TASK|(t->domain==1?0x20000000u:0);cpu.gpr[28]=0x80059170;
    cpu.gpr[29]=0x801ff000;cpu.gpr[31]=HALT;
    if(PcPortMipsRun(&cpu,0x80025544,HALT,4000)!=PC_PORT_MIPS_HALTED) {
        fprintf(stderr,"TYPE9 FAIL case=%s oracle %s\n",t->name,cpu.error);return 0;
    }
    capture(&guest,0);
    reset(t,1);
    fprintf(stderr,"TYPE9 NATIVE case=%s\n",t->name);fflush(stderr);
    native_active=1;
    func_80025544(PSX_ADDR(TASK));
    native_active=0;
    capture(&native,1);
    unsigned expected=t->gate?0:(t->spare>8?2:(t->spare>0?1:0));
    if(native.calls[3]!=expected || native.plain || PcPort_PrimLinkRejectCount() ||
       PcPort_PrimLinkGuestCount()!=(int)expected) {
        fprintf(stderr,"TYPE9 FAIL case=%s link-count/domain got=%u expected=%u plain=%u\n",t->name,native.calls[3],expected,native.plain);return 0;
    }
#define EQ(field) if(!equal(t->name,#field,&native.field,&guest.field,sizeof(native.field)))return 0
    EQ(sprite);EQ(work);EQ(ot);EQ(task);EQ(cursor);EQ(link);
#ifndef TYPE9_REAL_GTE
    EQ(calls);EQ(seq);EQ(nseq);EQ(vectors);EQ(alias);
    if(expected && !native.alias) {fprintf(stderr,"TYPE9 FAIL case=%s output alias\n",t->name);return 0;}
#else
    EQ(gte);
#endif
#undef EQ
    if(expected && guest.link[0] != OT+((uint32_t)(int32_t)(t->depth>>(t->shift&31))<<2)) {
#ifndef TYPE9_REAL_GTE
        fprintf(stderr,"TYPE9 FAIL case=%s signed OT fixture\n",t->name);return 0;
#endif
    }
    return 1;
}
static void pointer_fault(int sig) {
    (void)sig;
    static const char marker[]="TYPE9 POINTER_FAULT SIGSEGV during native case\n";
    if(native_active) {write(2,marker,sizeof(marker)-1);_exit(90);}
    _exit(91); /* An oracle/harness fault is never an accepted negative. */
}
int main(void) {
    static const Case cases[]={
        {"gate-nonzero",1,37,0x0032fff0,0x00640010,4096,0,0,24,0,0},
        {"first-exact-end",0,37,0x0032fff0,0x00640010,4096,0,0,0,0,0},
        {"first-over-end",0,37,0x0032fff0,0x00640010,4096,0,0,-1,0,0},
        {"tile-only-one-byte",0,37,0x0032fff0,0x00640010,4096,0,0,1,0,0},
        {"second-over-end",0,37,0x0032fff0,0x00640010,4096,0,0,7,0,0},
        {"second-exact-end",0,37,0x0032fff0,0x00640010,4096,0,0,8,0,0},
        {"mode-full",0,37,0x0032fff0,0x00640010,4096,0,0x60,9,0,0},
        {"projected-zero",0,37,0x1234fffe,0x5678fffe,4096,1,0x20,9,0,1},
        {"projected-plus-one",0,37,0x1234fffe,0x5678ffff,4096,2,0x40,9,0,0},
        {"projected-minus-one",0,37,0x12340010,0x5678000f,4096,31,0,9,0,0},
        {"projected-plus-three",0,37,0xfff80010,0xfff80013,4096,32,0x60,9,0,1},
        {"projected-minus-three",0,37,0xfff80010,0xfff8000d,4096,33,0x60,9,0,0},
        {"signed-x-wrap",0,64,0x11117fff,0x11110001,4096,33,0,9,0,1},
        {"maximum-difference",0,37,0x7fff8000,0x12347fff,4096,0,0,9,0,2},
        {"minimum-difference",0,37,0x80007fff,0x12348000,4096,0,0,9,0,2},
        {"kseg1-flags",0,37,0x005000c8,0x006400d8,4096,0,0xa5a50060,9,1,1},
        {"host-resolved",0,37,0x005000c8,0x006400d8,4096,0,0x40,9,2,1},
        {"negative-depth",0,37,0x005000c8,0x006400d8,-1,0,0x20,9,0,0},
        {"negative-shift",0,37,0x005000c8,0x006400d8,-4096,1,0x20,9,1,0},
        {"zero-depth",0,37,0x005000c8,0x006400d8,0,0,0x20,9,2,2},
    };
    FILE *f=fopen("disc/SLUS_006.64","rb");assert(f);
    assert(fread(image,1,sizeof(image),f)>=0x8004a6d0u-0x8000f800u);assert(!fclose(f));
    assert((uintptr_t)g_PsxRam+sizeof(g_PsxRam)<UINT32_MAX);
    signal(SIGSEGV,pointer_fault);
    if(!getenv("TYPE9_SKIP_BINDING")) {
        func_80025224(PSX_ADDR(TASK),9);
        if(bound!=(void (*)(void *))func_80025544) {fputs("TYPE9 FAIL binding slot9\n",stderr);return 1;}
    }
    for(unsigned i=0;i<sizeof(cases)/sizeof(cases[0]);i++)if(!run_case(&cases[i]))return 1;
#ifdef TYPE9_REAL_GTE
    puts("TYPE9 REAL_GTE PASS 20 cases: actual retail SDK instructions, complete visible GTE state, packed packets and guards");
#else
    puts("TYPE9 CONTROLLED PASS 20 cases: exact retail leaf/AddPrim; controlled GTE outputs, capacity, pointers, signed depth and guards");
#endif
    return 0;
}
