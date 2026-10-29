/* Paired-task adapter versus retail, using a deterministic heap double. */
#include "common.h"
#include "psx_memory.h"
#include "battle_mips_adapter.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

typedef struct WorkListEntry WorkListEntry;
#define WL_U32(pointer) ((u32)(uintptr_t)(pointer))
u8 g_PsxRam[PSX_RAM_SIZE];
s32 D_80059190, g_NumTimerWorkListEntries, g_NumWorkListEntries, g_WorkListCurTimer;
WorkListEntry *D_800594C0, *g_TimerWorkList, *D_80059590, *g_WorkList;
short D_80059494;
extern s32 D_80059184, D_80059464;
extern u8 D_800591AC, D_800591AF;
extern WorkListEntry *TimerWorkListAllocateTask(void *, s32) __attribute__((weak));
extern void TimerWorkListDeleteTask(WorkListEntry *) __attribute__((weak));
extern void *func_8001D0A4(void *, void *) __attribute__((weak));
extern void *func_8001D164(void *) __attribute__((weak));

#define AREA 0x80100000u
#define NODE 0x80100020u
#define OWNER 0x80100200u
#define STACK 0x801ff000u
#define HALT 0xfffffffcu
static unsigned allocations, frees;
static u32 allocated_size, allocated_flags, freed_pointer;

void *HeapAlloc(u_int size, u_int flags)
{
    allocations++;
    allocated_size = size;
    allocated_flags = flags;
    return PSX_ADDR(NODE);
}
u_int HeapFree(void *pointer)
{
    frees++;
    freed_pointer = (uintptr_t)pointer;
    return 17;
}

static u8 *address_bytes(u32 address, unsigned width)
{
    uintptr_t base = (uintptr_t)g_PsxRam;
    if (address >= base && (uint64_t)address + width <= base + sizeof(g_PsxRam))
        return (u8 *)(uintptr_t)address;
    if ((address < 0x200000u && (uint64_t)address + width <= 0x200000u) ||
        (address >= 0x80000000u && (uint64_t)address + width <= 0x80200000u))
        return PSX_ADDR(address);
    return NULL;
}
static int read_bus(void *context, u32 address, unsigned width, u32 *value)
{
    (void)context;
    const u8 *p = address_bytes(address, width);
    if (!p) return -1;
    *value = 0;
    for (unsigned i = 0; i < width; i++) *value |= (u32)p[i] << (i * 8);
    return 0;
}
static int write_bus(void *context, u32 address, unsigned width, u32 value)
{
    (void)context;
    u8 *p = address_bytes(address, width);
    if (!p) return -1;
    for (unsigned i = 0; i < width; i++) p[i] = value >> (i * 8);
    return 0;
}
static u32 word(u32 address)
{ u32 value; assert(read_bus(NULL, address, 4, &value) == 0); return value; }
static void put(u32 address, u32 value)
{ assert(write_bus(NULL, address, 4, value) == 0); }
static int heap_bridge(void *context, PcPortMipsCpu *cpu, u32 target)
{
    (void)context;
    if (target == 0x80031bdcu) {
        cpu->gpr[2] = (uintptr_t)HeapAlloc(cpu->gpr[4], cpu->gpr[5]);
        return 1;
    }
    if (target == 0x800320e8u) {
        cpu->gpr[2] = HeapFree((void *)(uintptr_t)cpu->gpr[4]);
        return 1;
    }
    return 0;
}

extern WorkListEntry *WorkListsAddTasks(u32,WorkListEntry*,void(*)(void*),void(*)(void*),void(*)(void*));
extern void WorkListRemoveTask(WorkListEntry *);
typedef struct { u32 id,tcount,wcount,marked,th,wh,alloc,size,flags;u8 bytes[0x240]; } PairSnapshot;
static void pair_reset(unsigned marked,unsigned linked) {
 memset(PSX_ADDR(AREA),0x5a,0x240);put(OWNER+0x10,0xa0000007u);
 allocations=allocated_size=allocated_flags=0;D_80059184=5;D_800591AC=marked;D_800591AF=3;D_80059464=9;
 g_NumTimerWorkListEntries=linked;g_NumWorkListEntries=linked;
 g_TimerWorkList=linked?PSX_ADDR(NODE+0x80):NULL;g_WorkList=linked?PSX_ADDR(NODE+0xc0):NULL;
 put(0x80059184,5);put(0x80059188,linked);put(0x8005918c,linked);put(0x80059464,9);
 put(0x8005958c,WL_U32(g_TimerWorkList));put(0x80059594,WL_U32(g_WorkList));
 assert(!write_bus(NULL,0x800591ac,1,marked));assert(!write_bus(NULL,0x800591af,1,3));
}
static PairSnapshot pair_snapshot(int native) {
 PairSnapshot s={0};s.id=native?D_80059184:word(0x80059184);s.tcount=native?g_NumTimerWorkListEntries:word(0x80059188);
 s.wcount=native?g_NumWorkListEntries:word(0x8005918c);s.marked=native?D_80059464:word(0x80059464);
 s.th=native?WL_U32(g_TimerWorkList):word(0x8005958c);s.wh=native?WL_U32(g_WorkList):word(0x80059594);
 s.alloc=allocations;s.size=allocated_size;s.flags=allocated_flags;memcpy(s.bytes,PSX_ADDR(AREA),sizeof(s.bytes));
 if(native) {assert(word(NODE+0x28)==WL_U32(WorkListRemoveTask));u32 guest=0x8001cb48;memcpy(s.bytes+NODE-AREA+0x28,&guest,4);}
 return s;
}
int main(void) {
 FILE *f=fopen("disc/SLUS_006.64","rb");assert(f);assert(!fseek(f,0x8001ca58u-0x8000f800u,SEEK_SET));
 assert(fread(PSX_ADDR(0x8001ca58u),1,0x840u,f)==0x840u);fclose(f);
 for(unsigned k=0;k<2;k++) {
  pair_reset(k,k);PcPortMipsCpu c;PcPortMipsBus bus={.read=read_bus,.write=write_bus,.bridge=heap_bridge};PcPortMipsCpuInit(&c,&bus);
  c.gpr[4]=0x68;c.gpr[5]=WL_U32(PSX_ADDR(OWNER));c.gpr[6]=0x800bcfac;c.gpr[7]=0x800bceac;
  c.gpr[28]=0x80059170;c.gpr[29]=STACK;c.gpr[31]=HALT;put(STACK+16,0x800bd024);
  int rc=PcPortMipsRun(&c,0x8001d1d8u,HALT,3000);if(rc!=PC_PORT_MIPS_HALTED)fprintf(stderr,"pair CPU: %s\n",c.error);assert(rc==PC_PORT_MIPS_HALTED);
  u32 expected=c.gpr[2];PairSnapshot oracle=pair_snapshot(0);
  pair_reset(k,k);u32 actual=WL_U32(WorkListsAddTasks(0x68,PSX_ADDR(OWNER),(void(*)(void*))0x800bcfac,(void(*)(void*))0x800bceac,(void(*)(void*))0x800bd024));
  PairSnapshot host=pair_snapshot(1);
  if(expected!=actual || memcmp(&oracle,&host,sizeof host)) {fprintf(stderr,"pair mismatch %u expected=%08x actual=%08x\n",k,expected,actual);return 1;}
 }
 puts("PAIR RETAIL PASS two synthetic allocation/list comparisons with deterministic heap double");
}
