#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef int16_t s16;
typedef int32_t s32;
typedef uint32_t u32;
typedef void (*SpuIRQCallbackProc)(void);
typedef struct {
    u8 flags, unk1;
    s16 nextBlockIndex;
    s32 spuAddress, size;
    u32 unkC;
} SoundSpuMemoryBlock;
#define MAX_SPU_MEMORY_BLOCKS 12
extern short g_SoundControlFlags;
extern int g_SoundSpuIRQCount;
extern SpuIRQCallbackProc g_SoundSpuIrqCallbackFn;
extern SoundSpuMemoryBlock g_SoundSpuMemoryBlocks[MAX_SPU_MEMORY_BLOCKS];
extern _Alignas(16) u8 g_PsxRam[2*1024*1024];
#define PSX_ADDR(index) (g_PsxRam + (index))
_Static_assert(sizeof(SoundSpuMemoryBlock)==16 && offsetof(SoundSpuMemoryBlock,spuAddress)==4, "block layout");

#ifdef BUILD_COMMANDS
#include "../../src/cleanroom/sound_spu_irq_allocator.inc"
#else
void SoundSetSpuIrqCallback(u32);
void SoundSpuIRQHandler(void);
int SoundSpuMemoryFreeBlock(int);
short g_SoundControlFlags;
int g_SoundSpuIRQCount;
SpuIRQCallbackProc g_SoundSpuIrqCallbackFn;
SoundSpuMemoryBlock g_SoundSpuMemoryBlocks[MAX_SPU_MEMORY_BLOCKS];
_Alignas(16) u8 g_PsxRam[2*1024*1024];

typedef struct { const void *address; unsigned size, write; } Access;
static Access events[48];
static unsigned ne, seen, calls, cases;
static u16 initial_flags, callback_flags;
static u32 initial_count, callback_count;
static void expect(const void *address,unsigned width,unsigned write)
{
    assert(ne<sizeof events/sizeof *events);
    events[ne++]=(Access){address,width,write};
}
static void begin(void){ne=0;seen=0;}
static void done(void)
{
#ifdef OBSERVE_ACCESSES
    assert(ne==seen);
#endif
    cases++;
}
static void unexpected(void){assert(!"unexpected IRQ callback");}
static void reenter(void)
{
    assert(calls++==0 && (u16)g_SoundControlFlags==(u16)(initial_flags|4));
    assert((u32)g_SoundSpuIRQCount==initial_count+1);
    memcpy(&g_SoundControlFlags,&callback_flags,2);
    memcpy(&g_SoundSpuIRQCount,&callback_count,4);
    g_SoundSpuIrqCallbackFn=unexpected;
}
#ifdef OBSERVE_ACCESSES
static void check(void *p,unsigned sz,unsigned write)
{
    assert(seen<ne); Access e=events[seen++];
    assert(p==e.address && sz==e.size && write==e.write);
}
void __tsan_init(void) {}
void __tsan_func_entry(void *p){(void)p;}
void __tsan_func_exit(void){}
#define CHECK(n) \
void __tsan_read##n(void *p){check(p,n,0);} \
void __tsan_write##n(void *p){check(p,n,1);} \
void __tsan_volatile_read##n(void *p){check(p,n,0);} \
void __tsan_volatile_write##n(void *p){check(p,n,1);}
CHECK(1) CHECK(2) CHECK(4) CHECK(8) CHECK(16)
#endif

static void irq(u16 bits,unsigned n)
{
    initial_flags=bits; initial_count=n%3==0 ? UINT32_MAX : (UINT32_C(0x7fffffff)+n);
    callback_flags=(u16)(bits^0xd054u); callback_count=~initial_count;
    memcpy(&g_SoundControlFlags,&bits,2);memcpy(&g_SoundSpuIRQCount,&initial_count,4);
    int present=(n%2)!=0;
    g_SoundSpuIrqCallbackFn=present?reenter:NULL;calls=0;
    begin();
    expect(&g_SoundControlFlags,2,0);expect(&g_SoundSpuIrqCallbackFn,sizeof g_SoundSpuIrqCallbackFn,0);
    expect(&g_SoundControlFlags,2,1);expect(&g_SoundSpuIRQCount,4,0);expect(&g_SoundSpuIRQCount,4,1);
    expect(&g_SoundControlFlags,2,0);expect(&g_SoundControlFlags,2,1);
    SoundSpuIRQHandler();
    assert(calls==(unsigned)present && (u16)g_SoundControlFlags==((present?callback_flags:bits)&0xfffb));
    assert((u32)g_SoundSpuIRQCount==(present?callback_count:initial_count+1));
    assert(g_SoundSpuIrqCallbackFn==(present?unexpected:NULL));done();
}

static void disposal(unsigned n, unsigned match)
{
    const int order[]={0,7,1,11,2,8,4,5,3,9,10,6};
    const unsigned length=n%12+1;
    SoundSpuMemoryBlock wanted[12];
    u8 ram[16];
    memset(g_SoundSpuMemoryBlocks,(int)(n^match),sizeof wanted);
    memset(g_PsxRam,(int)(0xa0u+n),sizeof ram);
    int key=(int)n-3200;
    for(unsigned i=0;i<length;i++){
        SoundSpuMemoryBlock *b=g_SoundSpuMemoryBlocks+order[i];
        b->spuAddress=(int)i-4000;
        b->nextBlockIndex=(s16)(i+1==length ? 0 : order[i+1]);
    }
    unsigned hit=match%(length+1);
    if(hit<length) {
        if (n%4==0) key=0;
        g_SoundSpuMemoryBlocks[order[hit]].spuAddress=key;
    }
    memcpy(wanted,g_SoundSpuMemoryBlocks,sizeof wanted);memcpy(ram,g_PsxRam,sizeof ram);
    begin();
    for(unsigned i=0;i<length;i++){
        int row=order[i]; SoundSpuMemoryBlock *b=&g_SoundSpuMemoryBlocks[row];
        expect(&b->spuAddress,4,0);expect(&b->nextBlockIndex,2,0);
        if(i==hit){
            if(i){
                expect(&g_SoundSpuMemoryBlocks[order[i-1]].nextBlockIndex,2,1);
                wanted[order[i-1]].nextBlockIndex=b->nextBlockIndex;
            }else{
                expect(g_PsxRam+2,2,1);memcpy(ram+2,&b->nextBlockIndex,2);
            }
            expect(&b->flags,1,1);expect(&b->unk1,1,1);expect(&b->spuAddress,4,1);expect(&b->nextBlockIndex,2,1);
            wanted[row].flags=0;wanted[row].unk1=0;wanted[row].spuAddress=0;wanted[row].nextBlockIndex=0;
            break;
        }
    }
    assert(SoundSpuMemoryFreeBlock(key)==(hit<length?key:0));
    assert(!memcmp(wanted,g_SoundSpuMemoryBlocks,sizeof wanted)&&!memcmp(ram,g_PsxRam,sizeof ram));done();
}

static void corrupt(s16 link)
{
    SoundSpuMemoryBlock initial[12];
    memset(g_SoundSpuMemoryBlocks,0,sizeof initial);
    g_SoundSpuMemoryBlocks[0].nextBlockIndex=link;
    g_SoundSpuMemoryBlocks[1].nextBlockIndex=1;
    memcpy(initial,g_SoundSpuMemoryBlocks,sizeof initial);
    begin();
    for(unsigned i=0;i<(link==1?12u:1u);++i){
        SoundSpuMemoryBlock *b=&g_SoundSpuMemoryBlocks[i?1:0];
        expect(&b->spuAddress,4,0);expect(&b->nextBlockIndex,2,0);
    }
    assert(SoundSpuMemoryFreeBlock(77)==0 && !memcmp(initial,g_SoundSpuMemoryBlocks,sizeof initial));
    done();
}

int main(void)
{
    for(unsigned n=0;n<65536;n++){
        u32 token=(u32)n*65537u;
        begin();expect(&g_SoundSpuIrqCallbackFn,sizeof g_SoundSpuIrqCallbackFn,1);
        SoundSetSpuIrqCallback(token);assert((uintptr_t)g_SoundSpuIrqCallbackFn==(uintptr_t)token);done();
        irq((u16)n,n);
    }
    for(unsigned n=0;n<300;n++)for(unsigned j=0;j<13;j++)disposal(n,j);
    corrupt(-1);corrupt(12);corrupt(32767);corrupt(1);
    printf("%u IRQ and allocator cases passed\n",cases);
    return 0;
}
#endif
