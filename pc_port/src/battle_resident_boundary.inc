/* ABI boundaries for a guest call into the resident executable. Direct calls
 * made by native field, world and renderer code keep their host ABI. */
static int battle_resident_guest_pointer(uint32_t address)
{
    return address < BATTLE_BUS_KUSEG_RAM_END ||
           (address & 0xff800000u) == 0x80000000u ||
           (address & 0xff800000u) == 0xa0000000u ||
           (address & 0xfffff000u) == 0x1f800000u;
}

/* A table starts with its entry count followed by that many 32-bit offsets.
 * The caller consumes addresses in the same domain as its table pointer. */
static int battle_resident_relocate_archive(BattleMipsRuntime *runtime, PcPortMipsCpu *cpu)
{
    const uint32_t base = cpu->gpr[4];
    if (base == 0 || (base & 3u) || (cpu->load_valid && cpu->load_reg != 0))
        return -1;

    const uint8_t *header = module_memory_pointer(runtime, base, 4, 0);
    if (!header) return -1;
    const uint32_t count = load_le(header, 4);
    const uint64_t bytes = ((uint64_t)count + 1) * 4;
    if (bytes > UINT32_MAX || bytes - 1 > UINT32_MAX - base)
        return -1;

    /* A native table remains the responsibility of its normal typed host
     * owner. Validate the complete range before the owner reads the count. */
    if (!battle_resident_guest_pointer(base)) {
        if (!host_address_valid(runtime, base, (unsigned)bytes, count != 0))
            return -1;
        return BRIDGE_HOOKS_NONE;
    }

    if (count) {
        uint8_t *table = module_memory_pointer(runtime, base, (unsigned)bytes, 1);
        if (!table || table != header) return -1;
        /* Shared guest globals can map to distinct native allocations, even
         * when adjacent guest words have consecutive addresses. Require one
         * contiguous archive before making the first change. */
        for (uint32_t i = 1; i <= count; ++i) {
            const uint32_t offset = i * 4u;
            if (module_memory_pointer(runtime, base + offset, 4, 1) != table + offset ||
                module_memory_pointer(runtime, base + offset, 4, 0) != table + offset)
                return -1;
        }
        runtime_note_store(runtime, table + 4, (unsigned)bytes - 4);
        for (uint32_t i = 1; i <= count; ++i) {
            uint8_t *entry = table + i * 4u;
            store_le(entry, 4, base + load_le(entry, 4));
        }
    }
    cpu->gpr[2] = count;
    cpu_meter_note_kind(CPU_METER_KIND_GENERIC);
    return 1;
}

#ifndef XENO_NATIVE_ONLY
#include "retail_data.h"

typedef struct BattleModelPacketRun {
    BattleMipsRuntime *runtime;
    const uint8_t *image;
    uint32_t previous_pc;
    uint32_t fetch_pc;
    unsigned draw_variant;
} BattleModelPacketRun;

/* Reachable instruction intervals of the US resident dispatcher, the build
 * callbacks in its 17 packed rows, and their math and packet helpers.
 * No resident instructions are copied into writable emulated RAM. */
static int battle_model_packet_pc(uint32_t pc)
{
    static const struct { uint32_t begin, end; } ranges[] = {
        {0x8002c8ccu, 0x8002cb54u}, {0x8002ccacu, 0x8002d354u},
        {0x8002d530u, 0x8002dd20u}, {0x80043a1cu, 0x80043a70u},
        {0x80043c24u, 0x80043c4cu}, {0x80043c60u, 0x80043c74u},
        {0x80048c4cu, 0x80048cd0u}, {0x80048d68u, 0x80048da8u},
        {0x80048dbcu, 0x80048e94u}, {0x8004a19cu, 0x8004a1f4u},
        {0x8004a260u, 0x8004a2c4u}, {0x8004a4d8u, 0x8004a530u}
    };
    for (unsigned i = 0; i < sizeof(ranges) / sizeof(ranges[0]); ++i)
        if (pc >= ranges[i].begin && pc < ranges[i].end)
            return (pc & 3u) == 0;
    return 0;
}

static int battle_model_packet_bridge(void *opaque, PcPortMipsCpu *cpu, uint32_t target)
{
    BattleModelPacketRun *run = opaque;
    const uint32_t previous = run->previous_pc;
    run->previous_pc = target;
    if (previous == 0x8002ca98u) {
        /* The dispatcher's single indirect call must use a build entry from
         * the verified, packed disc table, never a process callback. */
        const uint8_t *row = run->image + XENO_RD_SLUS_OFF(0x8004fe50u);
        unsigned i = 0;
        while (i < 17 && load_le(row + i * 40 + 24, 4) != target) ++i;
        if (i == 17) return -1;
    }
    if (battle_model_packet_pc(target)) {
        run->fetch_pc = target;
        return 0;
    }
    /* These two calls allocate optional lighting in the port's actual heap. */
    if ((target == 0x800324b8u && previous == 0x8002c924u) ||
        (target == 0x80031bdcu && previous == 0x8002c930u)) {
        const ResolvedFunction *owner = find_function(run->runtime, target);
        if (owner && owner->host && !xeno_port_is_generated_stub(owner->name))
            return bridge_heap_services(run->runtime, cpu, owner);
    }
    return -1;
}

static int battle_model_packet_read(void *opaque, uint32_t address, unsigned width,
                                    uint32_t *value)
{
    BattleModelPacketRun *run = opaque;
    if (run->fetch_pc == address && width == 4 && run->fetch_pc != 0) {
        run->fetch_pc = 0;
        *value = load_le(run->image + XENO_RD_SLUS_OFF(address), 4);
        return 0;
    }
    return runtime_read(run->runtime, address, width, value);
}

/* Resolve before checking overlap so physical mirrors and compact host
 * pointers into g_PsxRam receive the same treatment. */
static int battle_model_packet_span(BattleMipsRuntime *runtime, uint32_t base,
                                    uint32_t size, int writable)
{
    if (!base || !size || size - 1 > UINT32_MAX - base) return 0;
    const uint8_t *p = module_memory_pointer(runtime, base, size, writable);
    if (!p) return 0;
    if (writable) {
        const uintptr_t start = (uintptr_t)p, end = start + size;
        if (start < (uintptr_t)PSX_ADDR(PSX_EXE_SDATA_START) &&
            end > (uintptr_t)PSX_ADDR(PSX_EXE_RODATA_START)) return 0;
        if (start < (uintptr_t)PSX_ADDR(0x800500f8u) &&
            end > (uintptr_t)PSX_ADDR(0x8004fe50u)) return 0;
    }
    return 1;
}

static int battle_model_packet_write(void *opaque, uint32_t address, unsigned width,
                                     uint32_t value)
{
    BattleModelPacketRun *run = opaque;
    if (!battle_model_packet_span(run->runtime, address, width, 1)) return -1;
    return runtime_write(run->runtime, address, width, value);
}

/* Only these emitted material records consume the optional lighting stream.
 * Flat polygons store color plus normal, textured flat polygons use a normal,
 * and smooth untextured triangles can record a color. Replaying cached flat
 * lighting skips the four-byte color before reading the normal. */
static unsigned battle_model_lighting_stride(unsigned material, int cached)
{
    switch (material) {
    case 0: case 8: return 12;
    case 1: case 9: return 8;
    case 2: return cached ? 0 : 4;
    default: return 0;
    }
}

static int battle_model_packet_inputs(BattleMipsRuntime *runtime, PcPortMipsCpu *cpu,
                                      const uint8_t *rows)
{
    uint32_t model = cpu->gpr[4], out = cpu->gpr[5];
    if ((model & 3u) || (out & 3u) ||
        !battle_model_packet_span(runtime, model, 0x34, 0))
        return 0;
    const uint8_t *header = module_memory_pointer(runtime, model, 0x34, 0);
    uint32_t groups = load_le(header + 0x10, 4), packets = load_le(header + 0x14, 4);
    unsigned group_count = load_le(header + 6, 2);
    uint32_t mode = cpu->gpr[6], flags = load_le(header, 2);
    uint32_t lighting_size = load_le(header + 0x30, 4);
    int allocate = mode && lighting_size && !(flags & 1u);
    /* Only modes 2 and 3 with stored lighting use this field as a pointer.
     * Other modes compute normals locally or use each vertex's normal;
     * the guest may copy the opaque field into its cursor without following it. */
    int use_lighting = (mode == 2 || mode == 3) && (allocate || (flags & 1u));
    int cached = mode == 2 && (flags & 2u);
    uint32_t lighting = load_le(header + 0x18, 4);
    uint64_t lighting_used = 0;
    if (allocate && (lighting_size > BATTLE_BUS_KUSEG_RAM_END ||
                     !battle_model_packet_span(runtime, model + 0x18, 4, 1))) return 0;
    if ((allocate || ((mode == 2 || mode == 3) && (flags & 1u))) &&
        !battle_model_packet_span(runtime, model, 2, 1)) return 0;
    if (group_count) {
        const uint32_t vertices = load_le(header + 8, 4);
        const uint32_t normals = load_le(header + 0xc, 4);
        if ((vertices && !battle_model_packet_span(runtime, vertices, 8, 0)) ||
            (normals && !battle_model_packet_span(runtime, normals, 8, 0))) return 0;
    }
    for (unsigned g = 0; g < group_count; ++g) {
        if ((groups & 1u) || !battle_model_packet_span(runtime, groups, 4, 0)) return 0;
        const uint8_t *group = module_memory_pointer(runtime, groups, 4, 0);
        unsigned type = group[0], count = load_le(group + 2, 2);
        if (type >= 17) return 0;
        const uint8_t *row = rows + type * 40;
        uint32_t stride = load_le(row + 28, 4);
        unsigned light_stride = use_lighting ? battle_model_lighting_stride(type, cached) : 0;
        if (count && light_stride) {
            uint32_t length = count * light_stride;
            if (allocate) {
                if (lighting_used + length > lighting_size) return 0;
            } else {
                unsigned skip = cached && light_stride == 12 ? 4 : 0;
                uint64_t start = (uint64_t)lighting + lighting_used + skip;
                if (!lighting || (start & 3u) || start > UINT32_MAX ||
                    !battle_model_packet_span(runtime, (uint32_t)start, length - skip, !cached))
                    return 0;
            }
            lighting_used += length;
        }
        if (count) {
            uint32_t packet_bytes = count * load_le(row + 32, 4);
            uint32_t out_bytes = count * load_le(row + 36, 4);
            if (!battle_model_packet_span(runtime, packets, packet_bytes, 0) ||
                !battle_model_packet_span(runtime, out, out_bytes, 1)) return 0;
            if (packets > UINT32_MAX - packet_bytes || out > UINT32_MAX - out_bytes) return 0;
            packets += packet_bytes;
            out += out_bytes;
        }
        uint32_t bytes = 4 + stride * count;
        if (!battle_model_packet_span(runtime, groups, bytes, 0) || groups > UINT32_MAX - bytes)
            return 0;
        groups += bytes;
    }
    return 1;
}

static int battle_resident_model_packets(BattleMipsRuntime *runtime, PcPortMipsCpu *cpu)
{
    uint32_t caller = cpu->gpr[31], stack = cpu->gpr[29];
    if (PcPortMipsInterpreterDisabled() || !caller || (caller & 3u) ||
        battle_model_packet_pc(caller) || (stack & 3u) || stack < 48 ||
        !battle_model_packet_span(runtime, stack - 48, 48, 1) ||
        (cpu->load_valid && cpu->load_reg != 0)) return -1;
    const XenoRetailFileSlot *slot = XenoRetailData_File(XENO_RD_SLUS);
    if (!slot || slot->state != 1 || !slot->bytes) return -1;
    const uint8_t *rows = slot->bytes + XENO_RD_SLUS_OFF(0x8004fe50u);
    const uint8_t *live_rows = module_memory_pointer(runtime, 0x8004fe50u, 17 * 40, 0);
    if (!live_rows || memcmp(rows, live_rows, 17 * 40) ||
        !battle_model_packet_inputs(runtime, cpu, rows)) return -1;

    BattleModelPacketRun run = {runtime, slot->bytes, 0, 0, 0};
    PcPortMipsBus outer_bus = cpu->bus;
    cpu->bus.opaque = &run;
    cpu->bus.bridge = battle_model_packet_bridge;
    cpu->bus.read = battle_model_packet_read;
    cpu->bus.write = battle_model_packet_write;
    /* runtime_cop2_* use the renderer's live GTE, independent of bus opaque. */
    int result = PcPortMipsRun(cpu, 0x8002c8ccu, cpu->gpr[31], CALLBACK_STEP_LIMIT);
    cpu->bus = outer_bus;
    if (result != PC_PORT_MIPS_HALTED)
        fprintf(stderr, "[xeno-port][battle-mips] resident model packet stopped at 0x%08x: %s\n",
                cpu->pc, cpu->error);
    return result == PC_PORT_MIPS_HALTED ? 1 : -1;
}
/* The packed draw table selects one of six variants per material. Keep its
 * jump table, callbacks and GTE code in the owner's verified resident image.
 * The six environment-map controls in row 16 are updated separately by the
 * renderer; this guest entry covers the immutable polygon walkers. */
static int battle_model_draw_pc(uint32_t pc)
{
    if (pc & 3u) return 0;
    return (pc >= 0x8002c700u && pc < 0x8002c8ccu) ||
           (pc >= 0x8002e010u && pc < 0x8002eef8u) ||
           (pc >= 0x8002ef0cu && pc < 0x8003014cu) ||
           (pc >= 0x80030ee8u && pc < 0x800315a0u);
}

static int battle_model_draw_bridge(void *opaque, PcPortMipsCpu *cpu, uint32_t target)
{
    BattleModelPacketRun *run = opaque;
    const uint32_t previous = run->previous_pc;
    if (!battle_model_draw_pc(target)) return -1;
    if (previous == 0x8002c808u) {
        const uint8_t *jump = run->image + XENO_RD_SLUS_OFF(0x80018980u);
        if (target != load_le(jump + 4 * run->draw_variant, 4)) return -1;
    }
    if (previous == 0x8002c868u) {
        const uint8_t *rows = run->image + XENO_RD_SLUS_OFF(0x8004fe50u);
        unsigned i = 0;
        while (i < 16 && target != load_le(rows + 40 * i + 4 * run->draw_variant, 4)) ++i;
        if (i == 16) return -1;
    }
    run->previous_pc = target;
    run->fetch_pc = target;
    return 0;
}

/* A packet link is a 24-bit address in the ordering table's domain. Some
 * guest callers retain the compact host alias of a packet in g_PsxRam.
 * Use its physical offset for guest tables, just as PcPort_AddPrimDomainAware
 * does. The original walker itself strips the segment bits from the cursor. */
static int battle_model_draw_packet_token(BattleMipsRuntime *runtime, uint32_t out,
                                          uint32_t ot, uint32_t bytes, uint32_t *token)
{
    if ((out & 3u) || (ot & 3u) ||
        !battle_model_packet_span(runtime, ot, 4, 1) ||
        !battle_model_packet_span(runtime, out, bytes ? bytes : 4, 1)) return 0;
    uintptr_t ram = (uintptr_t)g_PsxRam;
    uintptr_t packet = (uintptr_t)module_memory_pointer(runtime, out, bytes ? bytes : 4, 1);
    uintptr_t table = (uintptr_t)module_memory_pointer(runtime, ot, 4, 1);
    if (table >= ram && table < ram + PSX_RAM_SIZE) {
        if (packet < ram || packet - ram >= BATTLE_BUS_KUSEG_RAM_END ||
            bytes > BATTLE_BUS_KUSEG_RAM_END - (packet - ram)) return 0;
        *token = 0x80000000u | (uint32_t)(packet - ram);
    } else {
        /* A compact native DMA token must remain representable after the
         * original code drops its high byte and the bus decodes low KUSEG. */
        if (packet < BATTLE_BUS_KUSEG_RAM_END || packet >= 0x1000000u ||
            bytes > 0x1000000u - packet) return 0;
        *token = (uint32_t)packet;
    }
    return 1;
}

static int battle_resident_draw_model(BattleMipsRuntime *runtime, PcPortMipsCpu *cpu)
{
    uint32_t model = cpu->gpr[4], caller = cpu->gpr[31], stack = cpu->gpr[29];
    unsigned variant = cpu->gpr[7];
    if (PcPortMipsInterpreterDisabled() || !caller || (caller & 3u) ||
        battle_model_draw_pc(caller) || (cpu->load_valid && cpu->load_reg) ||
        (stack & 3u) || stack < 48 ||
        !battle_model_packet_span(runtime, stack - 48, 48, 1) || variant >= 6 ||
        (model & 3u) || !battle_model_packet_span(runtime, model, 0x2e, 0)) return -1;

    const XenoRetailFileSlot *slot = XenoRetailData_File(XENO_RD_SLUS);
    if (!slot || slot->state != 1 || !slot->bytes) return -1;
    const uint8_t *rows = slot->bytes + XENO_RD_SLUS_OFF(0x8004fe50u);
    const uint8_t *jumps = slot->bytes + XENO_RD_SLUS_OFF(0x80018980u);
    const uint8_t *live_rows = module_memory_pointer(runtime, 0x8004fe50u, 17 * 40, 0);
    const uint8_t *live_jumps = module_memory_pointer(runtime, 0x80018980u, 6 * 4, 0);
    if (!live_rows || memcmp(rows, live_rows, 17 * 40) ||
        !live_jumps || memcmp(jumps, live_jumps, 6 * 4)) return -1;

    const uint8_t *header = module_memory_pointer(runtime, model, 0x2e, 0);
    uint32_t group = load_le(header + 0x10, 4);
    uint32_t vertices = load_le(header + 8, 4);
    unsigned vertex_count = load_le(header + 2, 2), groups = load_le(header + 6, 2);
    uint32_t bytes = 0;
    for (unsigned i = 0; i < groups; ++i) {
        if ((group & 1u) || !battle_model_packet_span(runtime, group, 4, 0)) return -1;
        const uint8_t *entry = module_memory_pointer(runtime, group, 4, 0);
        unsigned type = entry[0], count = load_le(entry + 2, 2);
        if (type >= 16 || count > INT16_MAX) return -1;
        const uint8_t *row = rows + type * 40;
        uint32_t command_bytes = count * load_le(row + 28, 4);
        uint32_t packet_bytes = count * load_le(row + 36, 4);
        if (!battle_model_packet_span(runtime, group, 4 + command_bytes, 0) ||
            group > UINT32_MAX - 4 - command_bytes || bytes > UINT32_MAX - packet_bytes) return -1;
        if (count) {
            if (!vertex_count || !battle_model_packet_span(runtime, vertices, vertex_count * 8, 0))
                return -1;
            const uint8_t *indices = module_memory_pointer(runtime, group, 4 + command_bytes, 0);
            unsigned corners = type < 8 ? 3 : 4;
            for (unsigned face = 0; face < count; ++face)
                for (unsigned corner = 0; corner < corners; ++corner)
                    if (load_le(indices + 4 + face * 8 + corner * 2, 2) >= vertex_count) return -1;
        }
        bytes += packet_bytes;
        group += 4 + command_bytes;
    }
    uint32_t token;
    if (!battle_model_draw_packet_token(runtime, cpu->gpr[5], cpu->gpr[6], bytes, &token))
        return -1;
    BattleModelPacketRun run = {runtime, slot->bytes, 0, 0, variant};
    PcPortMipsBus outer_bus = cpu->bus;
    cpu->gpr[5] = token;
    cpu->bus.opaque = &run;
    cpu->bus.bridge = battle_model_draw_bridge;
    cpu->bus.read = battle_model_packet_read;
    cpu->bus.write = battle_model_packet_write;
    int result = PcPortMipsRun(cpu, 0x8002c700u, caller, CALLBACK_STEP_LIMIT);
    cpu->bus = outer_bus;
    if (result != PC_PORT_MIPS_HALTED)
        fprintf(stderr, "[xeno-port][battle-mips] resident draw stopped at 0x%08x: %s\n",
                cpu->pc, cpu->error);
    return result == PC_PORT_MIPS_HALTED ? 1 : -1;
}

#endif

static int battle_resident_boundary(BattleMipsRuntime *runtime, PcPortMipsCpu *cpu, uint32_t target)
{
    const uint32_t model = cpu->gpr[4];
    const uint32_t caller = cpu->gpr[31];

    /* Only the scoped guest dispatcher above can select a producer from the
     * packed table. Other callers cannot enter a native descriptor callback
     * with that guest return address. */
    if (caller == 0x8002ca9cu) return -1;

    /* Native field/world callers keep their host descriptors. Guest battle
     * callers retain packed rows and execute the user-supplied packet code. */
    if (target == 0x8002c8ccu) {
#ifdef XENO_NATIVE_ONLY
        return -1;
#else
        return battle_resident_model_packets(runtime, cpu);
#endif
    }

    if (target == 0x80043b48u &&
        (caller == 0x80073b24u || caller == 0x80073b50u ||
         caller == 0x80073bc8u || caller == 0x80073e50u)) {
        /* These four HUD callers could opt into a separate register-accurate
         * packet linker. The ordinary hybrid uses the existing typed linker.
         * A native-only run cannot claim the retired register contract. */
#ifdef XENO_NATIVE_ONLY
        return -1;
#else
        const char *hosts = getenv("XENO_BATTLE_NATIVE_HOSTS");
        const uint32_t ot = cpu->gpr[4], packet = cpu->gpr[5];
        if ((hosts && strcmp(hosts, "1") == 0) ||
            (cpu->load_valid && cpu->load_reg != 0) ||
            (ot & 3u) || (packet & 3u))
            return -1;
        const uint8_t *ot_read = module_memory_pointer(runtime, ot, 4, 0);
        const uint8_t *packet_read = module_memory_pointer(runtime, packet, 4, 0);
        if (!ot_read || !packet_read ||
            ot_read != module_memory_pointer(runtime, ot, 4, 1) ||
            packet_read != module_memory_pointer(runtime, packet, 4, 1))
            return -1;
        return BRIDGE_HOOKS_NONE;
#endif
    }

    if (target == 0x8003342cu)
        return battle_resident_relocate_archive(runtime, cpu);

    if (target == 0x800223b0u && cpu->gpr[31] == 0x801e68f8u) {
        /* This startup call hands the native direction reader a sprite and
         * its compact host animation pointer. The reader follows that word. */
        if (model < BATTLE_BUS_KUSEG_RAM_END || (model & 3u)) return -1;
        const uint8_t *sprite = module_memory_pointer(runtime, model, 0xb0, 0);
        if (!sprite) return -1;
        const uint32_t animation = load_le(sprite + 0x58, 4);
        if (battle_resident_guest_pointer(animation) ||
            !host_address_valid(runtime, animation, 1, 0))
            return -1;
        return BRIDGE_HOOKS_NONE;
    }

    if (target == 0x8002c700u) {
#ifdef XENO_NATIVE_ONLY
        return -1;
#else
        return battle_resident_draw_model(runtime, cpu);
#endif
    }
    return BRIDGE_HOOKS_NONE;
}
