/* Host test of the native memory-card layer (pc_port/src/memcard_port.c).
 * Spec-level test (psx-spx card format + BIOS file API), not a retail
 * differential: the retail BIOS card driver talks to SIO hardware. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <psx/kernel.h>
#include "memcard_port.h"

void PcPort_ResetForTestDummy(void);
extern void PcPortCard_ResetForTest(void);
extern int _card_info(int chan);
static unsigned events[16][2], nevents;
void DeliverEvent(unsigned int a, int b) { assert(nevents < 16); events[nevents][0] = a; events[nevents++][1] = (unsigned)b; }

static unsigned char img[128 * 1024];
static const char* dir;
static void load_img(int port) {
    char p[512]; snprintf(p, sizeof p, "%s/card%d.mcd", dir, port + 1);
    FILE* f = fopen(p, "rb"); assert(f); assert(fread(img, 1, sizeof img, f) == sizeof img); fclose(f);
}
static unsigned get32(const unsigned char* p) { return p[0] | p[1] << 8 | p[2] << 16 | (unsigned)p[3] << 24; }
static void check_frames(void) {
    for (int fr = 0; fr < 36; ++fr) { unsigned char x = 0; for (int i = 0; i < 128; ++i) x ^= img[fr * 128 + i]; assert(x == 0); }
    assert(img[0] == 'M' && img[1] == 'C' && !memcmp(img, img + 63 * 128, 128));
}
static int count(const char* pat, char names[][21], int* sizes, int* heads) {
    struct DIRENTRY d; int n = 0; char path[40]; snprintf(path, sizeof path, "%s", pat);
    for (struct DIRENTRY* r = firstfile(path, &d); r; r = nextfile(&d)) {
        assert(r == &d && d.attr == 0x50 && d.next == NULL);
        if (names) strcpy(names[n], d.name);
        if (sizes) sizes[n] = d.size;
        if (heads) heads[n] = d.head;
        ++n;
    }
    return n;
}
static unsigned checks;
#define CHECK(x) do { assert(x); ++checks; } while (0)
int main(int argc, char** argv) {
    assert(argc == 2); dir = argv[1];
    setenv("XENO_MEMCARD_DIR", dir, 1); unsetenv("XENO_MEMCARD_SLOTS");
    unsigned char buf[3 * 8192], back[3 * 8192];
    for (unsigned i = 0; i < sizeof buf; ++i) buf[i] = (unsigned char)(i * 7 + (i >> 8));
    /* fresh card: formatted and empty, nothing written yet */
    CHECK(count("bu00:", 0, 0, 0) == 0);
    CHECK(PcPortCard_open("bu00:NOPE", 1) == -1);
    CHECK(erase("bu00:NOPE") == 0);
    /* create 3 blocks, write, read back */
    int fd = PcPortCard_open("bu00:BASLUS-00664GEAR0", (3u << 16) | 0x200);
    CHECK(fd >= 2);
    CHECK(PcPortCard_close(fd) == fd && PcPortCard_close(fd) == -1);
    CHECK(PcPortCard_open("bu00:BASLUS-00664GEAR0", (1u << 16) | 0x200) == -1); /* exists */
    fd = PcPortCard_open("bu00:BASLUS-00664GEAR0", 2);
    CHECK(PcPortCard_read(fd, back, 128) == -1);             /* write-only */
    CHECK(PcPortCard_write(fd, buf, 100) == -1);             /* not a sector multiple */
    CHECK(PcPortCard_write(fd, buf, 0x200) == 0x200);
    CHECK(PcPortCard_write(fd, buf + 0x200, sizeof buf - 0x200) == (int)sizeof buf - 0x200);
    CHECK(PcPortCard_write(fd, buf, 128) == -1);             /* past end of file */
    PcPortCard_close(fd);
    fd = PcPortCard_open("bu00:BASLUS-00664GEAR0", 3);
    CHECK(PcPortCard_read(fd, back, sizeof back) == (int)sizeof back && !memcmp(buf, back, sizeof buf));
    PcPortCard_close(fd);
    /* on-disk layout */
    load_img(0); check_frames();
    CHECK(get32(img + 128) == 0x51 && get32(img + 128 + 4) == 3 * 8192 && !strcmp((char*)img + 128 + 10, "BASLUS-00664GEAR0"));
    CHECK(img[128 + 8] == 1 && img[128 + 9] == 0);            /* -> block 2 */
    CHECK(get32(img + 256) == 0x52 && img[256 + 8] == 2);     /* -> block 3 */
    CHECK(get32(img + 384) == 0x53 && img[384 + 8] == 0xFF && img[384 + 9] == 0xFF);
    CHECK(get32(img + 512) == 0xA0);
    CHECK(!memcmp(img + 8192, buf, sizeof buf));
    /* second file, listing and wildcards */
    fd = PcPortCard_open("bu00:BESLES-XYZ", (1u << 16) | 0x200); CHECK(fd >= 2); PcPortCard_close(fd);
    char names[16][21]; int sizes[16], heads[16];
    CHECK(count("bu00:", names, sizes, heads) == 2 && !strcmp(names[0], "BASLUS-00664GEAR0") && sizes[0] == 3 * 8192 && heads[0] == 1 && heads[1] == 4 && sizes[1] == 8192);
    CHECK(count("bu00:*", 0, 0, 0) == 2 && count("bu00:BASLUS*", names, 0, 0) == 1);
    CHECK(count("bu00:BESLES-XY?", 0, 0, 0) == 1 && count("bu00:BESLES-XY", 0, 0, 0) == 0);
    CHECK(count("bu10:", 0, 0, 0) == 0);
    /* persistence across a fresh boot */
    PcPortCard_ResetForTest();
    CHECK(count("bu00:", 0, 0, 0) == 2);
    fd = PcPortCard_open("bu00:BASLUS-00664GEAR0", 1);
    CHECK(PcPortCard_read(fd, back, 8192) == 8192 && PcPortCard_read(fd, back + 8192, 16384) == 16384 && !memcmp(buf, back, sizeof buf));
    PcPortCard_close(fd);
    /* rename */
    CHECK(PcPortCard_rename("bu00:BESLES-XYZ", "bu00:BASLUS-00664GEAR0") == 0); /* target exists */
    CHECK(PcPortCard_rename("bu00:BESLES-XYZ", "bu10:NEW") == 0);               /* cross device */
    CHECK(PcPortCard_rename("bu00:BESLES-XYZ", "bu00:NEWNAME") == 1);
    CHECK(count("bu00:NEWNAME", 0, 0, 0) == 1 && count("bu00:BESLES-XYZ", 0, 0, 0) == 0);
    /* erase frees blocks for reuse */
    CHECK(erase("bu00:BASLUS-00664GEAR0") == 1 && count("bu00:", 0, 0, 0) == 1);
    load_img(0); check_frames();
    CHECK(get32(img + 128) == 0xA1 && get32(img + 256) == 0xA2 && get32(img + 384) == 0xA3);
    /* full card: 14 free blocks now */
    fd = PcPortCard_open("bu00:BIG", (15u << 16) | 0x200); CHECK(fd == -1);
    fd = PcPortCard_open("bu00:BIG", (14u << 16) | 0x200); CHECK(fd >= 2); PcPortCard_close(fd);
    fd = PcPortCard_open("bu00:ONE", (1u << 16) | 0x200); CHECK(fd == -1);
    load_img(0); check_frames();
    CHECK(get32(img + 128) == 0x51 && get32(img + 128 + 4) == 14 * 8192); /* reused deleted block 1 */
    /* format */
    CHECK(format("bu00:") == 1 && count("bu00:", 0, 0, 0) == 0);
    load_img(0); check_frames(); CHECK(get32(img + 128) == 0xA0);
    /* second port is independent */
    fd = PcPortCard_open("bu10:P2", (1u << 16) | 0x200); CHECK(fd >= 2); PcPortCard_close(fd);
    CHECK(count("bu10:", 0, 0, 0) == 1 && count("bu00:", 0, 0, 0) == 0);
    CHECK(PcPortCard_open("bu01:P2", 1) == -1 && PcPortCard_open("xx00:P2", 1) == -1);
    /* card presence events */
    nevents = 0;
    CHECK(_card_info(0x00) == 1 && _card_info(0x10) == 1 && _card_info(0x01) == 1);
    CHECK(nevents == 3 && events[0][0] == SwCARD && events[0][1] == EvSpIOE && events[1][1] == EvSpIOE && events[2][1] == EvSpTIMOUT);
    setenv("XENO_MEMCARD_SLOTS", "1", 1); PcPortCard_ResetForTest(); nevents = 0;
    CHECK(_card_info(0x10) == 1 && events[0][1] == EvSpTIMOUT);
    CHECK(PcPortCard_open("bu10:P2", 1) == -1 && count("bu10:", 0, 0, 0) == 0 && format("bu10:") == 0);
    CHECK(count("bu00:", 0, 0, 0) == 0);
    printf("PASS memcard_port checks=%u\n", checks);
    return 0;
}
