/* M2 retail differential kit (menu overlay). Header-only, one test TU.
 *
 * A native C body from src/menu/main/misc.c (XENO_PC_PORT arm) and the retail
 * machine code from disc/menu.bin (SHA pinned by the runner script) run on
 * identical guest memory. The MIPS interpreter is the TEST ORACLE ONLY.
 *
 *  - Guest memory = registered regions (host buffer <-> guest address) plus a
 *    field-wise guest view of the pointer-inflated host SystemMenu. Any other
 *    guest access faults the oracle, so hidden reads/writes cannot pass.
 *  - Callees are spies: the native side calls the spy symbol, the oracle side
 *    is bridged by target address. Both record identical rows (id + args with
 *    host pointers normalized to guest addresses) and get the same scripted
 *    return value / side effect. Any other call target outside the function
 *    under test is unresolved and fails the case.
 *  - Comparison: every registered region byte, the whole host SystemMenu,
 *    the call trace and the return value. Coverage: every instruction of the
 *    retail body must execute at least once across the cases.
 */
#ifndef M2_RETAIL_DIFF_H
#define M2_RETAIL_DIFF_H

#include "battle_mips_adapter.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define M2_CODE_BASE 0x801C5000u
#define M2_CODE_MAX 0x3B000u
#define M2_MENU_VAR 0x800625A0u /* g_Menu */
#define M2_MENU_GUEST 0x80100000u
#define M2_STACK_GUEST 0x801FE000u
#define M2_STACK_SIZE 0x2000u
#define M2_SP 0x801FF800u
#define M2_HALT 0xFFFFFFFCu

typedef struct {
    void *host;
    uint32_t guest, size;
    int readonly, compare;
    const char *name;
} M2Region;

/* M2F_ADDR: address-only (host interior pointer -> guest offset), no access. */
/* M2F_PTR32: a u32 slot holding a (low-memory) host pointer, the port's
 * MenuPsxPointerSlot representation; read/written as its guest address. */
enum { M2F_BYTES, M2F_PTR, M2F_ADDR, M2F_PTR32 };
typedef struct {
    uint32_t off, size;
    void *host;
    int kind; /* M2F_BYTES: raw host bytes; M2F_PTR: host pointer read as its guest address */
} M2Field;

typedef struct {
    uint32_t id, n, w[10];
} M2Ev;

typedef uint32_t (*M2RetFn)(uint32_t id, const uint32_t *args);
typedef struct {
    uint32_t addr;
    const char *name;
    unsigned nargs;
    M2RetFn ret;
    int deref;          /* arg index recorded by pointee content, -1 none */
    unsigned ignore;    /* bitmask of args not recorded (stack out-temps) */
    unsigned derefLen;  /* bytes (<= 64) hashed instead of the address */
} M2Callee;

static uint8_t m2_code[M2_CODE_MAX];
static uint32_t m2_code_size;
static uint8_t m2_stack[M2_STACK_SIZE];
static M2Region m2_regions[48];
static unsigned m2_nregions;
static M2Field m2_fields[512];
static unsigned m2_nfields;
static void *m2_menu_host;
static size_t m2_menu_host_size;
static M2Callee m2_callees[48];
static unsigned m2_ncallees;
static M2Ev m2_evs[512];
static unsigned m2_nev;
static unsigned m2_cases, m2_checks;
static const char *m2_tag = "M2";
static uint32_t m2_entry, m2_end;
static unsigned *m2_cov;

static void m2_fail(const char *why) {
    fprintf(stderr, "%s FAIL cases=%u %s\n", m2_tag, m2_cases, why);
    exit(1);
}
static void m2_require(int ok, const char *why) {
    m2_checks++;
    if (!ok)
        m2_fail(why);
}

static void m2_region(void *host, uint32_t guest, uint32_t size, int compare,
                      const char *name) {
    m2_require(m2_nregions < 48, "region table full");
    m2_regions[m2_nregions].host = host;
    m2_regions[m2_nregions].guest = guest;
    m2_regions[m2_nregions].size = size;
    m2_regions[m2_nregions].readonly = 0;
    m2_regions[m2_nregions].compare = compare;
    m2_regions[m2_nregions].name = name;
    m2_nregions++;
}
/* Host structs reached only through a view (no linear guest image): snapshot
 * and compare their host bytes. */
static struct {
    void *host;
    size_t size;
    const char *name;
} m2_blocks[16];
static unsigned m2_nblocks;
static void m2_hostblock(void *host, size_t size, const char *name) {
    m2_require(m2_nblocks < 16, "block table full");
    m2_blocks[m2_nblocks].host = host;
    m2_blocks[m2_nblocks].size = size;
    m2_blocks[m2_nblocks].name = name;
    m2_nblocks++;
}
static void m2_menu(void *host, size_t size) {
    m2_menu_host = host;
    m2_menu_host_size = size;
}
/* Strict guest views: inside a view only registered fields exist. The host
 * SystemMenu is the view at M2_MENU_GUEST; m2_view adds others (e.g. a
 * pointer-inflated struct embedded in a region). Field `off` is absolute. */
static uint32_t m2_views[16][2] = { { M2_MENU_GUEST, 0x4000u } };
static unsigned m2_nviews = 1;
static void m2_view(uint32_t base, uint32_t span) {
    m2_require(m2_nviews < 16, "view table full");
    m2_views[m2_nviews][0] = base;
    m2_views[m2_nviews][1] = span;
    m2_nviews++;
}
static int m2_in_view(uint32_t a) {
    unsigned i;
    for (i = 0; i < m2_nviews; i++)
        if (a >= m2_views[i][0] && a - m2_views[i][0] < m2_views[i][1])
            return 1;
    return 0;
}
static void m2_vfield(uint32_t gaddr, void *host, uint32_t size, int kind) {
    m2_require(m2_nfields < 512, "field table full");
    m2_fields[m2_nfields].off = gaddr;
    m2_fields[m2_nfields].size = size;
    m2_fields[m2_nfields].host = host;
    m2_fields[m2_nfields].kind = kind;
    m2_nfields++;
}
static void m2_field(uint32_t off, void *host, uint32_t size, int kind) {
    m2_vfield(M2_MENU_GUEST + off, host, size, kind);
}
#define M2_FIELD(off, lval) m2_field((off), &(lval), sizeof(lval), M2F_BYTES)
#define M2_PTRFIELD(off, lval) m2_field((off), &(lval), 4, M2F_PTR)
#define M2_ADDRFIELD(off, lval) m2_field((off), &(lval), sizeof(lval), M2F_ADDR)
#define M2_PTR32FIELD(off, lval) m2_field((off), &(lval), 4, M2F_PTR32)
/* A retail MenuString (0x80) at guest `g` backed by a host MenuString: the
 * host struct is wider (pointer pVramBuffer), so map it field-wise. Caller
 * registers the enclosing m2_view. Needs system/menu.h. */
#define M2_MENUSTRING(g, hs)                                                    \
    do {                                                                        \
        m2_vfield((g), (hs), 0x78u, M2F_BYTES);                                 \
        m2_vfield((g) + 0x78u, &(hs)->pVramBuffer, 4, M2F_PTR);                 \
        m2_vfield((g) + 0x7Cu, &(hs)->unk7C, 4, M2F_BYTES);                     \
    } while (0)

/* Host pointer -> guest address (identity for anything unmapped). */
static uint32_t m2_norm(uintptr_t v) {
    unsigned i;
    for (i = 0; i < m2_nfields; i++) {
        uintptr_t h = (uintptr_t)m2_fields[i].host;
        if (m2_fields[i].kind != M2F_PTR && m2_fields[i].kind != M2F_PTR32 && v >= h &&
            v < h + m2_fields[i].size)
            return m2_fields[i].off + (uint32_t)(v - h);
    }
    for (i = 0; i < m2_nregions; i++) {
        uintptr_t h = (uintptr_t)m2_regions[i].host;
        if (v >= h && v < h + m2_regions[i].size)
            return m2_regions[i].guest + (uint32_t)(v - h);
    }
    if (m2_menu_host && v >= (uintptr_t)m2_menu_host &&
        v < (uintptr_t)m2_menu_host + m2_menu_host_size) {
        if (v == (uintptr_t)m2_menu_host)
            return M2_MENU_GUEST;
        m2_fail("host SystemMenu interior pointer has no guest address");
    }
    return (uint32_t)v;
}
#define M2P(p) m2_norm((uintptr_t)(p))
/* Guest address -> host pointer (for spies returning pointers to native code);
 * unmapped values pass through unchanged. */
static void *m2_denorm(uint32_t g) {
    unsigned i;
    for (i = 0; i < m2_nregions; i++)
        if (g >= m2_regions[i].guest && g - m2_regions[i].guest < m2_regions[i].size)
            return (uint8_t *)m2_regions[i].host + (g - m2_regions[i].guest);
    if (g == M2_MENU_GUEST && m2_menu_host)
        return m2_menu_host;
    return (void *)(uintptr_t)g;
}

static uint8_t *m2_host(uint32_t a, unsigned w, int write) {
    unsigned i;
    for (i = 0; i < m2_nregions; i++) {
        M2Region *r = &m2_regions[i];
        if (a >= r->guest && (uint64_t)a + w <= (uint64_t)r->guest + r->size)
            return (write && r->readonly) ? NULL
                                          : (uint8_t *)r->host + (a - r->guest);
    }
    if (a >= M2_STACK_GUEST && (uint64_t)a + w <= M2_STACK_GUEST + M2_STACK_SIZE)
        return m2_stack + (a - M2_STACK_GUEST);
    if (!write && a >= M2_CODE_BASE && (uint64_t)a + w <= M2_CODE_BASE + m2_code_size)
        return m2_code + (a - M2_CODE_BASE);
    return NULL;
}

static int m2_rd(void *o, uint32_t a, unsigned w, uint32_t *v) {
    uint8_t *p;
    unsigned i;
    (void)o;
    *v = 0;
    if (a == M2_MENU_VAR && w == 4) {
        *v = M2_MENU_GUEST;
        return 0;
    }
    if (m2_in_view(a)) {
        uint32_t off = a;
        for (i = 0; i < m2_nfields; i++) {
            M2Field *f = &m2_fields[i];
            if (f->kind != M2F_ADDR && off >= f->off && off + w <= f->off + f->size) {
                if (f->kind == M2F_PTR || f->kind == M2F_PTR32) {
                    if (off != f->off || w != 4)
                        return -1;
                    *v = f->kind == M2F_PTR ? m2_norm(*(uintptr_t *)f->host)
                                            : m2_norm((uintptr_t)*(uint32_t *)f->host);
                    return 0;
                }
                p = (uint8_t *)f->host + (off - f->off);
                for (i = 0; i < w; i++)
                    *v |= (uint32_t)p[i] << (i * 8);
                return 0;
            }
        }
        fprintf(stderr, "%s unmapped view read 0x%08X w%u\n", m2_tag, off, w);
        return -1;
    }
    p = m2_host(a, w, 0);
    if (!p) {
        fprintf(stderr, "%s unmapped read 0x%08X w%u\n", m2_tag, a, w);
        return -1;
    }
    for (i = 0; i < w; i++)
        *v |= (uint32_t)p[i] << (i * 8);
    return 0;
}

static int m2_wr(void *o, uint32_t a, unsigned w, uint32_t v) {
    uint8_t *p = NULL;
    unsigned i;
    (void)o;
    if (m2_in_view(a)) {
        uint32_t off = a;
        for (i = 0; i < m2_nfields; i++) {
            M2Field *f = &m2_fields[i];
            if (f->kind == M2F_BYTES && off >= f->off && off + w <= f->off + f->size)
                p = (uint8_t *)f->host + (off - f->off);
            if (f->kind == M2F_PTR32 && off == f->off && w == 4) {
                *(uint32_t *)f->host = (uint32_t)(uintptr_t)m2_denorm(v);
                return 0;
            }
            if (f->kind == M2F_PTR && off == f->off && w == 4) {
                *(uintptr_t *)f->host = (uintptr_t)m2_denorm(v);
                return 0;
            }
        }
        if (!p)
            fprintf(stderr, "%s unmapped view write 0x%08X w%u\n", m2_tag, off, w);
    } else {
        p = m2_host(a, w, 1);
        if (!p)
            fprintf(stderr, "%s unmapped write 0x%08X w%u\n", m2_tag, a, w);
    }
    if (!p)
        return -1;
    for (i = 0; i < w; i++)
        p[i] = (uint8_t)(v >> (i * 8));
    return 0;
}

static void m2_load_code(const char *path) {
    FILE *f = fopen(path, "rb");
    m2_require(f != NULL, "open disc/menu.bin");
    m2_code_size = (uint32_t)fread(m2_code, 1, sizeof m2_code, f);
    fclose(f);
    m2_require(m2_code_size > 0x20000u, "read menu.bin");
}
static uint32_t m2_word(uint32_t a) {
    uint32_t v;
    m2_require(a >= M2_CODE_BASE && a + 4 <= M2_CODE_BASE + m2_code_size, "code word range");
    v = (uint32_t)m2_code[a - M2_CODE_BASE] | (uint32_t)m2_code[a - M2_CODE_BASE + 1] << 8 |
        (uint32_t)m2_code[a - M2_CODE_BASE + 2] << 16 |
        (uint32_t)m2_code[a - M2_CODE_BASE + 3] << 24;
    return v;
}
/* Copy retail overlay bytes (data tables) into a host backing buffer. */
static void m2_copy_retail(void *dst, uint32_t guest, uint32_t size) {
    m2_require(guest >= M2_CODE_BASE && guest + size <= M2_CODE_BASE + m2_code_size,
               "retail data range");
    memcpy(dst, m2_code + (guest - M2_CODE_BASE), size);
}

static void m2_callee(uint32_t addr, const char *name, unsigned nargs, M2RetFn ret) {
    m2_require(m2_ncallees < 48, "callee table full");
    m2_callees[m2_ncallees].addr = addr;
    m2_callees[m2_ncallees].name = name;
    m2_callees[m2_ncallees].nargs = nargs;
    m2_callees[m2_ncallees].ret = ret;
    m2_callees[m2_ncallees].deref = -1;
    m2_callees[m2_ncallees].derefLen = 0;
    m2_callees[m2_ncallees].ignore = 0;
    m2_ncallees++;
}
/* Arg `k` of the last registered callee points at a stack temporary (no
 * common guest address): record a hash of its `len` bytes instead. */
static void m2_callee_deref(unsigned k, unsigned len) {
    m2_callees[m2_ncallees - 1].deref = (int)k;
    m2_callees[m2_ncallees - 1].derefLen = len;
}
/* Args in `mask` of the last registered callee are out-pointers to stack
 * temporaries of the caller: recorded as 0 (their writes are not compared). */
static void m2_callee_ignore(unsigned mask) {
    m2_callees[m2_ncallees - 1].ignore = mask;
}
static uint32_t m2_hash(const uint8_t *b, unsigned n) {
    uint32_t h = 2166136261u;
    unsigned i;
    for (i = 0; i < n; i++)
        h = (h ^ b[i]) * 16777619u;
    return h;
}
/* Pins a callee's address to the retail jal at `site`. */
static uint32_t m2_jal_target(uint32_t site) {
    uint32_t ins = m2_word(site);
    m2_require((ins >> 26) == 3, "jal opcode at site");
    return 0x80000000u | ((ins & 0x3FFFFFFu) << 2);
}

/* Every jal in [entry,end) must target a registered callee; no jalr. */
static void m2_scan_calls(uint32_t entry, uint32_t end) {
    uint32_t a, w, t;
    unsigned i;
    for (a = entry; a < end; a += 4) {
        w = m2_word(a);
        m2_require(!((w >> 26) == 0 && (w & 0x3Fu) == 9), "jalr in body");
        if ((w >> 26) != 3)
            continue;
        t = 0x80000000u | ((w & 0x3FFFFFFu) << 2);
        for (i = 0; i < m2_ncallees; i++)
            if (m2_callees[i].addr == t)
                break;
        if (i == m2_ncallees) {
            fprintf(stderr, "%s unregistered callee 0x%08X at 0x%08X\n", m2_tag, t, a);
            m2_fail("unregistered callee");
        }
    }
}

/* Records a call row; returns the scripted value. Shared by spies + bridge. */
static uint32_t m2_call(unsigned idx, const uint32_t *args) {
    M2Callee *c = &m2_callees[idx];
    M2Ev *e;
    unsigned i;
    m2_require(m2_nev < 512, "trace overflow");
    e = &m2_evs[m2_nev++];
    memset(e, 0, sizeof *e);
    e->id = idx;
    e->n = c->nargs;
    for (i = 0; i < c->nargs; i++)
        e->w[i] = (c->ignore >> i) & 1u ? 0u : args[i];
    return c->ret ? c->ret(idx, args) : 0;
}
static unsigned m2_idx(uint32_t addr) {
    unsigned i;
    for (i = 0; i < m2_ncallees; i++)
        if (m2_callees[i].addr == addr)
            return i;
    m2_fail("spy for unregistered callee");
    return 0;
}
/* Native spy helper: M2_SPY(addr, a0, a1, ...); every argument is converted
 * to uintptr_t (pass pointers through M2_U, scalars as u32). */
static uint32_t m2_spyv(uint32_t addr, const uintptr_t *v, unsigned n) {
    uint32_t args[10];
    unsigned i;
    m2_require(n <= 10, "spy arity");
    unsigned idx = m2_idx(addr);
    for (i = 0; i < n; i++)
        args[i] = m2_norm(v[i]);
    if (m2_callees[idx].deref >= 0 && (unsigned)m2_callees[idx].deref < n)
        args[m2_callees[idx].deref] =
            m2_hash((const uint8_t *)v[m2_callees[idx].deref], m2_callees[idx].derefLen);
    return m2_call(idx, args);
}
#define M2_ARGS(...) (sizeof((uintptr_t[]){__VA_ARGS__}) / sizeof(uintptr_t))
#define M2_SPY(addr, ...) m2_spyv((addr), (uintptr_t[]){__VA_ARGS__}, M2_ARGS(__VA_ARGS__))
#define M2_U(x) ((uintptr_t)(x))

static int m2_bridge(void *o, PcPortMipsCpu *c, uint32_t t) {
    unsigned i, k;
    (void)o;
    if (t >= m2_entry && t < m2_end) {
        if (m2_cov)
            m2_cov[(t - m2_entry) / 4]++;
        return 0;
    }
    for (i = 0; i < m2_ncallees; i++) {
        if (m2_callees[i].addr == t) {
            uint32_t args[10];
            for (k = 0; k < m2_callees[i].nargs; k++) {
                if (k < 4)
                    args[k] = c->gpr[4 + k];
                else if (m2_rd(NULL, c->gpr[29] + 0x10u + (k - 4) * 4u, 4, &args[k]) != 0)
                    m2_fail("read stack arg");
            }
            if (m2_callees[i].deref >= 0) {
                uint8_t tmp[64];
                uint32_t v, k2;
                for (k2 = 0; k2 < m2_callees[i].derefLen; k2++) {
                    if (m2_rd(NULL, args[m2_callees[i].deref] + k2, 1, &v) != 0)
                        m2_fail("deref arg read");
                    tmp[k2] = (uint8_t)v;
                }
                args[m2_callees[i].deref] = m2_hash(tmp, m2_callees[i].derefLen);
            }
            c->gpr[2] = m2_call(i, args);
            return 1;
        }
    }
    fprintf(stderr, "%s unresolved target 0x%08X\n", m2_tag, t);
    return -1;
}

/* Runs the retail body with o32 args; returns v0. */
static uint32_t m2_oracle(uint32_t entry, uint32_t end, unsigned *cov,
                          const uint32_t *args, unsigned nargs) {
    PcPortMipsBus bus;
    PcPortMipsCpu c;
    unsigned i;
    int rc;
    memset(&bus, 0, sizeof bus);
    bus.read = m2_rd;
    bus.write = m2_wr;
    bus.bridge = m2_bridge;
    PcPortMipsCpuInit(&c, &bus);
    for (i = 0; i < nargs; i++) {
        if (i < 4)
            c.gpr[4 + i] = args[i];
        else
            m2_wr(NULL, M2_SP + 0x10u + (i - 4) * 4u, 4, args[i]);
    }
    c.gpr[29] = M2_SP;
    c.gpr[31] = M2_HALT;
    m2_entry = entry;
    m2_end = end;
    m2_cov = cov;
    rc = PcPortMipsRun(&c, entry, M2_HALT, 2000000);
    m2_cov = NULL;
    if (rc != PC_PORT_MIPS_HALTED) {
        fprintf(stderr, "%s oracle rc=%d %s pc=0x%08X\n", m2_tag, rc, c.error, c.pc);
        m2_fail("retail execution");
    }
    return c.gpr[2];
}

/* Snapshot of every compared region + the host SystemMenu. */
typedef struct {
    uint8_t *buf;
    size_t len;
} M2Snap;
static size_t m2_snap_len(void) {
    size_t n = m2_menu_host_size;
    unsigned i;
    for (i = 0; i < m2_nregions; i++)
        n += m2_regions[i].size;
    for (i = 0; i < m2_nblocks; i++)
        n += m2_blocks[i].size;
    return n;
}
static void m2_save(M2Snap *s) {
    size_t n = 0;
    unsigned i;
    if (!s->buf) {
        s->len = m2_snap_len();
        s->buf = malloc(s->len);
        m2_require(s->buf != NULL, "snapshot alloc");
    }
    for (i = 0; i < m2_nregions; i++) {
        memcpy(s->buf + n, m2_regions[i].host, m2_regions[i].size);
        n += m2_regions[i].size;
    }
    if (m2_menu_host)
        memcpy(s->buf + n, m2_menu_host, m2_menu_host_size);
    n += m2_menu_host_size;
    for (i = 0; i < m2_nblocks; i++) {
        memcpy(s->buf + n, m2_blocks[i].host, m2_blocks[i].size);
        n += m2_blocks[i].size;
    }
}
static void m2_restore(const M2Snap *s) {
    size_t n = 0;
    unsigned i;
    for (i = 0; i < m2_nregions; i++) {
        memcpy(m2_regions[i].host, s->buf + n, m2_regions[i].size);
        n += m2_regions[i].size;
    }
    if (m2_menu_host)
        memcpy(m2_menu_host, s->buf + n, m2_menu_host_size);
    n += m2_menu_host_size;
    for (i = 0; i < m2_nblocks; i++) {
        memcpy(m2_blocks[i].host, s->buf + n, m2_blocks[i].size);
        n += m2_blocks[i].size;
    }
}
static void m2_compare(const M2Snap *a, const M2Snap *b) {
    size_t n = 0, j;
    unsigned i;
    for (i = 0; i < m2_nregions; i++) {
        if (m2_regions[i].compare) {
            for (j = 0; j < m2_regions[i].size; j++) {
                if (a->buf[n + j] != b->buf[n + j]) {
                    fprintf(stderr, "%s diff %s 0x%08X native=%02X retail=%02X\n", m2_tag,
                            m2_regions[i].name, m2_regions[i].guest + (uint32_t)j,
                            a->buf[n + j], b->buf[n + j]);
                    m2_fail("state mismatch");
                }
            }
        }
        n += m2_regions[i].size;
    }
    for (j = 0; j < m2_menu_host_size; j++) {
        if (a->buf[n + j] != b->buf[n + j]) {
            fprintf(stderr, "%s diff SystemMenu host+0x%zX native=%02X retail=%02X\n", m2_tag,
                    j, a->buf[n + j], b->buf[n + j]);
            m2_fail("menu mismatch");
        }
    }
    n += m2_menu_host_size;
    for (i = 0; i < m2_nblocks; i++) {
        for (j = 0; j < m2_blocks[i].size; j++) {
            if (a->buf[n + j] != b->buf[n + j]) {
                fprintf(stderr, "%s diff %s host+0x%zX native=%02X retail=%02X\n", m2_tag,
                        m2_blocks[i].name, j, a->buf[n + j], b->buf[n + j]);
                m2_fail("host block mismatch");
            }
        }
        n += m2_blocks[i].size;
    }
    m2_checks++;
}

static M2Ev m2_native_evs[512];
static unsigned m2_native_nev;
static void m2_trace_begin(void) {
    m2_nev = 0;
}
static void m2_trace_stash(void) {
    m2_native_nev = m2_nev;
    memcpy(m2_native_evs, m2_evs, sizeof m2_evs);
    m2_nev = 0;
}
static void m2_trace_compare(void) {
    unsigned i, k;
    if (m2_nev != m2_native_nev) {
        fprintf(stderr, "%s trace length native=%u retail=%u\n", m2_tag, m2_native_nev, m2_nev);
    }
    for (i = 0; i < m2_nev && i < m2_native_nev; i++) {
        M2Ev *a = &m2_native_evs[i], *b = &m2_evs[i];
        if (a->id != b->id || memcmp(a->w, b->w, sizeof a->w) != 0) {
            fprintf(stderr, "%s trace row %u native=%s retail=%s\n", m2_tag, i,
                    m2_callees[a->id].name, m2_callees[b->id].name);
            for (k = 0; k < 10; k++)
                if (a->w[k] != b->w[k])
                    fprintf(stderr, "  arg%u native=0x%08X retail=0x%08X\n", k, a->w[k], b->w[k]);
            m2_fail("trace mismatch");
        }
    }
    m2_require(m2_nev == m2_native_nev, "trace length mismatch");
}

static void m2_pattern(void *p, size_t n, unsigned seed) {
    uint8_t *b = p;
    size_t i;
    for (i = 0; i < n; i++) {
        seed = seed * 1664525u + 1013904223u;
        b[i] = (uint8_t)(seed >> 24);
    }
}

static void m2_coverage(const char *name, const unsigned *cov, unsigned n,
                        uint32_t entry, const uint32_t *allow, unsigned nallow) {
    unsigned i, k, missing = 0;
    for (i = 0; i < n; i++) {
        if (cov[i])
            continue;
        for (k = 0; k < nallow; k++)
            if (allow[k] == entry + i * 4)
                break;
        if (k < nallow)
            continue;
        fprintf(stderr, "%s %s uncovered 0x%08X\n", m2_tag, name, entry + i * 4);
        missing++;
    }
    m2_require(missing == 0, "instruction coverage");
}

#endif
