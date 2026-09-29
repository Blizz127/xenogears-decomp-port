#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "common.h"
#include "psyq/libcd.h"
#include "battle_mips_adapter.h"

/* Execute the pinned retail routine and compare its boundary trace with the
 * extracted production body. CD transport is deliberately not under test. */
static u8 ram[0x200000];
u8 D_801EA8F4[8];
u32 g_ArchiveTable, g_ArchiveHeader, g_ArchiveDebugTable;
typedef struct { u32 op, a, b, c, d, e; } Event;
typedef struct { u8 command, status, error; s32 result; } Reply;
static Event events[2][256];
static unsigned counts[2], side, reply_at, reply_count;
static Reply replies[128];
static s32 debug_mode;
static u8 disc_header[16];
static unsigned reload;

static void event(u32 op,u32 a,u32 b,u32 c,u32 d,u32 e) {
    assert(counts[side]<256);
    events[side][counts[side]++]=(Event){op,a,b,c,d,e};
}
static u8 *ptr(u32 a,unsigned w) {
    a&=0x1fffffff; assert((uint64_t)a+w<=sizeof ram); return ram+a;
}
static u32 word(const u8 *p) {
    return (u32)p[0]|(u32)p[1]<<8|(u32)p[2]<<16|(u32)p[3]<<24;
}
static void put(u32 a,u32 v) {
    u8*p=ptr(a,4); for(unsigned i=0;i<4;++i)p[i]=v>>(8*i);
}
static void swap_tables(void) {
    if (!reload) return;
    if(side==0) {
        put(0x8004fdf0,word(ptr(0x8004fdf0,4))+0x100);
        put(0x8004fdf4,word(ptr(0x8004fdf4,4))+0x100);
        put(0x8004fe48,word(ptr(0x8004fe48,4))+0x100);
    } else {
        g_ArchiveTable+=0x100; g_ArchiveHeader+=0x100; g_ArchiveDebugTable+=0x100;
    }
}
static void ArchiveCdDataSync(s32 a) {event(1,a,0,0,0,0);swap_tables();}
static s32 func_8002C3D8(void) {event(2,0,0,0,0,0);return debug_mode;}
static void func_801E9340(char *path,void *dst,s32 size) {
    const char *names[]={"c:\\work\\cdrom.mdg","c:\\work\\cdrom.fid","c:\\work\\cdrom.fnd",
        "c:\\work\\cdrom2.mdg","c:\\work\\cdrom2.fid","c:\\work\\cdrom2.fnd"};
    unsigned i; for(i=0;i<6 && strcmp(names[i],path);++i){} assert(i<6);
    event(3,i,(u32)(uintptr_t)dst,size,0,0);swap_tables();
}
static CdlLOC *test_pos(int a,CdlLOC *p) {
    event(4,a,0,0,0,0);u8 bytes[]={0,2,0,0};memcpy(p,bytes,4);return p;
}
static void Vsync(s32 a) {event(5,a,0,0,0,0);}
static int test_control(u_char command,u_char *param,u_char *result) {
    assert(reply_at<reply_count);Reply r=replies[reply_at++];assert(r.command==command);
    assert(command==2 ? param!=NULL : param==NULL);
    event(6,command,param?word(param):0,param!=NULL,0,0);
    result[0]=r.status;result[1]=r.error;return r.result;
}
static void ArchiveCdSetMode(s32 a) {event(7,a,0,0,0,0);}
static s32 ArchiveReadFileFromCdSector(s32 sector,void *dst,s32 size,s32 a,u32 flags) {
    event(8,sector,sector==0x17?0:(u32)(uintptr_t)dst,size,a,flags);
    if(sector==0x17) {
        for(unsigned i=0;i<16;++i)assert(((u8*)dst)[i]==0);
        assert(size==16);memcpy(dst,disc_header,16);
    }
    swap_tables();return -37; /* Retail ignores this return value. */
}
#define CdIntToPos test_pos
#define CdControlB test_control
#include "disc_validation.inc"
#undef CdIntToPos
#undef CdControlB

static int read_bus(void *o,u32 a,unsigned w,u32 *v) {
    (void)o;u8*p=ptr(a,w);*v=0;for(unsigned i=0;i<w;++i)*v|=(u32)p[i]<<(8*i);return 0;
}
static int write_bus(void *o,u32 a,unsigned w,u32 v) {
    (void)o;u8*p=ptr(a,w);for(unsigned i=0;i<w;++i)p[i]=v>>(8*i);return 0;
}
static int bridge(void *o,PcPortMipsCpu *c,u32 target) {
    (void)o;u32 a=c->gpr[4],b=c->gpr[5],d=c->gpr[6];
    switch(target) {
    case 0x80028a60: ArchiveCdDataSync(a);break;
    case 0x8002c3d8: c->gpr[2]=func_8002C3D8();break;
    case 0x801e9340: func_801E9340((char*)ptr(a,1),(void*)(uintptr_t)b,d);break;
    case 0x80041430: test_pos(a,(CdlLOC*)ptr(b,4));c->gpr[2]=b;break;
    case 0x8004b54c: Vsync(a);break;
    case 0x80041248: assert(d==0x801ea8f4);c->gpr[2]=test_control(a,b?ptr(b,4):NULL,ptr(d,8));break;
    case 0x8002a428: ArchiveCdSetMode(a);break;
    case 0x8002954c:
        c->gpr[2]=ArchiveReadFileFromCdSector(a,a==0x17?ptr(b,d):(void*)(uintptr_t)b,d,
            c->gpr[7],word(ptr(c->gpr[29]+16,4)));break;
    default:return 0;
    }
    return 1;
}
static void reply(unsigned cmd,unsigned status,unsigned error,s32 result) {
    assert(reply_count<128);replies[reply_count++]=(Reply){cmd,status,error,result};
}
static void schedule(unsigned wait,unsigned seek_bits,unsigned seek_ok) {
    reply_count=0;
    for(unsigned i=0;i<wait;++i)reply(1,0xe0,0,0);
    reply(1,0xf0,0,0); /* Lid-open status, return ignored. */
    for(unsigned i=0;i<wait;++i)reply(1,0xf0,0,0);
    reply(1,0xe0,0,0);
    for(unsigned i=0;i<wait;++i) {reply(1,0xe0,0,1);reply(1,0xe2,0,0);}
    reply(1,0xe2,0,-1);
    unsigned rounds=(!seek_ok && seek_bits!=3)?2:1;
    for(unsigned round=0;round<rounds;++round) {
        for(unsigned i=0;i<wait;++i)reply(0x13,0,0,0);
        reply(0x13,0,0,-1);
        for(unsigned i=0;i<wait;++i)reply(2,0,0,0);
        reply(2,0,0,1);
        reply(0x15,(seek_bits&1)?1:0,(seek_bits&2)?0x40:0,round?1:(s32)seek_ok);
    }
}
int main(void) {
    FILE*f=fopen("disc/menu.bin","rb");assert(f);
    assert(fread(ram+0x1c5000,1,0x3b000,f)>0x25000);fclose(f);
    const s32 discs[]={1,2,0,-1,0x101,INT32_MAX,INT32_MIN,-48};
    unsigned fixtures=0;
    for(unsigned dbg=0;dbg<3;++dbg)for(unsigned di=0;di<8;++di)
    for(unsigned data=0;data<3;++data)for(unsigned wait=0;wait<4;++wait)
    for(unsigned bits=0;bits<4;++bits)for(unsigned ok=0;ok<2;++ok)
    for(reload=0;reload<2;++reload) {
        debug_mode=dbg==2?-1:(s32)dbg;schedule(wait,bits,ok);
        memset(disc_header,0xa5,16);disc_header[3]=(u32)discs[di]+0x30+(data==1);
        disc_header[4]=0x5f;disc_header[5]=0x58;disc_header[6]=0x45;disc_header[7]=data==2?0:0x4e;
        memset(events,0,sizeof events);counts[0]=counts[1]=0;
        put(0x8004fdf0,0x00100000);put(0x8004fdf4,0x00110000);put(0x8004fe48,0x00120000);
        memset(ptr(0x801ea8f4,8),0xa5,8);memset(D_801EA8F4,0xa5,8);
        memset(ram+0x1af000,0xa5,0x1000);side=0;reply_at=0;
        PcPortMipsBus bus={.read=read_bus,.write=write_bus,.bridge=bridge};PcPortMipsCpu cpu;
        PcPortMipsCpuInit(&cpu,&bus);cpu.gpr[4]=discs[di];cpu.gpr[29]=0x801b0000;cpu.gpr[31]=0xfffffffc;
        int status=PcPortMipsRun(&cpu,0x801e93a0,0xfffffffc,10000);
        if(status)fprintf(stderr,"MIPS %d %s pc=%08x\n",status,cpu.error,cpu.pc);
        assert(status==PC_PORT_MIPS_HALTED);
        unsigned retail_replies=reply_at;
        side=1;reply_at=0;g_ArchiveTable=0x00100000;g_ArchiveHeader=0x00110000;g_ArchiveDebugTable=0x00120000;
        assert(func_801E93A0(discs[di])==(s32)cpu.gpr[2]);
        assert(counts[0]==counts[1]);assert(memcmp(events[0],events[1],sizeof events[0])==0);
        assert(reply_at==retail_replies && reply_at==(dbg?0:reply_count));
        assert(!memcmp(D_801EA8F4,ptr(0x801ea8f4,8),8));
        assert(g_ArchiveTable==word(ptr(0x8004fdf0,4)));
        assert(g_ArchiveHeader==word(ptr(0x8004fdf4,4)));
        assert(g_ArchiveDebugTable==word(ptr(0x8004fe48,4)));
        ++fixtures;
    }
    printf("PASS %u retail/native disc validation fixtures; CD/file services intercepted\n",fixtures);
}
