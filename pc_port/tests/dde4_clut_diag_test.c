/*
 * Differential regression test for func_8002DDE4 (SLUS 0x8002DDE4, the
 * multi-block VRAM uploader behind the field object-overlay texture upload
 * in func_801E742C and the anim-script 0xFC image chain) and for its
 * XENO_FIELD_DIAG CLUT readback.
 *
 * Oracle: the raw retail bytes of [8002DDE4,8002DFE0) from disc/SLUS_006.64
 * run on the project's MIPS interpreter with LoadImage (0x80044894) bridged
 * to a recording spy. Native: func_8002DDE4 extracted verbatim from
 * pc_port/src/game_overrides.c (DDE4_BODY) with the same spy. Both sides
 * record every LoadImage(rect, pixels) call as (x, y, w, h, blob offset,
 * payload hash) plus the return value, and both must leave the image blob
 * untouched.
 *
 * The native body is run twice per case: with XENO_FIELD_DIAG unset (the
 * retail path) and with XENO_FIELD_DIAG=1, where the diagnostic reads the
 * uploaded CLUT back through StoreImage into a stack buffer. That readback
 * accepted rectangles up to 256 x 4 halfwords but used a 256-halfword buffer,
 * which smashed the stack on map 2 (the 256 x 4 CLUT of archive 6B9's Gear
 * texture) and made XENO_FIELD_DIAG unusable there. The StoreImage spy here
 * writes the full w*h halfwords, the O0/O2 builds use -fstack-protector-all
 * and the third build is ASan+UBSan, so the overflow is a runtime failure,
 * not a silent one. The 256 x 4 cases are ordered first because the
 * diagnostic only reads back during its first eight logged calls.
 *
 * Limits: block widths/heights stay below 0x8000 (retail and native agree on
 * the s16 stride there; larger values never occur in shipped archives); the
 * spies do not model VRAM, only the call sequence and payload bytes.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "common.h"
#include "psyq/libgpu.h"
#include "battle_mips_adapter.h"

#ifndef DDE4_BODY
#error "DDE4_BODY must name the file holding func_8002DDE4 extracted from pc_port/src/game_overrides.c"
#endif

enum { REC_MAX = 64, BLOB_MAX = 0x30000, GUEST_BLOB = 0x80100000u,
       RETAIL_ENTRY = 0x8002DDE4u, RETAIL_END = 0x8002DFE0u,
       RETAIL_LOADIMAGE = 0x80044894u, HALT = 0xFFFFFFFCu };

static u8 ram[0x200000];
static u8 blob[BLOB_MAX], blobInitial[BLOB_MAX];
static u32 blobSize;

typedef struct Record { s16 x, y, w, h; u32 offset; u32 hash; } Record;
static Record records[REC_MAX], expectedRecords[REC_MAX];
static unsigned nRecords, recordOverflow, storeImages, storeMaxHalfwords, drawSyncs;
static u32 unexpectedTarget;

static u32 fnv(const u8* p, u32 n) {
    u32 h = 2166136261u;
    while (n--) { h ^= *p++; h *= 16777619u; }
    return h;
}

static void recordLoad(s16 x, s16 y, s16 w, s16 h, u32 offset, const u8* pixels) {
    Record* r;
    u32 bytes = (u32)((s32)w * (s32)h * 2);
    if (nRecords >= REC_MAX) { recordOverflow = 1; return; }
    r = &records[nRecords++];
    r->x = x; r->y = y; r->w = w; r->h = h; r->offset = offset;
    r->hash = (offset < blobSize && bytes <= blobSize - offset) ? fnv(pixels, bytes) : 0xDEADBEEFu;
}

/* Native SDK spies (prototypes from psyq/libgpu.h). */
int LoadImage(RECT16* rect, u_long* p) {
    recordLoad(rect->x, rect->y, rect->w, rect->h, (u32)((u8*)p - blob), (const u8*)p);
    return 0;
}
int StoreImage(RECT16* rect, u_long* p) {
    /* The GPU readback writes w*h halfwords into the caller's buffer. */
    u16* out = (u16*)p;
    s32 n = (s32)rect->w * (s32)rect->h, i;
    storeImages++;
    if ((unsigned)n > storeMaxHalfwords) storeMaxHalfwords = (unsigned)n;
    for (i = 0; i < n; ++i) out[i] = (u16)(0x8000u | (u32)(rect->x + i * 7 + rect->y * 13));
    return 0;
}
int DrawSync(int mode) { (void)mode; drawSyncs++; return 0; }

#include DDE4_BODY

/* Retail oracle bus. */
static u8* address(u32 a, unsigned w) {
    if (a >= 0x80000000u && (uint64_t)a + w <= 0x80200000u) return ram + (a & 0x1fffff);
    return NULL;
}
static int rd(void* u, u32 a, unsigned w, u32* v) {
    unsigned i; u8* p = address(a, w); (void)u;
    if (!p) return -1;
    *v = 0;
    for (i = 0; i < w; ++i) *v |= (u32)p[i] << (8 * i);
    return 0;
}
static int wr(void* u, u32 a, unsigned w, u32 v) {
    unsigned i; u8* p = address(a, w); (void)u;
    if (!p) return -1;
    for (i = 0; i < w; ++i) p[i] = (u8)(v >> (8 * i));
    return 0;
}
static u32 c2read(void* u, int control, unsigned r) { (void)u; (void)control; (void)r; return 0; }
static void c2write(void* u, int control, unsigned r, u32 v) { (void)u; (void)control; (void)r; (void)v; }
static int c2command(void* u, u32 op) { (void)u; (void)op; return -1; }
static int bridge(void* u, PcPortMipsCpu* c, u32 t) {
    (void)u;
    if (t == RETAIL_LOADIMAGE) {
        u8* r = address(c->gpr[4], 8);
        u32 p = c->gpr[5];
        if (!r || p < GUEST_BLOB || p >= GUEST_BLOB + blobSize) { unexpectedTarget = t; return -1; }
        recordLoad((s16)(r[0] | (r[1] << 8)), (s16)(r[2] | (r[3] << 8)),
                   (s16)(r[4] | (r[5] << 8)), (s16)(r[6] | (r[7] << 8)),
                   p - GUEST_BLOB, ram + (p & 0x1fffff));
        c->gpr[2] = 0;
        return 1;
    }
    if (t >= RETAIL_ENTRY && t < RETAIL_END) return 0;   /* guest code inside the slice */
    unexpectedTarget = t;
    return -1;
}

static u32 seed;
static u32 next(void) { seed = seed * 1103515245u + 12345u; return seed >> 8; }

/* Case description. */
typedef struct Block { u32 magic; u16 baseX, baseY, offsX, offsY, w, h; } Block;
typedef struct Case {
    unsigned count;
    Block blocks[8];
    s32 texMode, texX, texY, clutMode, clutX, clutY;
} Case;

static void buildBlob(const Case* c) {
    u32 n = 0, i;
    memset(blob, 0, sizeof(blob));
    *(u32*)(blob + n) = c->count; n += 4;
    for (i = 0; i < c->count; ++i) { *(u32*)(blob + n) = next(); n += 4; }   /* skipped words */
    for (i = 0; i < c->count; ++i) {
        const Block* b = &c->blocks[i];
        u32 bytes = (u32)b->w * b->h * 2, k;
        assert(n + 16 + bytes <= BLOB_MAX);
        assert((bytes & 3u) == 0);
        *(u32*)(blob + n) = b->magic; n += 4;
        *(u16*)(blob + n) = b->baseX; *(u16*)(blob + n + 2) = b->baseY;
        *(u16*)(blob + n + 4) = b->offsX; *(u16*)(blob + n + 6) = b->offsY; n += 8;
        *(u16*)(blob + n) = b->w; *(u16*)(blob + n + 2) = b->h; n += 4;
        for (k = 0; k < bytes; ++k) blob[n + k] = (u8)next();
        n += bytes;
    }
    blobSize = n;
    memcpy(blobInitial, blob, sizeof(blob));
}

static void resetSpies(void) {
    nRecords = 0; recordOverflow = 0; unexpectedTarget = 0;
    memset(records, 0, sizeof(records));
}

static int runNative(const Case* c, const char* diag, s32* ret) {
    memcpy(blob, blobInitial, sizeof(blob));
    resetSpies();
    setenv("XENO_FIELD_DIAG", diag, 1);
    *ret = func_8002DDE4(blob, c->texMode, c->texX, c->texY, c->clutMode, c->clutX, c->clutY);
    return memcmp(blob, blobInitial, sizeof(blob)) == 0;
}

static int compare(unsigned cfg, const char* pass, s32 ret, s32 expectedRet, unsigned expectedCount, int blobIntact) {
    if (!blobIntact || ret != expectedRet || nRecords != expectedCount || recordOverflow ||
        memcmp(records, expectedRecords, sizeof(records)) != 0) {
        unsigned i;
        fprintf(stderr, "DDE4 FAIL cfg=%u pass=%s ret=%d/%d records=%u/%u blob=%d\n",
                cfg, pass, (int)ret, (int)expectedRet, nRecords, expectedCount, blobIntact);
        for (i = 0; i < expectedCount && i < REC_MAX; ++i)
            fprintf(stderr, "  retail[%u] xy=(%d,%d) wh=(%d,%d) off=%u hash=%08x | native xy=(%d,%d) wh=(%d,%d) off=%u hash=%08x\n",
                    i, expectedRecords[i].x, expectedRecords[i].y, expectedRecords[i].w, expectedRecords[i].h,
                    expectedRecords[i].offset, expectedRecords[i].hash,
                    records[i].x, records[i].y, records[i].w, records[i].h, records[i].offset, records[i].hash);
        return 0;
    }
    return 1;
}

int main(void) {
    static const u16 clutSizes[][2] = { {256, 4}, {256, 2}, {256, 1}, {160, 1}, {208, 1}, {64, 1}, {256, 8}, {1, 4}, {16, 16} };
    /* Every payload is a whole number of words: retail reads the next block
     * header with lw, so an odd halfword count would trap on hardware too. */
    static const u16 texSizes[][2] = { {64, 136}, {32, 256}, {12, 256}, {4, 112}, {2, 16}, {8, 8}, {1, 2}, {46, 8}, {64, 32} };
    static const s32 modes[] = { 0, 1, 2, 3, 0x10001, 0x20002, -1, 0x7FFF0001 };
    static const Case fixed[] = {
        /* The map-2 Gear texture shape: 160x1 CLUT + 64x136 head block, mode 1/1. */
        { 2, { {0x1101, 0, 496, 0, 0, 256, 4}, {0x1100, 832, 256, 0, 0, 64, 136} }, 1, 576, 256, 1, 0, 252 },
        { 1, { {0x1101, 0, 496, 0, 0, 256, 4} }, 1, 0, 0, 1, 0, 252 },
        { 1, { {0x1101, 0, 496, 0, 0, 256, 4} }, 0, 0, 0, 2, 0, 460 },
        { 1, { {0x1101, 0, 480, 0, 1, 256, 2} }, 0, 0, 0, 0, 0, 0 },
        { 1, { {0x1101, 0, 460, 0, 2, 256, 8} }, 1, 0, 0, 1, 0, 0 },
        { 3, { {0x1101, 0, 496, 0, 0, 160, 1}, {0x1100, 832, 256, 0, 136, 8, 16}, {0x1100, 832, 256, 26, 192, 8, 8} }, 1, 576, 256, 1, 0, 252 },
        { 2, { {0x1101, 0, 496, 0, 0, 208, 1}, {0x1100, 832, 256, 0, 0, 32, 256} }, 1, 576, 256, 1, 0, 252 },
        { 1, { {0x1101, 0, 496, 0, 0, 256, 4} }, 2, -5, -7, 2, 0xFFF0, 0x1FFFC },
        { 2, { {0x1101, 0, 497, 0, 0, 256, 15}, {0x1100, 960, 52, 0, 0, 64, 204} }, 0, 0, 0, 0, 0, 0 },
        { 2, { {0x1101, 0, 496, 0, 0, 160, 1}, {0x1102, 832, 256, 0, 0, 8, 8} }, 1, 576, 256, 1, 0, 252 },
        { 1, { {0x0000, 0, 0, 0, 0, 8, 8} }, 1, 0, 0, 1, 0, 0 },
    };
    unsigned cfg, cases = 0, nFixed = (unsigned)(sizeof(fixed) / sizeof(fixed[0]));
    FILE* f;

    f = fopen("disc/SLUS_006.64", "rb");
    if (!f) { fputs("DDE4 FAIL disc/SLUS_006.64 missing\n", stderr); return 1; }
    assert(!fseek(f, 0x800L + (long)(RETAIL_ENTRY - 0x80010000u), SEEK_SET));
    assert(fread(ram + (RETAIL_ENTRY & 0x1fffff), 1, RETAIL_END - RETAIL_ENTRY, f) == RETAIL_END - RETAIL_ENTRY);
    assert(!fclose(f));
    if (*(u32*)(ram + (RETAIL_ENTRY & 0x1fffff)) != 0x27bdffb8u) {
        fputs("DDE4 FAIL retail slice does not start with addiu sp,-0x48\n", stderr);
        return 1;
    }

    for (cfg = 0; cfg < nFixed + 160; ++cfg) {
        Case c;
        PcPortMipsBus bus = { .read = rd, .write = wr, .bridge = bridge,
                              .cop2_read = c2read, .cop2_write = c2write, .cop2_command = c2command };
        PcPortMipsCpu cpu;
        s32 expectedRet, ret;
        unsigned expectedCount, i;
        int intact;

        seed = 0x9e3779b9u ^ (cfg * 2654435761u);
        if (cfg < nFixed) {
            c = fixed[cfg];
        } else {
            memset(&c, 0, sizeof(c));
            c.count = 1 + next() % 6;
            for (i = 0; i < c.count; ++i) {
                u32 r = next();
                Block* b = &c.blocks[i];
                b->magic = (r & 0x1F) == 0 ? 0x1100u + 2u + (r >> 8) % 3 : ((r & 1) ? 0x1101u : 0x1100u);
                b->baseX = (u16)(next() % 1024); b->baseY = (u16)(next() % 512);
                b->offsX = (u16)(next() % 300); b->offsY = (u16)(next() % 300);
                if (b->magic == 0x1101u) { const u16* s = clutSizes[next() % 9]; b->w = s[0]; b->h = s[1]; }
                else { const u16* s = texSizes[next() % 9]; b->w = s[0]; b->h = s[1]; }
            }
            c.texMode = modes[next() % 8];
            c.clutMode = modes[next() % 8];
            c.texX = (s32)(next() % 2048) - 512;
            c.texY = (s32)(next() % 1024) - 256;
            c.clutX = (s32)next();
            c.clutY = (s32)next();
        }
        buildBlob(&c);

        /* Retail oracle. */
        memcpy(ram + (GUEST_BLOB & 0x1fffff), blob, blobSize);
        resetSpies();
        PcPortMipsCpuInit(&cpu, &bus);
        cpu.gpr[4] = GUEST_BLOB;
        cpu.gpr[5] = (u32)c.texMode;
        cpu.gpr[6] = (u32)c.texX;
        cpu.gpr[7] = (u32)c.texY;
        cpu.gpr[29] = 0x801ff000u;
        cpu.gpr[31] = HALT;
        *(u32*)(ram + 0x1ff010) = (u32)c.clutMode;
        *(u32*)(ram + 0x1ff014) = (u32)c.clutX;
        *(u32*)(ram + 0x1ff018) = (u32)c.clutY;
        if (PcPortMipsRun(&cpu, RETAIL_ENTRY, HALT, 400000) != PC_PORT_MIPS_HALTED || unexpectedTarget || recordOverflow) {
            fprintf(stderr, "DDE4 FAIL oracle cfg=%u target=%08x %s\n", cfg, unexpectedTarget, cpu.error);
            return 1;
        }
        if (memcmp(ram + (GUEST_BLOB & 0x1fffff), blob, blobSize) != 0) {
            fprintf(stderr, "DDE4 FAIL oracle cfg=%u modified the image blob\n", cfg);
            return 1;
        }
        expectedRet = (s32)cpu.gpr[2];
        expectedCount = nRecords;
        memcpy(expectedRecords, records, sizeof(records));

        intact = runNative(&c, "0", &ret);
        if (!compare(cfg, "diag-off", ret, expectedRet, expectedCount, intact)) return 1;
        intact = runNative(&c, "1", &ret);
        if (!compare(cfg, "diag-on", ret, expectedRet, expectedCount, intact)) return 1;
        ++cases;
    }
    if (storeImages == 0 || storeMaxHalfwords < 1024) {
        fprintf(stderr, "DDE4 FAIL diagnostic readback not exercised (StoreImage calls=%u max=%u halfwords)\n",
                storeImages, storeMaxHalfwords);
        return 1;
    }
    printf("DDE4 PASS %u cases: LoadImage rect/offset/payload sequence and return vs retail, "
           "blob untouched, diag readback %u StoreImage calls (max %u halfwords), %u DrawSync\n",
           cases, storeImages, storeMaxHalfwords, drawSyncs);
    return 0;
}
