/* CPU-only provenance fixture. Links the actual helper, staged model hook,
 * and actual domain-aware OT linker. No GL, SDL, retail assets, or CPU VM. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "guest_prim_link.h"
#include "hd2d_world_packets.h"
#include "model_prim_link.h"
#include "psx_memory.h"

uint8_t g_PsxRam[PSX_RAM_SIZE];

typedef union Packet {
    uint32_t word[13];
    unsigned char byte[52];
} Packet;

static int opening = 1;
static int current_map = 14;
static uint64_t scene_epoch = UINT64_C(1);
static unsigned checks;
static unsigned allocation_calls;
static unsigned release_calls;
static unsigned live_blocks;
static int fail_allocation;

/* Observe the actual helper allocation without production test hooks. The
 * overflow fixture's independent packet allocation uses __real_* below. */
void *__real_calloc(size_t count, size_t bytes);
void __real_free(void *ptr);
void *__wrap_calloc(size_t count, size_t bytes)
{
    void *ptr;
    ++allocation_calls;
    if (fail_allocation) return NULL;
    ptr = __real_calloc(count, bytes);
    if (ptr) ++live_blocks;
    return ptr;
}
void __wrap_free(void *ptr)
{
    if (ptr) {
        assert(live_blocks);
        --live_blocks;
        ++release_calls;
    }
    __real_free(ptr);
}

uint64_t PcPort_Hd2dSceneEpoch(void) { return scene_epoch; }
int PcPort_Hd2dOpeningActive(void) { return opening; }
int PcPort_Hd2dCurrentMap(void) { return current_map; }

static void check(const char *name, int condition)
{
    ++checks;
    if (!condition) {
        fprintf(stderr, "ASSERTION %s\n", name);
        exit(1);
    }
}

static Packet packet(unsigned code, unsigned words)
{
    Packet p;
    memset(&p, 0x35, sizeof p);
    p.word[0] = (uint32_t)(words << 24) | UINT32_C(0x123456);
    p.byte[7] = (unsigned char)code;
    return p;
}

static void fresh(void)
{
    opening = 1;
    current_map = 14;
    ++scene_epoch;
    PcPort_Hd2dWorldReset();
    PcPort_PrimLinkReset();
    check("fresh-valid", PcPort_Hd2dWorldValid());
}

static void submit(Packet *p, u32 *ot)
{
    const u32 length = p->word[0] & UINT32_C(0xff000000);
    PcPort_Hd2dWorldBegin(7, 7);
    PcPort_LinkModelPrim(ot, 0, p, length);
    PcPort_Hd2dWorldEnd();
}

static unsigned classify(Packet *p, int overridden)
{
    PcPort_Hd2dWorldPacketSeen(p);
    return PcPort_Hd2dWorldPacketClassify(p, p->byte[7], overridden);
}

static void allocation_checks(void)
{
    Packet p = packet(0x2c, 9);
    u32 ot = UINT32_C(0xaa005432);
    unsigned calls;
    unsigned frees;
    uint64_t generation;
    opening = 0;
    PcPort_Hd2dWorldReset();
    calls = allocation_calls;
    generation = PcPort_Hd2dWorldGeneration();
    PcPort_Hd2dWorldBegin(7, 7);
    PcPort_Hd2dWorldPacketLinked((const void *)(uintptr_t)1);
    PcPort_Hd2dWorldPacketSeen((const void *)(uintptr_t)1);
    check("disabled-no-packet-read",
          PcPort_Hd2dWorldPacketClassify((const void *)(uintptr_t)1, 0x2c, 0) == 0u);
    PcPort_Hd2dWorldEnd();
    PcPort_Hd2dWorldGpuWalkDone();
    check("disabled-no-allocation", allocation_calls == calls && !live_blocks);
    opening = 1;
    current_map = 13;
    check("out-of-scope-no-allocation",
          !PcPort_Hd2dWorldValid() && allocation_calls == calls);

    fail_allocation = 1;
    current_map = 14;
    check("allocation-failure-invalid", !PcPort_Hd2dWorldValid());
    check("one-failed-attempt", allocation_calls == calls + 1u && !live_blocks);
    check("failure-invalidates-generation", generation != PcPort_Hd2dWorldGeneration());
    generation = PcPort_Hd2dWorldGeneration();
    submit(&p, &ot);
    Packet expected = packet(0x2c, 9);
    expected.word[0] = UINT32_C(0x09005432);
    check("allocation-failure-preserves-real-link",
          !memcmp(&p, &expected, sizeof p) &&
          ot == (UINT32_C(0xaa000000) | ((u32)(uintptr_t)&p & UINT32_C(0xffffff))));
    check("allocation-failure-cannot-certify", classify(&p, 0) == 0u);
    fail_allocation = 0;
    for (unsigned i = 0; i < 8u; ++i) {
        PcPort_Hd2dWorldBegin(7, 7);
        PcPort_Hd2dWorldPacketLinked((const void *)(uintptr_t)1);
        PcPort_Hd2dWorldPacketSeen((const void *)(uintptr_t)1);
        check("failed-storage-no-unknown-pointer-read",
              PcPort_Hd2dWorldPacketClassify((const void *)(uintptr_t)1, 0x2c, 0) == 0u);
        PcPort_Hd2dWorldEnd();
        PcPort_Hd2dWorldGpuWalkDone();
        check("allocation-failure-stays-latched",
              !PcPort_Hd2dWorldValid() && allocation_calls == calls + 1u &&
              generation == PcPort_Hd2dWorldGeneration());
    }
    ++scene_epoch;
    check("new-epoch-retries-once",
          PcPort_Hd2dWorldValid() && allocation_calls == calls + 2u && live_blocks == 1u);
    check("recovery-does-not-resurrect-dropped-token", classify(&p, 0) == 0u);
    submit(&p, &ot);
    check("new-registration-after-oom-positive", classify(&p, 0) == 2u);
    PcPort_Hd2dWorldGpuWalkDone();
    submit(&p, &ot);
    generation = PcPort_Hd2dWorldGeneration();
    ++scene_epoch;
    check("epoch-reuses-storage-clears-pending",
          PcPort_Hd2dWorldValid() && allocation_calls == calls + 2u &&
          classify(&p, 0) == 0u && generation != PcPort_Hd2dWorldGeneration());

    frees = release_calls;
    opening = 0;
    PcPort_Hd2dWorldReset();
    PcPort_Hd2dWorldReset();
    check("reset-releases-once", !live_blocks && release_calls == frees + 1u);
    check("reset-while-off-does-not-reallocate",
          !PcPort_Hd2dWorldValid() && allocation_calls == calls + 2u);
    opening = 1;
    fail_allocation = 1;
    check("allocation-can-fail-after-reset", !PcPort_Hd2dWorldValid());
    fail_allocation = 0;
    PcPort_Hd2dWorldReset();
    check("explicit-reset-recovers-oom",
          PcPort_Hd2dWorldValid() && live_blocks == 1u &&
          allocation_calls == calls + 4u);
    PcPort_Hd2dWorldReset();
    for (unsigned transition = 0; transition < 2u; ++transition) {
        const uint64_t unchanged_epoch = scene_epoch;
        calls = allocation_calls;
        fail_allocation = 1;
        check("scope-recovery-starts-failed", !PcPort_Hd2dWorldValid());
        fail_allocation = 0;
        if (transition == 0) {
            current_map = 1; /* Eligible map change without an epoch change. */
        } else {
            opening = 0;
            check("inactive-change-does-not-retry",
                  !PcPort_Hd2dWorldValid() && allocation_calls == calls + 1u);
            opening = 1;
        }
        check("same-epoch-scope-change-retries-once",
              PcPort_Hd2dWorldValid() && scene_epoch == unchanged_epoch &&
              allocation_calls == calls + 2u && live_blocks == 1u);
        check("scope-recovery-does-not-resurrect-token", classify(&p, 0) == 0u);
        PcPort_Hd2dWorldReset();
    }
    current_map = 14;
}

static void shape_and_link_checks(void)
{
    static const unsigned code[] = {0x20,0x24,0x28,0x2c,0x30,0x34,0x38,0x3c};
    static const unsigned words[] = {4,7,5,9,6,9,8,12};
    unsigned shape;
    unsigned raw;
    for (shape = 0; shape < sizeof code / sizeof code[0]; ++shape) {
        for (raw = 0; raw < 2u; ++raw) {
            Packet p = packet(code[shape] | raw, words[shape]);
            Packet expected = p;
            u32 ot = UINT32_C(0xaa00abcd);
            uint64_t generation;
            fresh();
            expected.word[0] = (p.word[0] & UINT32_C(0xff000000)) |
                               UINT32_C(0xabcd);
            submit(&p, &ot);
            check("real-native-domain-link", PcPort_PrimLinkNativeCount() == 1);
            check("real-ot-publication",
                  ot == (UINT32_C(0xaa000000) |
                         ((u32)(uintptr_t)&p & UINT32_C(0xffffff))));
            check("registration-only-retail-link-write",
                  !memcmp(&p, &expected, sizeof p));
            generation = PcPort_Hd2dWorldGeneration();
            check("opaque-polygon-positive", classify(&p, 0) == 2u);
            PcPort_Hd2dWorldGpuWalkDone();
            check("classification-retirement-no-packet-write",
                  !memcmp(&p, &expected, sizeof p));
            check("normal-walk-generation-stable",
                  generation == PcPort_Hd2dWorldGeneration());
            check("walk-retired", classify(&p, 0) == 0u);
        }
    }
    for (shape = 0; shape < sizeof code / sizeof code[0]; ++shape) {
        Packet p = packet(code[shape] | 2u, words[shape]);
        u32 ot = 0;
        fresh();
        submit(&p, &ot);
        check("all-translucent-polygons-protected", classify(&p, 0) == 0u);
        check("translucent-not-global-failure", PcPort_Hd2dWorldValid());
        PcPort_Hd2dWorldGpuWalkDone();
    }
    {
        Packet p = packet(0x2c, 9);
        Packet expected = p;
        u32 *guest_ot = (u32 *)PSX_ADDR(UINT32_C(0x80001000));
        Packet *guest_packet = (Packet *)PSX_ADDR(UINT32_C(0x80002000));
        fresh();
        *guest_ot = UINT32_C(0xaa005432);
        *guest_packet = p;
        expected.word[0] = UINT32_C(0x09005432);
        submit(guest_packet, guest_ot);
        check("real-guest-domain-link", PcPort_PrimLinkGuestCount() == 1);
        check("guest-packet-publication", *guest_ot == UINT32_C(0xaa002000));
        check("guest-postlink-snapshot", classify(guest_packet, 0) == 2u);
        check("guest-no-extra-packet-writes",
              !memcmp(guest_packet, &expected, sizeof expected));
        PcPort_Hd2dWorldGpuWalkDone();
        fresh();
        *guest_ot = UINT32_C(0xaa005432);
        submit(&p, guest_ot);
        check("rejected-domain-link", PcPort_PrimLinkRejectCount() == 1);
        check("rejected-domain-ot-unchanged", *guest_ot == UINT32_C(0xaa005432));
        check("rejected-domain-not-registered", classify(&p, 0) == 0u);
    }
}

static void map4_scope_checks(void)
{
    /* The audited establishing scene has scripts0..13 and one static model
     * actor14. This tests the existing threshold contract, not actor identity
     * as a substitute for exact packet registration. */
    for (int actor = 0; actor <= 14; ++actor) {
        fresh();
        current_map = 4;
        Packet p = packet(0x2c, 9);
        u32 ot = 0;
        PcPort_Hd2dWorldBegin(actor, 14);
        PcPort_LinkModelPrim(&ot, 0, &p, UINT32_C(0x09000000));
        PcPort_Hd2dWorldEnd();
        Packet copy = p;
        check("map4-real-model-link", PcPort_PrimLinkNativeCount() == 1);
        check("map4-copy-not-provenance", classify(&copy, 0) == 0u);
        check("map4-only-nonscripted-boundary-eligible",
              classify(&p, 0) == (actor == 14 ? 2u : 0u));
        check("map4-classification-does-not-change-packet", !memcmp(&p, &copy, sizeof p));
        PcPort_Hd2dWorldGpuWalkDone();
        check("map4-retired-packet-protected", classify(&p, 0) == 0u);
    }
    for (int protection = 0; protection < 8; ++protection) {
        fresh();
        current_map = protection == 7 ? 3 : 4;
        if (protection == 4) opening = 0;
        Packet p = packet(protection == 0 ? 0x2eu : protection == 2 ? 0x64u : 0x2cu,
                          protection == 2 ? 4u : 9u);
        u32 ot = 0;
        if (protection != 3) PcPort_Hd2dWorldBegin(14, 14);
        PcPort_LinkModelPrim(&ot, 0, &p, p.word[0] & UINT32_C(0xff000000));
        if (protection != 3) PcPort_Hd2dWorldEnd();
        if (protection == 5) p.byte[8] ^= 1u;
        if (protection == 6) {
            PcPort_Hd2dWorldBegin(13, 14);
            PcPort_LinkModelPrim(&ot, 0, &p, UINT32_C(0x09000000));
            PcPort_Hd2dWorldEnd();
        }
        const Packet before = p;
        check("map4-translucent-override-UI-unknown-inactive-mutation-reuse-protected",
              classify(&p, protection == 1) == 0u);
        check("map4-protection-does-not-change-packet", !memcmp(&p, &before, sizeof p));
        PcPort_Hd2dWorldGpuWalkDone();
    }
}

static void scope_and_unknown_checks(void)
{
    Packet p;
    Packet before;
    u32 ot;
    int which;
    for (which = 0; which < 6; ++which) {
        fresh();
        p = packet(0x2c, 9);
        before = p;
        ot = 0;
        if (which == 0) opening = 0;
        if (which == 1) current_map = 13;
        if (which != 2)
            PcPort_Hd2dWorldBegin(which == 3 ? 6 : 7,
                                 which == 4 ? -1 : 7);
        if (which == 5) PcPort_Hd2dWorldEnd();
        PcPort_LinkModelPrim(&ot, 0, &p, UINT32_C(0x09000000));
        if (which != 2 && which != 5) PcPort_Hd2dWorldEnd();
        check("unauthorized-world-scope-protected", classify(&p, 0) == 0u);
        before.word[0] = UINT32_C(0x09000000);
        check("scope-rejection-only-retail-link-writes",
              !memcmp(&p, &before, sizeof p));
    }
    for (int verified_map = 1; verified_map <= 2; ++verified_map) {
        fresh();
        current_map = verified_map;
        p = packet(0x2c, 9);
        ot = 0;
        submit(&p, &ot);
        before = p;
        check("verified-map-copy-is-not-provenance", classify(&before, 0) == 0u);
        check("verified-map-exact-packet-positive", classify(&p, 0) == 2u);
        PcPort_Hd2dWorldGpuWalkDone();
        PcPort_Hd2dWorldBegin(6, 7);
        PcPort_LinkModelPrim(&ot, 0, &p, UINT32_C(0x09000000));
        PcPort_Hd2dWorldEnd();
        check("verified-map-scripted-actor-stays-protected", classify(&p, 0) == 0u);
        PcPort_Hd2dWorldGpuWalkDone();
    }

    fresh();
    opening = 0;
    PcPort_Hd2dWorldBegin(7, 7);
    PcPort_Hd2dWorldPacketLinked((const void *)(uintptr_t)1);
    PcPort_Hd2dWorldPacketSeen((const void *)(uintptr_t)1);
    check("inactive-hooks-do-not-read-packets",
          PcPort_Hd2dWorldPacketClassify((const void *)(uintptr_t)1, 0x2c, 0) == 0u);
    PcPort_Hd2dWorldEnd();
    PcPort_Hd2dWorldGpuWalkDone();

    fresh();
    /* Unknown pointers, including a pointer sharing the low24 bits with a
     * registered packet, must return before any packet byte is read. */
    p = packet(0x2c, 9);
    ot = 0;
    submit(&p, &ot);
    {
        const void *different_full_pointer =
            (const void *)((uintptr_t)&p ^ (UINTPTR_MAX > UINT32_MAX ?
                                          UINT64_C(0x100000000) :
                                          UINT64_C(0x1000000)));
        PcPort_Hd2dWorldPacketSeen(different_full_pointer);
        check("full-pointer-identity-not-low24",
              PcPort_Hd2dWorldPacketClassify(different_full_pointer, 0x2c, 0) == 0u);
        PcPort_Hd2dWorldPacketSeen((const void *)(uintptr_t)1);
        check("unknown-not-dereferenced",
              PcPort_Hd2dWorldPacketClassify((const void *)(uintptr_t)1, 0x2c, 0) == 0u);
        check("null-protected", PcPort_Hd2dWorldPacketClassify(NULL, 0x2c, 0) == 0u);
    }
    before = p; /* Identical bytes at a different full address are unknown. */
    check("copy-not-provenance", classify(&before, 0) == 0u);
    check("unknown-cannot-poison-valid-world", classify(&p, 0) == 2u);
    PcPort_Hd2dWorldGpuWalkDone();

    for (which = 0; which < 4; ++which) {
        fresh();
        p = packet(which == 0 ? 0x64u : 0x2cu,
                   which == 1 ? 8u : which == 2 ? 0u : which == 3 ? 255u : 9u);
        ot = 0;
        submit(&p, &ot);
        check("nonpolygon-or-invalid-length-protected", classify(&p, 0) == 0u);
    }
    fresh();
    p = packet(0x2c, 9);
    ot = 0;
    submit(&p, &ot);
    check("all-overrides-protected", classify(&p, 1) == 0u);
    check("override-not-global-failure", PcPort_Hd2dWorldValid());
    PcPort_Hd2dWorldGpuWalkDone();
    check("overridden-packet-retired", classify(&p, 0) == 0u);
}

static void lifetime_checks(void)
{
    Packet previous = packet(0x24, 7);
    Packet next = packet(0x2c, 9);
    u32 ot_a = 0;
    u32 ot_b = 0;
    uint64_t generation;
    fresh();
    submit(&previous, &ot_a);
    submit(&next, &ot_b);
    generation = PcPort_Hd2dWorldGeneration();
    PcPort_Hd2dWorldGpuWalkDone(); /* Unrelated empty OT / prior presentation. */
    check("queued-ot-survives-unrelated-walk", classify(&previous, 0) == 2u);
    PcPort_Hd2dWorldGpuWalkDone();
    check("parsed-ot-is-retired", classify(&previous, 0) == 0u);
    check("future-ot-survives-prior-retirement", classify(&next, 0) == 2u);
    PcPort_Hd2dWorldGpuWalkDone();
    check("ordinary-lifetime-keeps-generation",
          generation == PcPort_Hd2dWorldGeneration());
    /* Reusing a pointer after its actual completed walk is a new token. */
    next.byte[8] ^= 0x20u;
    submit(&next, &ot_b);
    check("retired-storage-can-be-reused", classify(&next, 0) == 2u);
    PcPort_Hd2dWorldGpuWalkDone();

    fresh();
    submit(&previous, &ot_a);
    PcPort_Hd2dWorldPacketSeen(&previous); /* Culled / skipped before Classify. */
    PcPort_Hd2dWorldGpuWalkDone();
    check("seen-skipped-packet-retired", classify(&previous, 0) == 0u);

    for (int transition = 0; transition < 4; ++transition) {
        fresh();
        submit(&previous, &ot_a);
        generation = PcPort_Hd2dWorldGeneration();
        if (transition == 0) ++scene_epoch;
        if (transition == 1) current_map = 1;
        if (transition == 2) opening = 0;
        if (transition == 3) PcPort_Hd2dWorldReset();
        check("lifecycle-invalidates-generation",
              generation != PcPort_Hd2dWorldGeneration());
        check("lifecycle-retires-stale-permission", classify(&previous, 0) == 0u);
        opening = 1;
        check("reenabling-does-not-resurrect-token", classify(&previous, 0) == 0u);
    }
}

static void mutation_checks(void)
{
    unsigned offset;
    /* Every byte of the largest legal packet matters, including tag/link,
     * command, geometry, colour, UV, CLUT, tpage, and padding. */
    for (offset = 0; offset < sizeof(Packet); ++offset) {
        Packet p = packet(0x3c, 12);
        u32 ot = 0;
        uint64_t generation;
        fresh();
        submit(&p, &ot);
        generation = PcPort_Hd2dWorldGeneration();
        p.byte[offset] ^= 1u;
        PcPort_Hd2dWorldPacketSeen(&p);
        check("every-snapshot-byte-protects",
              PcPort_Hd2dWorldPacketClassify(&p, 0x3c, 0) == 0u);
        check("mutation-invalidates-whole-pending-generation",
              !PcPort_Hd2dWorldValid() &&
              generation != PcPort_Hd2dWorldGeneration());
        p.byte[offset] ^= 1u;
        PcPort_Hd2dWorldGpuWalkDone();
    }
    {
        Packet p = packet(0x2c, 9);
        Packet q = packet(0x20, 4);
        u32 ot = 0;
        uint64_t accepted_generation;
        fresh();
        submit(&p, &ot);
        submit(&q, &ot);
        accepted_generation = PcPort_Hd2dWorldGeneration();
        check("positive-before-late-mutation", classify(&p, 0) == 2u);
        p.byte[19] ^= 1u;
        PcPort_Hd2dWorldGpuWalkDone();
        check("post-classify-mutation-invalidates-earlier-positive",
              !PcPort_Hd2dWorldValid() &&
              accepted_generation != PcPort_Hd2dWorldGeneration());
        check("unseen-queued-world-cannot-escape-poison", classify(&q, 0) == 0u);
        PcPort_Hd2dWorldGpuWalkDone();
        check("drained-nonoverflow-can-recover", PcPort_Hd2dWorldValid());
        check("recovery-cannot-revalidate-old-offer",
              accepted_generation != PcPort_Hd2dWorldGeneration());
    }
}

static void ambiguity_checks(void)
{
    for (int ambiguity = 0; ambiguity < 8; ++ambiguity) {
        Packet p = packet(0x2c, 9);
        Packet q = packet(0x24, 7);
        u32 ot = 0;
        uint64_t generation;
        fresh();
        submit(&p, &ot);
        submit(&q, &ot);
        generation = PcPort_Hd2dWorldGeneration();
        if (ambiguity == 0) PcPort_Hd2dWorldPacketLinked(&p); /* Outside scope. */
        if (ambiguity == 1) submit(&p, &ot); /* Same-address duplicate link. */
        if (ambiguity == 2) {
            p.byte[8] ^= 0x40u;
            submit(&p, &ot); /* Changed storage reused while still pending. */
        }
        if (ambiguity == 3) {
            PcPort_Hd2dWorldPacketSeen(&p);
            PcPort_Hd2dWorldPacketSeen(&p);
        }
        if (ambiguity == 4) {
            check("positive-before-repeat-classify", classify(&p, 0) == 2u);
            check("repeat-classify-protected",
                  PcPort_Hd2dWorldPacketClassify(&p, 0x2c, 0) == 0u);
        }
        if (ambiguity == 5) {
            PcPort_Hd2dWorldPacketSeen(&p);
            check("actual-command-must-match-snapshot",
                  PcPort_Hd2dWorldPacketClassify(&p, 0x2e, 0) == 0u);
        }
        if (ambiguity == 6) {
            PcPort_Hd2dWorldBegin(7, 7);
            PcPort_Hd2dWorldBegin(6, 7);
            PcPort_Hd2dWorldEnd();
            PcPort_Hd2dWorldEnd();
        }
        if (ambiguity == 7)
            check("classify-requires-parser-seen",
                  PcPort_Hd2dWorldPacketClassify(&p, 0x2c, 0) == 0u);
        check("ambiguity-invalidates-generation",
              !PcPort_Hd2dWorldValid() &&
              generation != PcPort_Hd2dWorldGeneration());
        check("other-pending-positive-also-protected", classify(&q, 0) == 0u);
        PcPort_Hd2dWorldGpuWalkDone();
    }
}

static void overflow_checks(void)
{
    const size_t count = (size_t)PCPORT_HD2D_WORLD_PACKET_CAPACITY + 1u;
    Packet *packets = __real_calloc(count, sizeof *packets);
    u32 ot = 0;
    uint64_t accepted_generation;
    size_t i;
    check("fixture-overflow-allocation", packets != NULL);
    fresh();
    for (i = 0; i < count; ++i) packets[i] = packet(0x20, 4);
    submit(packets, &ot);
    accepted_generation = PcPort_Hd2dWorldGeneration();
    check("positive-before-overflow", classify(packets, 0) == 2u);
    for (i = 1; i < count; ++i) submit(packets + i, &ot);
    check("overflow-invalidates-earlier-positive",
          !PcPort_Hd2dWorldValid() &&
          accepted_generation != PcPort_Hd2dWorldGeneration());
    check("overflow-does-not-only-drop-last",
          classify(packets + 1, 0) == 0u);
    check("overflow-dropped-packet-protected",
          classify(packets + count - 1u, 0) == 0u);
    for (i = 2; i + 1u < count; ++i)
        PcPort_Hd2dWorldPacketSeen(packets + i);
    PcPort_Hd2dWorldGpuWalkDone();
    check("overflow-stays-invalid-after-tracked-drain", !PcPort_Hd2dWorldValid());
    ++scene_epoch;
    check("new-epoch-recovers-overflow", PcPort_Hd2dWorldValid());
    check("new-epoch-cannot-resurrect-dropped-token",
          classify(packets + count - 1u, 0) == 0u);
    submit(packets, &ot);
    check("fresh-registration-after-reset-positive", classify(packets, 0) == 2u);
    PcPort_Hd2dWorldGpuWalkDone();
    __real_free(packets);
}

int main(void)
{
    allocation_checks();
    shape_and_link_checks();
    map4_scope_checks();
    scope_and_unknown_checks();
    lifetime_checks();
    mutation_checks();
    ambiguity_checks();
    overflow_checks();
    PcPort_Hd2dWorldReset();
    check("fixture-leaves-no-metadata-allocation", !live_blocks);
    printf("HD2D world provenance PASS checks=%u; real link, exact identity, "
           "scope, protection, queued lifetime, late invalidation, overflow\n", checks);
    return 0;
}
