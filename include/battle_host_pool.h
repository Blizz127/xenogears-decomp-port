/* Port bodies of the battle overlay's 20-byte slot pool (retail C in main66.c,
 * retail build only), shared by the port TUs that allocate and release motion
 * records (main65_port.c, main66_port.c, main72_p2_port.c, main73_p2_port.c)
 * so none of them depends on another adopted TU being linked.
 *
 * Pool header (a guest address, e.g. D_800C3D0C): +0 base word (the slot
 * array), +4 first-free halfword, +6 slot count halfword.  A slot is in use
 * while its byte 0 is nonzero.  Everything is a u32 guest word behind BG_*:
 * the base and the slot addresses are in whatever address domain the
 * allocator used, and the slot index is plain u32 arithmetic on them as in
 * retail.
 *
 * Only for XENO_PC_PORT, after battle_host_guest.h. */
#ifndef XENO_BATTLE_HOST_POOL_H
#define XENO_BATTLE_HOST_POOL_H

#ifdef XENO_PC_PORT

/* func_800A2330.s: take the first free slot, 0 when the pool is full (or the
 * first-free slot is in use).  Retail order: the slot test comes before the
 * first-free halfword is bumped, the scan re-reads that halfword every step
 * and reads the base word once. */
static inline u32 bhp_take(u32 pool) {
    u32 first = BG_U16(pool + 4u);
    u32 count = BG_U16(pool + 6u);
    u32 slot;
    u32 base;

    if (!(first < count)) {
        return 0;
    }
    slot = first * 20u + BG_U32(pool);
    if (BG_U8(slot) != 0) {
        return 0;
    }
    first = BG_U16(pool + 4u);
    count = BG_U16(pool + 6u);
    BG_U16(pool + 4u) = (u16)(first + 1u);
    first = BG_U16(pool + 4u);
    if (!(first < count)) {
        return slot;
    }
    base = BG_U32(pool);
    for (;;) {
        if (BG_U8(first * 20u + base) == 0) {
            return slot;
        }
        BG_U16(pool + 4u) = (u16)(BG_U16(pool + 4u) + 1u);
        first = BG_U16(pool + 4u);
        if (!(first < count)) {
            return slot;
        }
    }
}

/* func_800A23E8.s: release a slot (0: nothing, -1).  The index is the u32
 * byte distance from the base over 20; the first-free halfword drops to it
 * when lower, the slot's in-use byte is cleared, and the index is returned. */
static inline s32 bhp_free(u32 pool, u32 slot) {
    u32 index;

    if (slot == 0) {
        return -1;
    }
    index = (slot - BG_U32(pool)) / 20u;
    if ((s32)index < (s32)BG_U16(pool + 4u)) {
        BG_U16(pool + 4u) = (u16)index;
    }
    BG_U8(slot) = 0;
    return (s32)index;
}

#endif /* XENO_PC_PORT */

#endif
