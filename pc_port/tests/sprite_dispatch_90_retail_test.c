/* Opcode 0x90 (dispatch index 6), retail 800210F0..8002113C.
 *
 * Retail compares the sprite's CURRENT animation package (+0x44) with the one
 * at +0x48.  Equal -> bind the package at +0x4C and SET +0xB0 bit 10; not
 * equal -> bind the +0x48 package (the value already in $a1 across the branch)
 * and CLEAR that bit.
 *
 * The binding is observed through func_800222BC's own effects rather than by
 * intercepting it: it is defined in the unit under test, so replacing it would
 * mean not testing the real callee.  It stores the bound package into +0x44 and
 * raises +0x3C bit 30 whenever the package actually changes.
 *
 * The two cases are chosen so that each rejects the other's argument: in the
 * equal case +0x48 still holds the CURRENT package, so a body that passed
 * +0x48 there would rebind nothing and leave +0x44 unchanged; in the not-equal
 * case +0x4C holds a third package, so a body that passed +0x4C would bind the
 * wrong one.  Both flag polarities are asserted, and bit 10 starts set in the
 * clearing case and clear in the setting case.
 */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>

extern void func_8001FBE4(void*, uint32_t, void*);

/* func_8001EE68 and D_800591AD come from the shared no-op guard fixture.
 * D_800591AD is zero there, which keeps func_800222BC out of its battle-only
 * VRAM branch -- state this fixture deliberately does not model. */

static uint8_t* lowmem(size_t n)
{
    uint8_t* q = mmap(NULL, 0x1000, PROT_READ | PROT_WRITE,
                      MAP_PRIVATE | MAP_ANONYMOUS | MAP_32BIT, -1, 0);
    assert(q != MAP_FAILED);
    memset(q, 0, n);
    return q;
}

static uint32_t rd32(const uint8_t* p) { uint32_t v; memcpy(&v, p, 4); return v; }
static void wr32(uint8_t* p, uint32_t v) { memcpy(p, &v, 4); }

/* An animation package FILE: func_80022224 reads relative offsets at +4/+8/+C. */
static uint8_t* make_file(void)
{
    uint8_t* f = lowmem(0x40);
    wr32(f + 0x4, 0x20);
    wr32(f + 0x8, 0x28);
    wr32(f + 0xC, 0x30);
    return f;
}

int main(void)
{
    uint8_t p[0x100], ops[2] = { 0, 0 };
    uint8_t* package = lowmem(0x40);
    uint8_t* fileX = make_file();
    uint8_t* fileY = make_file();
    uint8_t* fileZ = make_file();
    uint32_t X = (uint32_t)(uintptr_t)fileX;
    uint32_t Y = (uint32_t)(uintptr_t)fileY;
    uint32_t Z = (uint32_t)(uintptr_t)fileZ;

    assert(X != Y && Y != Z && X != Z);

    /* Case A: +0x44 == +0x48, so retail binds +0x4C and sets bit 10. */
    memset(p, 0, sizeof p);
    wr32(p + 0x24, (uint32_t)(uintptr_t)package);
    wr32(p + 0x44, X);
    wr32(p + 0x48, X);
    wr32(p + 0x4C, Y);
    wr32(p + 0xB0, 0x1u);              /* bit 10 clear, other bits preserved */
    func_8001FBE4(p, 0x90, ops);
    assert(rd32(p + 0x44) == Y);       /* bound +0x4C, not +0x48 */
    assert((rd32(p + 0x3C) & 0x40000000u) != 0u);
    assert((rd32(p + 0xB0) & 0x400u) != 0u);
    assert((rd32(p + 0xB0) & 0x1u) != 0u);   /* untouched bits survive */

    /* Case B: +0x44 != +0x48, so retail binds +0x48 and clears bit 10. */
    memset(p, 0, sizeof p);
    wr32(p + 0x24, (uint32_t)(uintptr_t)package);
    wr32(p + 0x44, X);
    wr32(p + 0x48, Y);
    wr32(p + 0x4C, Z);
    wr32(p + 0xB0, 0x401u);            /* bit 10 set, plus a bit to preserve */
    func_8001FBE4(p, 0x90, ops);
    assert(rd32(p + 0x44) == Y);       /* bound +0x48, not +0x4C */
    assert((rd32(p + 0x3C) & 0x40000000u) != 0u);
    assert((rd32(p + 0xB0) & 0x400u) == 0u);
    assert((rd32(p + 0xB0) & 0x1u) != 0u);

    /* Case C: not equal, and +0x48 already IS the current package -- retail
     * still takes the clearing path, but func_800222BC rebinds nothing.  This
     * pins that the flag is driven by the +0x44/+0x48 comparison alone and not
     * by whether a rebind happened. */
    memset(p, 0, sizeof p);
    wr32(p + 0x24, (uint32_t)(uintptr_t)package);
    wr32(p + 0x44, X);
    wr32(p + 0x48, 0);                 /* NULL: func_800222BC is a no-op */
    wr32(p + 0x4C, Z);
    wr32(p + 0xB0, 0x400u);
    func_8001FBE4(p, 0x90, ops);
    assert(rd32(p + 0x44) == X);       /* nothing rebound */
    assert((rd32(p + 0x3C) & 0x40000000u) == 0u);
    assert((rd32(p + 0xB0) & 0x400u) == 0u);   /* flag cleared regardless */

    puts("SPRITE 90 PASS bind source/flag polarity/no-rebind path");
    return 0;
}
