#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "fixture_types.h"

/* Exercise the Windows audio gate with the same ordered service oracle on a host build. */
#if defined(TEST_WINDOWS_CD_GATE) && !defined(_WIN32)
#define _WIN32 1
#endif
#ifdef _WIN32
void PsyX_Sys_SoundGateEnterCritical(void);
void PsyX_Sys_SoundGateExitCritical(void);
#endif

void SoundSetReverbModeWithAllocation(s32, s32, s32, s32);
void SoundInitiateReverbWorkAreaTransfer(s32, s32);
void SoundSetCdAttr(long, long);
void SoundInitializeAudioManager(AudioManager *);
void SoundApplyVolumeSettings(void);
void SoundExecuteReverbWorkAreaTransfer(void);
void SoundHandleError(s32);
int SoundSpuMemoryFreeBlock(s32);

extern u8 g_PsxRam[];
#define PSX_ADDR(address) ((void *)(g_PsxRam + ((u32)(address) & 0x1fffff)))
#ifdef BUILD_COMMANDS
#include "../../src/cleanroom/sound_reverb_controls.inc"
#include "../../src/cleanroom/sound_manager_init_cd.inc"
#else
SoundVolumeController g_SoundVolumeController;
SpuVolume g_SoundReverbDepth;
s32 g_ReverbWorkAreaSizes[SPU_REV_MODE_MAX], g_SoundReverbMemoryHandle;
u8 g_SoundReverbType, g_SoundReverbDelay, g_SoundReverbFeedback;
s32 g_SoundUploadDestBuffer, g_SoundUploadSourceAddr, g_SoundUploadBytesRemaining;
short g_SoundControlFlags;
_Alignas(8) u8 g_PsxRam[0x200000];

enum Boundary { READ, WRITE, CLEANUP, ENTER_CD_GATE, SET_CD, EXIT_CD_GATE, HEAP_ALLOC, ERROR, EXECUTE,
                GET_MODE, FREE_BLOCK, ALLOC_BLOCK, APPLY, SET_DEPTH, SET_TYPE, SET_DELAY, SET_FEEDBACK };
struct Event { enum Boundary kind; const void *address; u32 width, a, b, c; };
static struct Event events[80];
static unsigned used, seen, cases;
static const struct Event *take(enum Boundary kind)
{
    if (seen >= used || events[seen].kind != kind) {
        fprintf(stderr, "case %u event %u expected %d observed %d\n",
                cases, seen, seen < used ? (int)events[seen].kind : -1, (int)kind);
        assert(0);
    }
    return &events[seen++];
}
static void append(struct Event event)
{
    assert(used < sizeof events / sizeof events[0]);
    events[used++] = event;
}
static void service(enum Boundary kind, u32 a, u32 b, u32 c)
{
    const struct Event *e = take(kind);
    assert(e->a == a && e->b == b && e->c == c);
}
static void expectService(enum Boundary kind, u32 a, u32 b, u32 c)
{
    append((struct Event){kind, NULL, 0, a, b, c});
}
static void memory(enum Boundary kind, const void *address, u32 width)
{
#ifdef OBSERVE_ACCESSES
    append((struct Event){kind, address, width, 0, 0, 0});
#else
    (void)kind; (void)address; (void)width;
#endif
}
#ifdef OBSERVE_ACCESSES
static void observe(enum Boundary kind, void *address, u32 width)
{
    /* The query service takes the address of a local long on the caller's stack. */
    uintptr_t a = (uintptr_t)address, sp = (uintptr_t)&a;
    if ((a >= sp ? a-sp : sp-a) < 65536) return;
    const struct Event *e = take(kind);
    if (e->address != address || e->width != width)
        fprintf(stderr, "case %u access %u expected %p/%u observed %p/%u\n",
                cases, seen, e->address, e->width, address, width);
    assert(e->address == address && e->width == width);
}
void __tsan_read_range(void *p, size_t n) { observe(READ, p, (u32)n); }
void __tsan_unaligned_read4(void *p) { observe(READ, p, 4); }
void __tsan_init(void) {}
void __tsan_func_entry(void *p) { (void)p; }
void __tsan_func_exit(void) {}
#define OBSERVERS(n) \
    void __tsan_read##n(void *p) { observe(READ,p,n); } \
    void __tsan_volatile_read##n(void *p) { observe(READ,p,n); } \
    void __tsan_write##n(void *p) { observe(WRITE,p,n); } \
    void __tsan_volatile_write##n(void *p) { observe(WRITE,p,n); }
OBSERVERS(1)
OBSERVERS(2)
OBSERVERS(4)
OBSERVERS(8)
#undef OBSERVERS
#endif
#define R(field) memory(READ, &(field), sizeof(field))
#define W(field) memory(WRITE, &(field), sizeof(field))
static void finish(void) { assert(seen == used); seen = used = 0; ++cases; }

struct Row { u8 offset, width; u32 value; };
static const struct Row writes[] = {
    {0x32,2,1}, {0x36,2,1}, {0x3a,2,0x30}, {0x64,4,0x01000000},
    {0x70,4,0x7f000000}, {0x58,4,0x00660000}, {0x54,4,0x00006600},
    {0x1a,1,0}, {0x1b,1,0}, {0x30,2,0}, {0x34,2,0}, {0x38,2,4},
    {0x3c,2,4}, {0x3e,2,4}, {0x28,4,0}, {0x24,4,0}, {0x20,4,0},
    {0x48,4,0}, {0x7c,4,0}, {0x88,4,0}, {0x6c,2,0}, {0x78,2,0},
    {0x84,2,0}, {0x90,2,0}, {0x5c,4,0}, {0x60,2,0}, {0x50,4,0x10000}
};
static struct Guarded { u32 before; AudioManager m; u32 after; } manager, original, expected;
static unsigned challenge;
static void putRows(AudioManager *m)
{
    for (unsigned k = 0; k < sizeof writes / sizeof *writes; ++k) {
        const struct Row *r = &writes[k];
        memcpy((u8 *)m + r->offset, &r->value, r->width);
    }
}
void func_8003B930(AudioManager *m)
{
    service(CLEANUP, 0, 0, 0);
    assert(m == &manager.m && !memcmp(&manager, &original, sizeof manager));
    if (challenge & 1) memset(m, (int)challenge, sizeof *m);
}
static void testManager(void)
{
    for (challenge = 0; challenge < 256; ++challenge) {
        memset(&manager,(int)(challenge+97),sizeof manager);
        manager.m.prefix[0] = (u8)challenge;
        if (challenge & 2) putRows(&manager.m);
        original = expected = manager;
        if (challenge & 1) memset(&expected.m,(int)challenge,sizeof expected.m);
        putRows(&expected.m);
        expectService(CLEANUP,0,0,0);
        for (unsigned k = 0; k < sizeof writes/sizeof *writes; ++k)
            memory(WRITE,(u8 *)&manager.m + writes[k].offset,writes[k].width);
        SoundInitializeAudioManager(&manager.m);
        assert(!memcmp(&manager,&expected,sizeof manager));
        finish();
    }
}

static SoundVolumeController cdBeforeProvider, cdAtEnd;
#ifdef _WIN32
static unsigned cdGateDepth;
void PsyX_Sys_SoundGateEnterCritical(void)
{
    service(ENTER_CD_GATE,0,0,0);
    assert(cdGateDepth == 0);
    ++cdGateDepth;
}
void PsyX_Sys_SoundGateExitCritical(void)
{
    service(EXIT_CD_GATE,0,0,0);
    assert(cdGateDepth == 1);
    --cdGateDepth;
}
#endif
void SpuSetCommonAttr(SpuCommonAttr *attr)
{
    service(SET_CD,0,0,0);
#ifdef _WIN32
    assert(cdGateDepth == 1);
#endif
    assert(attr == &g_SoundVolumeController.commonAttr);
    assert(!memcmp(&cdBeforeProvider, &g_SoundVolumeController, sizeof cdBeforeProvider));
    if (challenge & 1) memset(&g_SoundVolumeController, 0x7c, sizeof g_SoundVolumeController);
}
static void testCd(void)
{
    const long values[] = {0,1,-1,LONG_MAX,LONG_MIN,0x12345678,(long)UINT64_C(0xabcdef0187654321)};
    for (unsigned i=0;i<sizeof values/sizeof *values;++i)
        for (unsigned j=0;j<sizeof values/sizeof *values;++j)
            for (challenge=0;challenge<4;++challenge) {
                memset(&g_SoundVolumeController,(int)(i*53+j*13),sizeof g_SoundVolumeController);
                SpuCommonAttr *a=&g_SoundVolumeController.commonAttr;
                if (challenge & 2) { a->cd.reverb=(s32)values[i]; a->cd.mix=(s32)values[j]; a->mask|=0x300u; }
                cdBeforeProvider=g_SoundVolumeController;
                cdBeforeProvider.commonAttr.cd.reverb=(s32)(u32)values[i];
                cdBeforeProvider.commonAttr.cd.mix=(s32)(u32)values[j];
                cdBeforeProvider.commonAttr.mask|=0x300u;
                cdAtEnd=cdBeforeProvider;
                if (challenge&1) memset(&cdAtEnd,0x7c,sizeof cdAtEnd);
#ifdef _WIN32
                expectService(ENTER_CD_GATE,0,0,0);
#endif
                R(a->mask); W(a->cd.reverb); W(a->cd.mix); W(a->mask);
                expectService(SET_CD,0,0,0);
#ifdef _WIN32
                expectService(EXIT_CD_GATE,0,0,0);
#endif
                SoundSetCdAttr(values[i],values[j]);
#ifdef _WIN32
                assert(cdGateDepth == 0);
#endif
                assert(!memcmp(&g_SoundVolumeController,&cdAtEnd,sizeof cdAtEnd));
                finish();
            }
}

static s32 inputAddress,inputSize,priorBuffer;
static short initialFlags;
static uintptr_t heapResult;
static int heapCalled, areaError;
static void prepareArea(s32 address,s32 size,s32 buffer,uintptr_t alloc)
{
    inputAddress=address; inputSize=size; priorBuffer=buffer; heapResult=alloc;
    heapCalled=buffer==0; areaError=heapCalled && (u32)alloc==0;
    g_SoundUploadDestBuffer=buffer;
    g_SoundUploadSourceAddr=address; g_SoundUploadBytesRemaining=size;
    initialFlags=(short)(cases*719u); g_SoundControlFlags=initialFlags;
}
static void expectArea(void)
{
    R(g_SoundUploadDestBuffer); W(g_SoundUploadSourceAddr); W(g_SoundUploadBytesRemaining);
    if (heapCalled) {
        expectService(HEAP_ALLOC,0x840,0,0); W(g_SoundUploadDestBuffer);
        if (areaError) expectService(ERROR,30,0,0);
    }
    R(g_SoundControlFlags); W(g_SoundControlFlags); expectService(EXECUTE,0,0,0);
}
void *SoundHeapAllocate(u32 size)
{
    service(HEAP_ALLOC,size,0,0);
    assert(g_SoundUploadSourceAddr==inputAddress && g_SoundUploadBytesRemaining==inputSize);
    g_SoundUploadDestBuffer=999; g_SoundControlFlags=(short)0x8181; g_SoundUploadSourceAddr=-37;
    return (void *)heapResult;
}
void SoundHandleError(s32 id)
{
    service(ERROR,(u32)id,0,0);
    if (id==30) {
        assert(g_SoundUploadDestBuffer==0 && g_SoundUploadSourceAddr==-37);
        g_SoundUploadDestBuffer=-53; g_SoundUploadBytesRemaining=31; g_SoundControlFlags=0x1122;
    } else {
        assert(id==32 && g_SoundReverbMemoryHandle==0); g_SoundReverbMemoryHandle=-39;
    }
}
void SoundExecuteReverbWorkAreaTransfer(void)
{
    service(EXECUTE,0,0,0);
    short flags=areaError?(short)0x1122:heapCalled?(short)0x8181:initialFlags;
    assert(g_SoundControlFlags==(short)(flags|0x20));
    assert(g_SoundUploadDestBuffer==(areaError?-53:heapCalled?(s32)(u32)heapResult:priorBuffer));
    assert(g_SoundUploadSourceAddr==(heapCalled?-37:inputAddress));
    assert(g_SoundUploadBytesRemaining==(areaError?31:inputSize));
    g_SoundUploadSourceAddr=-71; g_SoundUploadBytesRemaining=11; g_SoundControlFlags=28; g_SoundUploadDestBuffer=3;
}
static void endArea(void)
{
    assert(g_SoundUploadSourceAddr==-71 && g_SoundUploadBytesRemaining==11 && g_SoundControlFlags==28 && g_SoundUploadDestBuffer==3);
}
static void testArea(void)
{
    const s32 words[]={0,1,-1,INT32_MIN,INT32_MAX,0x201073};
    const uintptr_t allocations[]={0,1,0x8000d580u,(uintptr_t)UINT64_C(0x300000000),(uintptr_t)UINT64_C(0x300000041)};
    for(unsigned a=0;a<6;++a) for(unsigned s=0;s<6;++s) for(unsigned d=0;d<6;++d)
        for(unsigned k=0;k<5;++k) {
            prepareArea(words[a],words[s],words[d],allocations[k]); expectArea();
            SoundInitiateReverbWorkAreaTransfer(inputAddress,inputSize);
            endArea(); finish();
        }
}

static long observedMode;
static u32 allocBlockResult;
static void *sizeSlot;
static s32 sizeAfterFree;
static s32 storedMode, storedDepth, storedDelay, storedFeedback;
void SpuGetReverbModeType(long *out)
{
    service(GET_MODE,0,0,0);
    *out=observedMode;
    g_SoundReverbType=0xb9;
}
int SoundSpuMemoryFreeBlock(s32 handle)
{
    service(FREE_BLOCK,(u32)handle,0,0);
    memcpy(sizeSlot,&sizeAfterFree,sizeof sizeAfterFree);
    return 0;
}
u32 SoundSpuMemoryAllocateBlockAtAddress(s32 size,s32 address,s32 type)
{
    service(ALLOC_BLOCK,(u32)size,(u32)address,(u32)type);
    u32 changed=0x51ddaa31;
    memcpy(sizeSlot,&changed,sizeof changed);
    g_SoundReverbMemoryHandle=123;
    return allocBlockResult;
}
void SoundApplyVolumeSettings(void)
{
    service(APPLY,0,0,0);
    assert(g_SoundReverbType==(u8)storedMode);
    assert((u16)g_SoundVolumeController.currentReverbDepth==(u16)storedDepth);
    assert(g_SoundReverbDelay==(u8)storedDelay && g_SoundReverbFeedback==(u8)storedFeedback);
    g_SoundReverbDepth.left=-32761; g_SoundReverbDepth.right=30151;
    g_SoundReverbType=32; g_SoundReverbDelay=71; g_SoundReverbFeedback=13;
}
void SpuSetReverbModeDepth(short left,short right)
{
    service(SET_DEPTH,(u32)(s32)left,(u32)(s32)right,0);
}
long SpuSetReverbModeType(long mode)
{
    service(SET_TYPE,(u32)mode,0,0);
    assert(mode==(long)(s32)mode);
    return -1;
}
void SpuSetReverbModeDelayTime(long value)
{
    service(SET_DELAY,(u32)value,0,0);
    assert(value==(long)(s32)value);
}
void SpuSetReverbModeFeedback(long value)
{
    service(SET_FEEDBACK,(u32)value,0,0);
    assert(value==(long)(s32)value);
}
static void testMode(s32 inputMode,u8 savedType,unsigned variant)
{
    g_SoundReverbType=savedType;
    memset(&g_SoundVolumeController,(int)(variant+39),sizeof g_SoundVolumeController);
    g_SoundReverbDelay=42; g_SoundReverbFeedback=51;
    g_SoundReverbMemoryHandle=variant&1?-1:(s32)(0xdea00000u+variant);
    g_SoundReverbDepth.left=33; g_SoundReverbDepth.right=-44;
    const s32 suppliedDepth=(s32)(0x87880000u+variant);
    const s32 suppliedDelay=(s32)(0x91450000u+variant*53);
    const s32 suppliedFeedback=(s32)(0x4321789au-variant);
    const s32 mode=inputMode==-1 ? (s32)savedType : inputMode;
    if (inputMode==-2) {
        SoundVolumeController saved=g_SoundVolumeController;
        s32 handle=g_SoundReverbMemoryHandle;
        SoundSetReverbModeWithAllocation(inputMode,suppliedDepth,suppliedDelay,suppliedFeedback);
        assert(g_SoundReverbType==savedType && g_SoundReverbDelay==42 && g_SoundReverbFeedback==51);
        assert(g_SoundReverbMemoryHandle==handle && !memcmp(&saved,&g_SoundVolumeController,sizeof saved));
        finish(); return;
    }
    u32 reported=(u32)mode+(variant&2?1u:0u);
    observedMode=(long)(((variant&4 ? UINT64_C(0x47771000) << 32 : 0)) | reported);
    int reallocates=mode==0 || (u32)mode!=reported;
    s32 oldHandle=g_SoundReverbMemoryHandle;
    allocBlockResult=(variant&8)?0:(0x81230111u+variant);
    u32 fetchedSize=variant&16 ? 0x80002000u : (0x681u+variant);
    sizeAfterFree=(s32)(fetchedSize^0xc30fd391u);
    sizeSlot=(u32)mode<SPU_REV_MODE_MAX ? (void *)&g_ReverbWorkAreaSizes[mode] : PSX_ADDR(0x800508e8u+4u*(u32)mode);
    memcpy(sizeSlot,&fetchedSize,sizeof fetchedSize);
    if (inputMode==-1) R(g_SoundReverbType);
    expectService(GET_MODE,0,0,0);
    if (reallocates) {
        R(g_SoundReverbMemoryHandle);
        if (oldHandle!=-1) {
            expectService(FREE_BLOCK,(u32)oldHandle,0,0);
            fetchedSize=(u32)sizeAfterFree;
        }
        memory(READ,sizeSlot,4);
        u32 address=0x80000u-fetchedSize;
        expectService(ALLOC_BLOCK,fetchedSize,address,5); W(g_SoundReverbMemoryHandle);
        if (allocBlockResult==0) expectService(ERROR,32,0,0);
        prepareArea((s32)address,(s32)fetchedSize,(variant&16)?0:73,0x12408u);
    }
    const int clear=inputMode==0 || (reallocates && !allocBlockResult);
    storedMode=(reallocates && !allocBlockResult)?0:mode;
    storedDepth=clear?0:suppliedDepth;
    storedDelay=clear?0:suppliedDelay;
    storedFeedback=clear?0:suppliedFeedback;
    W(g_SoundReverbType); W(g_SoundVolumeController.currentReverbDepth);
    W(g_SoundReverbDelay); W(g_SoundReverbFeedback);
    expectService(APPLY,0,0,0);
    if (reallocates) {
        expectService(SET_DEPTH,0,0,0); expectService(SET_TYPE,(u32)storedMode,0,0); expectArea();
    } else {
        R(g_SoundReverbDepth.left); R(g_SoundReverbDepth.right);
        expectService(SET_DEPTH,(u32)-32761,30151,0);
        expectService(SET_DELAY,(u32)storedDelay,0,0); expectService(SET_FEEDBACK,(u32)storedFeedback,0,0);
    }
    SoundSetReverbModeWithAllocation(inputMode,suppliedDepth,suppliedDelay,suppliedFeedback);
    assert(g_SoundReverbType==32 && g_SoundReverbDelay==71 && g_SoundReverbFeedback==13);
    if (reallocates) {
        assert(g_SoundReverbMemoryHandle==(allocBlockResult?(s32)allocBlockResult:-39)); endArea();
    } else assert(g_SoundReverbMemoryHandle==oldHandle);
    finish();
}
int main(void)
{
    testManager();
    unsigned managerCases=cases;
    testCd();
    unsigned cdCases=cases-managerCases;
    testArea();
    unsigned areaCases=cases-managerCases-cdCases;
    const s32 modes[]={-2,-1,0,1,2,3,4,5,6,7,8,9,10,255,-3,INT32_MIN,0x40000004};
    const u8 recalls[]={0,4,9,10,255};
    for (unsigned k=0;k<sizeof modes/sizeof *modes;++k)
        for (unsigned j=0;j<(modes[k]==-1 ? sizeof recalls:1);++j)
            for(unsigned route=0;route<32;++route) testMode(modes[k],recalls[j],route);
    printf("PASS %u manager, %u CD, %u transfer, %u mode cases\n",
           managerCases,cdCases,areaCases,cases-managerCases-cdCases-areaCases);
    return 0;
}
#endif
