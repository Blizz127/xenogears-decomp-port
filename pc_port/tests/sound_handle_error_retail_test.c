/*
 * Retail certificate for SoundHandleError (SLUS_006.64 [8003F6B0,8003F738),
 * 0x88 bytes, sha256 5352ec72...).
 *
 * The shipped production body from src/slus_006.64/system/sound.c is linked
 * directly (no reimplementation). Its five callees are recording boundaries in
 * this test only: the runner compiles a scratch copy of sound.c in which those
 * five definitions are weakened, so the strong recorders below win. That keeps
 * the unit under test real while making the ordered call trace and arguments
 * observable.
 *
 * Contract checked: the guard is `(g_SoundControlFlags & 0x88) == 0`. When it
 * passes, the body sets bit 3, stores the error id, frees SPU block 0x10000,
 * reloads the WDS entry, re-adds the SEDS entry, then calls func_8003BDFC(0x10)
 * and func_80039E60((D_80050924[0] << 16) | 1) -- in that exact order. When the
 * guard fails the body is a complete no-op.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "system/sound.h"

void SoundHandleError(s32 errorId);

/* Defined by sound.c as externs; the test owns the storage. */
short g_SoundControlFlags;
short g_SoundSpuErrorId;
SoundFile D_80050910;
u16 D_80050924[4];
SoundWDSEntry D_80050940;

static char s_trace[32];
static unsigned s_checks;
static int s_freeAddr;
static int s_bdfcFlags;
static s32 s_packed;

static void fail(const char* name, const char* detail)
{
    fprintf(stderr, "ASSERTION %s %s\n", name, detail);
    exit(1);
}

static void expect_s32(const char* field, s32 actual, s32 expected)
{
    char detail[160];

    s_checks++;
    if (actual != expected) {
        snprintf(detail, sizeof(detail), "field=%s actual=%d expected=%d",
                 field, (int)actual, (int)expected);
        fail("sound.handleerror", detail);
    }
}

static void expect_str(const char* field, const char* actual, const char* expected)
{
    char detail[160];

    s_checks++;
    if (strcmp(actual, expected) != 0) {
        snprintf(detail, sizeof(detail), "field=%s actual='%s' expected='%s'",
                 field, actual, expected);
        fail("sound.handleerror", detail);
    }
}

/* ---- recording boundaries for the five retail callees ---- */

int SoundSpuMemoryFreeBlock(int targetAddress)
{
    strncat(s_trace, "F", sizeof(s_trace) - strlen(s_trace) - 1);
    s_freeAddr = targetAddress;
    return 0;
}

SoundWDSEntry* SoundLoadWdsFile(SoundWDSEntry* pWdsFile, s32 mode)
{
    (void)pWdsFile;
    (void)mode;
    strncat(s_trace, "W", sizeof(s_trace) - strlen(s_trace) - 1);
    return NULL;
}

void SoundAddSedsEntry(SoundFile* pSoundFile)
{
    (void)pSoundFile;
    strncat(s_trace, "S", sizeof(s_trace) - strlen(s_trace) - 1);
}

s16 func_8003BDFC(s32 flags)
{
    strncat(s_trace, "B", sizeof(s_trace) - strlen(s_trace) - 1);
    s_bdfcFlags = (int)flags;
    return 0;
}

void func_80039E60(s32 packedId)
{
    strncat(s_trace, "E", sizeof(s_trace) - strlen(s_trace) - 1);
    s_packed = packedId;
}

/* ---- cases ---- */

typedef struct Case {
    s32 flags;
    const char* name;
    int runs;
} Case;

int main(void)
{
    static const Case cases[] = {
        { 0x0000, "flags.0000", 1 },
        { 0x0008, "flags.0008", 0 },
        { 0x0080, "flags.0080", 0 },
        { 0x0088, "flags.0088", 0 },
        { 0x0081, "flags.0081", 0 },
        { 0x0100, "flags.0100", 1 },
        { 0x8000, "flags.8000", 1 },
        { 0xFFF7, "flags.fff7", 0 },
        { 0xFFFF, "flags.ffff", 0 },
    };
    unsigned i;

    D_80050924[0] = 0x00DA;
    D_80050924[1] = 0;

    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        const Case* c = &cases[i];
        s32 before = c->flags;
        s32 errorId = (s32)(0x100 + i);

        g_SoundControlFlags = (short)c->flags;
        g_SoundSpuErrorId = 0;
        s_trace[0] = 0;
        s_freeAddr = -1;
        s_bdfcFlags = -1;
        s_packed = -1;

        SoundHandleError(errorId);

        if (c->runs) {
            expect_str(c->name, s_trace, "FWSBE");
            expect_s32("flags.after", g_SoundControlFlags, (s32)(short)(before | 8));
            expect_s32("errorId", g_SoundSpuErrorId, errorId);
            expect_s32("freeAddr", s_freeAddr, 0x10000);
            expect_s32("bdfcFlags", s_bdfcFlags, 0x10);
            expect_s32("packed", s_packed, (0x00DA << 16) | 1);

            /* idempotence: bit 3 is now set, so the guard blocks a repeat */
            s_trace[0] = 0;
            s_freeAddr = -1;
            SoundHandleError((s32)0x7FFF);
            expect_str("second.trace", s_trace, "");
            expect_s32("second.freeAddr", s_freeAddr, -1);
        } else {
            expect_str(c->name, s_trace, "");
            expect_s32("flags.unchanged", g_SoundControlFlags, (s32)(short)before);
            expect_s32("flags.errorId", g_SoundSpuErrorId, 0);
            expect_s32("flags.freeAddr", s_freeAddr, -1);
        }
    }

    printf("SOUND HANDLE ERROR certificate PASS checks=%u\n", s_checks);
    return 0;
}
