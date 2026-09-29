/* Native PSX memory-card layer: BIOS card/file API over raw card images.
 *
 * Retail game code talks to the memory card only through the PsyQ libapi
 * calls it links (InitCARD/StartCARD/_bu_init/_card_info plus the "buXY:"
 * file calls open/read/write/close/erase/format/rename/firstfile/nextfile).
 * PsyCross ships all of them as silent `return 0` no-ops, which makes a save
 * path fail silently or, worse, deserialize garbage on load.  This file is
 * their native owner (PsyCross's copies are removed by
 * pc_port/patches/psycross_card_native_owner.patch).
 *
 * Storage: one standard raw 128 KiB card image per port (the ".mcd/.mcr"
 * layout every PSX emulator reads), so saves are interchangeable with
 * emulators.  Images live in $XENO_MEMCARD_DIR (default "memcards/") as
 * card1.mcd (port 0, "bu00:") and card2.mcd (port 1, "bu10:").  A missing
 * image is a fresh, formatted card; it is written on the first change.
 * Every mutation is written through to disk (temp file + rename).
 * $XENO_MEMCARD_SLOTS selects which ports hold a card ("12" default, "1",
 * "2", or "0" for none).
 *
 * Card image layout (psx-spx "Memory Card Data Format"):
 *   16 blocks x 8 KiB; block 0 is the directory, 64 frames x 128 bytes.
 *   frame 0  : "MC", zero fill, XOR checksum in byte 0x7F
 *   frame 1-15: one directory entry per data block 1..15
 *       +00 u32 state  A0 free; 51 first/only block; 52 middle; 53 last;
 *                      A1/A2/A3 deleted first/middle/last
 *       +04 u32 file size in bytes (first block only)
 *       +08 u16 next block (block number minus 1), FFFF = end of chain
 *       +0A     file name, NUL terminated (first block only), max 20 chars
 *       +7F     XOR of bytes 00..7E
 *   frame 16-35: broken-sector list (all "none"), frame 63 = copy of frame 0
 *   blocks 1-15: file data, chained through the directory
 *
 * BIOS semantics implemented (all synchronous: Xenogears never passes
 * FNBLOCK/FASYNC):
 *   open(name, mode): FREAD 1, FWRITE 2, FCREAT 0x200 with the block count in
 *     mode bits 16..31.  Creating an existing name fails.  Returns an fd >= 2
 *     or -1.
 *   read/write(fd, buf, n): n must be a positive multiple of 128 and stay
 *     inside the file; returns n or -1.  The position advances by n.
 *   close(fd): returns fd or -1.
 *   erase(name), rename(old, new), format(dev): return 1 on success, 0 on
 *     failure (rename fails if the new name exists or the devices differ).
 *   firstfile(pattern, dir)/nextfile(dir): enumerate live files whose name
 *     matches the pattern ('?' = any one character, '*' = any remainder, an
 *     empty pattern matches every file); returns dir or NULL.  DIRENTRY gets
 *     name, attr 0x50, size in bytes, head = first block, next = NULL.
 *   _card_info(chan): delivers SwCARD/EvSpIOE when a card is present on that
 *     port (slot 0), SwCARD/EvSpTIMOUT otherwise, and returns 1.  The event
 *     is delivered before returning, so a caller that immediately polls
 *     TestEvent (func_801C881C does) sees it on the first probe.
 *
 * Failures are reported on stderr (fail-visible); nothing returns a fake
 * success. */
#include "memcard_port.h"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <psx/kernel.h>

/* PsyCross LIBAPI.C (not libapi.h: it redeclares host open/read/...). */
extern void DeliverEvent(unsigned int ev1, int ev2);

#define CARD_BYTES (128 * 1024)
#define BLOCK_BYTES 0x2000
#define FRAME_BYTES 0x80
#define DATA_BLOCKS 15
#define NAME_MAX_CHARS 20
#define MAX_FDS 16
#define FD_BASE 2

#define PSX_FREAD 0x0001
#define PSX_FWRITE 0x0002
#define PSX_FCREAT 0x0200

typedef struct PcPortCard {
    int loaded;
    unsigned char image[CARD_BYTES];
} PcPortCard;

typedef struct PcPortCardFd {
    int used;
    int port;
    int first_block;
    int size;
    int pos;
    int mode;
} PcPortCardFd;

static PcPortCard s_cards[2];
static PcPortCardFd s_fds[MAX_FDS];
static struct {
    int active;
    int port;
    int next_block;
    char pattern[NAME_MAX_CHARS + 2];
} s_search;

static void card_log(const char* what, const char* name)
{
    fprintf(stderr, "[xeno-port][memcard] %s%s%s\n", what, name ? ": " : "",
            name ? name : "");
}

static const char* card_dir(void)
{
    const char* dir = getenv("XENO_MEMCARD_DIR");
    return (dir && dir[0]) ? dir : "memcards";
}

static void card_path(int port, char* out, size_t size)
{
    snprintf(out, size, "%s/card%d.mcd", card_dir(), port + 1);
}

int PcPortCard_Present(int port)
{
    const char* slots = getenv("XENO_MEMCARD_SLOTS");
    if (port < 0 || port > 1) return 0;
    if (!slots || !slots[0]) return 1;
    return strchr(slots, '1' + port) != NULL;
}

static unsigned char* frame(PcPortCard* card, int index)
{
    return card->image + index * FRAME_BYTES;
}

static void frame_checksum(unsigned char* f)
{
    unsigned char x = 0;
    int i;
    for (i = 0; i < FRAME_BYTES - 1; ++i) x ^= f[i];
    f[FRAME_BYTES - 1] = x;
}

static uint32_t get32(const unsigned char* p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) |
           ((uint32_t)p[3] << 24);
}

static void put32(unsigned char* p, uint32_t v)
{
    p[0] = (unsigned char)v;
    p[1] = (unsigned char)(v >> 8);
    p[2] = (unsigned char)(v >> 16);
    p[3] = (unsigned char)(v >> 24);
}

static void format_image(unsigned char* image)
{
    int i;
    memset(image, 0, CARD_BYTES);
    image[0] = 'M';
    image[1] = 'C';
    frame_checksum(image);
    for (i = 1; i <= DATA_BLOCKS; ++i) {
        unsigned char* f = image + i * FRAME_BYTES;
        put32(f, 0xA0);
        f[8] = 0xFF;
        f[9] = 0xFF;
        frame_checksum(f);
    }
    for (i = 16; i < 36; ++i) {
        unsigned char* f = image + i * FRAME_BYTES;
        put32(f, 0xFFFFFFFFu);
        f[8] = 0xFF;
        f[9] = 0xFF;
        frame_checksum(f);
    }
    for (i = 36; i < 63; ++i) memset(image + i * FRAME_BYTES, 0xFF, FRAME_BYTES);
    memcpy(image + 63 * FRAME_BYTES, image, FRAME_BYTES);
}

static int card_formatted(const PcPortCard* card)
{
    return card->image[0] == 'M' && card->image[1] == 'C';
}

/* Returns the card for a port if a card is inserted there, loading (or
 * synthesizing a fresh formatted) image on first use. */
static PcPortCard* card_get(int port)
{
    PcPortCard* card;
    char path[1024];
    FILE* f;
    if (!PcPortCard_Present(port)) return NULL;
    card = &s_cards[port];
    if (card->loaded) return card;
    card_path(port, path, sizeof(path));
    f = fopen(path, "rb");
    if (!f) {
        format_image(card->image);
    } else {
        size_t n = fread(card->image, 1, CARD_BYTES, f);
        fclose(f);
        if (n != CARD_BYTES) {
            card_log("short card image, treating card as unformatted", path);
            memset(card->image, 0, CARD_BYTES);
        }
    }
    card->loaded = 1;
    return card;
}

static int card_flush(int port)
{
    char path[1024], temp[1100];
    FILE* f;
    const char* dir = card_dir();
    if (mkdir(dir, 0755) != 0 && errno != EEXIST) {
        card_log("cannot create card directory", dir);
        return -1;
    }
    card_path(port, path, sizeof(path));
    snprintf(temp, sizeof(temp), "%s.tmp", path);
    f = fopen(temp, "wb");
    if (!f) {
        card_log("cannot write card image", temp);
        return -1;
    }
    if (fwrite(s_cards[port].image, 1, CARD_BYTES, f) != CARD_BYTES) {
        fclose(f);
        remove(temp);
        card_log("short write of card image", temp);
        return -1;
    }
    if (fclose(f) != 0 || rename(temp, path) != 0) {
        remove(temp);
        card_log("cannot replace card image", path);
        return -1;
    }
    return 0;
}

/* "buXY:NAME" -> port X (slot Y must be 0), *name = "NAME". */
static int parse_path(const char* path, const char** name)
{
    *name = "";
    if (!path || path[0] != 'b' || path[1] != 'u') return -1;
    if (path[2] != '0' && path[2] != '1') return -1;
    if (path[3] != '0' || path[4] != ':') return -1;
    *name = path + 5;
    return path[2] - '0';
}

static unsigned char* entry(PcPortCard* card, int block)
{
    return frame(card, block);
}

static int find_file(PcPortCard* card, const char* name)
{
    int b;
    if (!name[0]) return 0;
    for (b = 1; b <= DATA_BLOCKS; ++b) {
        unsigned char* e = entry(card, b);
        if (get32(e) == 0x51 &&
            strncmp((const char*)e + 0x0A, name, NAME_MAX_CHARS) == 0 &&
            strlen(name) <= NAME_MAX_CHARS)
            return b;
    }
    return 0;
}

static int next_block(PcPortCard* card, int block)
{
    unsigned char* e = entry(card, block);
    unsigned next = (unsigned)e[8] | ((unsigned)e[9] << 8);
    if (next == 0xFFFF || next >= DATA_BLOCKS) return 0;
    return (int)next + 1;
}

static PcPortCardFd* fd_get(int fd)
{
    if (fd < FD_BASE || fd >= FD_BASE + MAX_FDS) return NULL;
    if (!s_fds[fd - FD_BASE].used) return NULL;
    return &s_fds[fd - FD_BASE];
}

static int create_file(PcPortCard* card, const char* name, int blocks)
{
    int chain[DATA_BLOCKS];
    int count = 0, b, i;
    if (blocks <= 0) blocks = 1;
    for (b = 1; b <= DATA_BLOCKS && count < blocks; ++b)
        if ((get32(entry(card, b)) & 0xF0) == 0xA0) chain[count++] = b;
    if (count < blocks) return 0;
    for (i = 0; i < blocks; ++i) {
        unsigned char* e = entry(card, chain[i]);
        uint32_t state = blocks == 1 || i == 0 ? 0x51 : i == blocks - 1 ? 0x53 : 0x52;
        unsigned next = i + 1 < blocks ? (unsigned)(chain[i + 1] - 1) : 0xFFFFu;
        memset(e, 0, FRAME_BYTES);
        put32(e, state);
        if (i == 0) {
            put32(e + 4, (uint32_t)blocks * BLOCK_BYTES);
            strncpy((char*)e + 0x0A, name, NAME_MAX_CHARS);
        }
        e[8] = (unsigned char)next;
        e[9] = (unsigned char)(next >> 8);
        frame_checksum(e);
    }
    return chain[0];
}

int PcPortCard_open(const char* path, unsigned int mode)
{
    const char* name;
    int port = parse_path(path, &name);
    PcPortCard* card;
    int block, i;
    if (port < 0) {
        card_log("open: not a memory-card path", path);
        return -1;
    }
    card = card_get(port);
    if (!card || !card_formatted(card)) return -1;
    if (!name[0] || strlen(name) > NAME_MAX_CHARS) return -1;
    block = find_file(card, name);
    if (mode & PSX_FCREAT) {
        if (block) return -1;
        block = create_file(card, name, (int)(mode >> 16));
        if (!block) return -1;
        if (card_flush(port) != 0) return -1;
    } else if (!block) {
        return -1;
    }
    for (i = 0; i < MAX_FDS; ++i) {
        if (!s_fds[i].used) {
            s_fds[i].used = 1;
            s_fds[i].port = port;
            s_fds[i].first_block = block;
            s_fds[i].size = (int)get32(entry(card, block) + 4);
            s_fds[i].pos = 0;
            s_fds[i].mode = (int)(mode & 0xFFFF);
            return FD_BASE + i;
        }
    }
    card_log("open: no free file descriptor", path);
    return -1;
}

static int transfer(int fd, unsigned char* buffer, int length, int writing)
{
    PcPortCardFd* f = fd_get(fd);
    PcPortCard* card;
    int block, skip, done = 0;
    if (!f || length <= 0 || (length % FRAME_BYTES) != 0) return -1;
    if (!(f->mode & (writing ? PSX_FWRITE : PSX_FREAD))) return -1;
    if (f->pos + length > f->size) return -1;
    card = card_get(f->port);
    if (!card) return -1;
    block = f->first_block;
    for (skip = f->pos / BLOCK_BYTES; skip > 0 && block; --skip)
        block = next_block(card, block);
    while (done < length) {
        int offset = (f->pos + done) % BLOCK_BYTES;
        int chunk = BLOCK_BYTES - offset;
        unsigned char* data;
        if (!block) {
            card_log("broken block chain", NULL);
            return -1;
        }
        if (chunk > length - done) chunk = length - done;
        data = card->image + block * BLOCK_BYTES + offset;
        if (writing) memcpy(data, buffer + done, (size_t)chunk);
        else memcpy(buffer + done, data, (size_t)chunk);
        done += chunk;
        if (offset + chunk == BLOCK_BYTES) block = next_block(card, block);
    }
    if (writing && card_flush(f->port) != 0) return -1;
    f->pos += length;
    return length;
}

int PcPortCard_read(int fd, void* buffer, int length)
{
    return transfer(fd, (unsigned char*)buffer, length, 0);
}

int PcPortCard_write(int fd, const void* buffer, int length)
{
    return transfer(fd, (unsigned char*)buffer, length, 1);
}

int PcPortCard_close(int fd)
{
    PcPortCardFd* f = fd_get(fd);
    if (!f) return -1;
    f->used = 0;
    return fd;
}

int erase(char* path)
{
    const char* name;
    int port = parse_path(path, &name);
    PcPortCard* card = port < 0 ? NULL : card_get(port);
    int block, guard = 0;
    if (!card || !card_formatted(card)) return 0;
    block = find_file(card, name);
    if (!block) return 0;
    while (block && guard++ < DATA_BLOCKS) {
        unsigned char* e = entry(card, block);
        int next = next_block(card, block);
        put32(e, 0xA0 | (get32(e) & 0x0F));
        frame_checksum(e);
        block = next;
    }
    return card_flush(port) == 0;
}

int PcPortCard_rename(const char* from, const char* to)
{
    const char *old_name, *new_name;
    int port = parse_path(from, &old_name);
    int port2 = parse_path(to, &new_name);
    PcPortCard* card;
    int block;
    unsigned char* e;
    if (port < 0 || port != port2) return 0;
    card = card_get(port);
    if (!card || !card_formatted(card)) return 0;
    if (!new_name[0] || strlen(new_name) > NAME_MAX_CHARS) return 0;
    block = find_file(card, old_name);
    if (!block || find_file(card, new_name)) return 0;
    e = entry(card, block);
    memset(e + 0x0A, 0, NAME_MAX_CHARS + 1);
    strncpy((char*)e + 0x0A, new_name, NAME_MAX_CHARS);
    frame_checksum(e);
    return card_flush(port) == 0;
}

int format(char* path)
{
    const char* name;
    int port = parse_path(path, &name);
    PcPortCard* card = port < 0 ? NULL : card_get(port);
    int i;
    if (!card) return 0;
    for (i = 0; i < MAX_FDS; ++i)
        if (s_fds[i].used && s_fds[i].port == port) s_fds[i].used = 0;
    format_image(card->image);
    return card_flush(port) == 0;
}

static int name_matches(const char* pattern, const char* name)
{
    if (!pattern[0]) return 1;
    for (;; ++pattern, ++name) {
        if (*pattern == '*') return 1;
        if (!*pattern) return !*name;
        if (!*name) return 0;
        if (*pattern != '?' && *pattern != *name) return 0;
    }
}

static struct DIRENTRY* search_from(struct DIRENTRY* dir)
{
    PcPortCard* card;
    int b;
    if (!s_search.active) return NULL;
    card = card_get(s_search.port);
    if (!card || !card_formatted(card)) {
        s_search.active = 0;
        return NULL;
    }
    for (b = s_search.next_block; b <= DATA_BLOCKS; ++b) {
        unsigned char* e = entry(card, b);
        char name[NAME_MAX_CHARS + 1];
        if (get32(e) != 0x51) continue;
        memcpy(name, e + 0x0A, NAME_MAX_CHARS);
        name[NAME_MAX_CHARS] = 0;
        if (!name_matches(s_search.pattern, name)) continue;
        s_search.next_block = b + 1;
        memset(dir, 0, sizeof(*dir));
        memcpy(dir->name, name, sizeof(dir->name) < sizeof(name) ? sizeof(dir->name) : sizeof(name));
        dir->attr = 0x50;
        dir->size = (int)get32(e + 4);
        dir->next = NULL;
        dir->head = b;
        return dir;
    }
    s_search.active = 0;
    return NULL;
}

struct DIRENTRY* firstfile(char* path, struct DIRENTRY* dir)
{
    const char* pattern;
    int port = parse_path(path, &pattern);
    s_search.active = 0;
    if (port < 0 || !dir) return NULL;
    s_search.active = 1;
    s_search.port = port;
    s_search.next_block = 1;
    strncpy(s_search.pattern, pattern, sizeof(s_search.pattern) - 1);
    s_search.pattern[sizeof(s_search.pattern) - 1] = 0;
    return search_from(dir);
}

struct DIRENTRY* nextfile(struct DIRENTRY* dir)
{
    if (!dir) return NULL;
    return search_from(dir);
}

/* ---- card presence / BIOS card driver entry points ---------------------- */

void InitCARD(int val) { (void)val; }
int StartCARD(void) { return 1; }
int StopCARD(void) { return 1; }
void _bu_init(void) {}

static void card_event(int chan, int present)
{
    (void)chan;
    DeliverEvent(SwCARD, present ? EvSpIOE : EvSpTIMOUT);
}

static int chan_port(int chan)
{
    if ((chan & 0x0F) != 0) return -1;
    if (chan == 0x00) return 0;
    if (chan == 0x10) return 1;
    return -1;
}

int _card_info(int chan)
{
    int port = chan_port(chan);
    card_event(chan, port >= 0 && card_get(port) != NULL);
    return 1;
}

int _card_load(int chan)
{
    int port = chan_port(chan);
    card_event(chan, port >= 0 && card_get(port) != NULL);
    return 1;
}

int _card_clear(int chan)
{
    int port = chan_port(chan);
    card_event(chan, port >= 0 && card_get(port) != NULL);
    return 1;
}

int _card_auto(int val) { (void)val; return 1; }
void _new_card(void) {}
int _card_status(int drv) { (void)drv; return 1; }
int _card_wait(int drv) { (void)drv; return 1; }
unsigned int _card_chan(void) { return 0; }

/* Test hook: drop cached images and descriptors (the host test harness uses
 * it to model a fresh boot against the same image directory). */
void PcPortCard_ResetForTest(void)
{
    memset(s_cards, 0, sizeof(s_cards));
    memset(s_fds, 0, sizeof(s_fds));
    memset(&s_search, 0, sizeof(s_search));
}
