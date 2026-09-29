/*
 * Retail certificate for func_8002B8B0 (queue one streamed sector).
 *
 * Retail (func_8002B8B0.s 0x8002B8B0-0x8002BA40, frame 0x20):
 *   g_ArchiveCurFileSize <= 0 -> clear it and return without touching the ring.
 *   Otherwise rotate D_8004FE10 over D_8004FE40 ring slots (wrapping to 0)
 *   until a NOT_LOADED slot is found; when the walk ends on a loaded slot the
 *   walk test compares the counter against D_8004FE40 and then clears
 *   g_ArchiveCurFileSize.
 *   The found slot is marked state 1 with id D_8004FE26 (then bumped) and
 *   PCread(0x800) fills D_8004FE08 + index*0x800 with up to three
 *   func_8002804C(i,0,0xFF,0) retries; a read that never succeeds still runs the
 *   accounting below.
 *   Then g_ArchiveCurFileSize -= 0x800 and g_ArchiveCurFileSector += 1, and
 *   g_ArchiveCurFileSize is cleared when it did not stay positive.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "system/archive.h"
#include "psyq/pc.h"

extern void func_8002B8B0(void);

static u8 s_stream[0x8000];

int g_ArchiveCurFileSize;
int D_8004FE40;
ArchiveStreamFileSectionHeader* D_8004FE2C;
s32 D_8004FE10;
u16 D_8004FE26;
s8* D_8004FE08;
s32 D_80059F04;
s32 g_ArchiveCurFileSector;

static unsigned s_checks;
static int s_pcread;
static int s_pcread_fd;
static void* s_pcread_buf;
static int s_pcread_len;
static int s_pcread_fail;
static int s_retry;
static s32 s_retry_a;
static s32 s_retry_b;
static s32 s_retry_c;
static s32 s_retry_d;

int PCread(int fd, char* buff, int len)
{
    s_pcread++;
    s_pcread_fd = fd;
    s_pcread_buf = buff;
    s_pcread_len = len;
    if (s_pcread_fail > 0) {
        s_pcread_fail--;
        return 0;
    }
    return 1;
}

void func_8002804C(s32 a, s32 b, s32 c, s32 d)
{
    s_retry++;
    s_retry_a = a;
    s_retry_b = b;
    s_retry_c = c;
    s_retry_d = d;
}

static void fail(const char* name, const char* detail)
{
    fprintf(stderr, "ASSERTION %s %s\n", name, detail);
    exit(1);
}

static void expect_eq_s32(const char* field, s32 actual, s32 expected)
{
    char detail[160];

    s_checks++;
    if (actual != expected) {
        snprintf(detail, sizeof(detail), "field=%s actual=%d expected=%d",
                 field, (int)actual, (int)expected);
        fail("temp2.queue", detail);
    }
}

static void expect_eq_ptr(const char* field, const void* actual, const void* expected)
{
    char detail[160];

    s_checks++;
    if (actual != expected) {
        snprintf(detail, sizeof(detail), "field=%s actual=%p expected=%p",
                 field, actual, expected);
        fail("temp2.queue", detail);
    }
}

static u16* slot(int i)
{
    return (u16*)(s_stream + 4 + (i << 3));
}

static u8* slotbuf(int i)
{
    return (u8*)(s_stream + (*(s32*)s_stream * 8) + 0x24 + (i << 11));
}

static void reset_state(void)
{
    memset(s_stream, 0, sizeof(s_stream));
    *(s32*)s_stream = 4;

    g_ArchiveCurFileSize = 0x1000;
    D_8004FE40 = 4;
    D_8004FE2C = (ArchiveStreamFileSectionHeader*)(s_stream + 4);
    D_8004FE10 = 0;
    D_8004FE26 = 5;
    D_8004FE08 = (s8*)((u8*)s_stream + (*(s32*)s_stream * 8) + 0x24);
    D_80059F04 = 0x21;
    g_ArchiveCurFileSector = 0x30;

    s_pcread = 0;
    s_pcread_fd = 0;
    s_pcread_buf = NULL;
    s_pcread_len = 0;
    s_pcread_fail = 0;
    s_retry = 0;
    s_retry_a = 0;
    s_retry_b = 0;
    s_retry_c = 0;
    s_retry_d = 0;
}

int main(void)
{
    /* Idle stream: no read, ring untouched. */
    reset_state();
    g_ArchiveCurFileSize = 0;
    func_8002B8B0();
    expect_eq_s32("idle.size", g_ArchiveCurFileSize, 0);
    expect_eq_s32("idle.reads", s_pcread, 0);
    expect_eq_s32("idle.rotor", D_8004FE10, 0);
    expect_eq_s32("idle.id", D_8004FE26, 5);
    expect_eq_s32("idle.slot", slot(0)[0], 0);
    expect_eq_s32("idle.sector", g_ArchiveCurFileSector, 0x30);

    /* Happy path: first free slot is marked and filled. */
    reset_state();
    func_8002B8B0();
    expect_eq_s32("hit.reads", s_pcread, 1);
    expect_eq_s32("hit.fd", s_pcread_fd, 0x21);
    expect_eq_ptr("hit.buf", s_pcread_buf, slotbuf(0));
    expect_eq_s32("hit.len", s_pcread_len, 0x800);
    expect_eq_s32("hit.state", slot(0)[0], 1);
    expect_eq_s32("hit.id", slot(0)[1], 5);
    expect_eq_s32("hit.nextid", D_8004FE26, 6);
    expect_eq_s32("hit.rotor", D_8004FE10, 1);
    expect_eq_s32("hit.size", g_ArchiveCurFileSize, 0x800);
    expect_eq_s32("hit.sector", g_ArchiveCurFileSector, 0x31);
    expect_eq_s32("hit.retry", s_retry, 0);

    /* Last sector: size reaches zero and stays there. */
    reset_state();
    g_ArchiveCurFileSize = 0x800;
    func_8002B8B0();
    expect_eq_s32("last.size", g_ArchiveCurFileSize, 0);
    expect_eq_s32("last.state", slot(0)[0], 1);
    expect_eq_s32("last.sector", g_ArchiveCurFileSector, 0x31);

    /* Rotation skips loaded slots and wraps the rotor. */
    reset_state();
    D_8004FE10 = 2;
    slot(2)[0] = 3;
    slot(3)[0] = 0;
    func_8002B8B0();
    expect_eq_s32("rot.state", slot(3)[0], 1);
    expect_eq_s32("rot.keep", slot(2)[0], 3);
    expect_eq_ptr("rot.buf", s_pcread_buf, slotbuf(3));
    expect_eq_s32("rot.rotor", D_8004FE10, 0);
    expect_eq_s32("rot.reads", s_pcread, 1);

    /* Full ring: no slot, no read, stream size cleared. */
    reset_state();
    slot(0)[0] = 3;
    slot(1)[0] = 3;
    slot(2)[0] = 3;
    slot(3)[0] = 1;
    func_8002B8B0();
    expect_eq_s32("full.reads", s_pcread, 0);
    expect_eq_s32("full.size", g_ArchiveCurFileSize, 0);
    expect_eq_s32("full.id", D_8004FE26, 5);
    expect_eq_s32("full.sector", g_ArchiveCurFileSector, 0x30);
    expect_eq_s32("full.rotor", D_8004FE10, 0);

    /* Read retries three times and still accounts the sector. */
    reset_state();
    s_pcread_fail = 4;
    func_8002B8B0();
    expect_eq_s32("retry.reads", s_pcread, 4);
    expect_eq_s32("retry.n", s_retry, 4);
    expect_eq_s32("retry.a", s_retry_a, 3);
    expect_eq_s32("retry.b", s_retry_b, 0);
    expect_eq_s32("retry.c", s_retry_c, 0xFF);
    expect_eq_s32("retry.d", s_retry_d, 0);
    expect_eq_s32("retry.size", g_ArchiveCurFileSize, 0x800);
    expect_eq_s32("retry.sector", g_ArchiveCurFileSector, 0x31);
    expect_eq_s32("retry.state", slot(0)[0], 1);

    /* One transient failure then success. */
    reset_state();
    s_pcread_fail = 1;
    func_8002B8B0();
    expect_eq_s32("transient.reads", s_pcread, 2);
    expect_eq_s32("transient.n", s_retry, 1);
    expect_eq_s32("transient.a", s_retry_a, 0);
    expect_eq_s32("transient.size", g_ArchiveCurFileSize, 0x800);

    /* New id is published on the slot, not the previous one. */
    reset_state();
    D_8004FE26 = 0x1234;
    slot(1)[0] = 2;
    D_8004FE10 = 1;
    func_8002B8B0();
    expect_eq_s32("id.slot", slot(2)[1], 0x1234);
    expect_eq_s32("id.slot.state", slot(2)[0], 1);
    expect_eq_s32("id.next", D_8004FE26, 0x1235);
    expect_eq_s32("id.keep", slot(1)[0], 2);
    expect_eq_s32("id.rotor", D_8004FE10, 3);
    expect_eq_ptr("id.buf", s_pcread_buf, slotbuf(2));

    printf("TEMP2 SECTOR QUEUE B8B0 certificate PASS checks=%u\n", s_checks);
    return 0;
}
