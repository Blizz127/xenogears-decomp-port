#include "battle_target_setup.h"

static int setup(const PcPortMipsBus *memory, uint32_t mode,
                          int (*invoke)(void *, uint32_t, uint32_t),
                          void *callback_context, PcPortMipsCpu *cpu)
{
    uint32_t task, callback, value, last;
    if (!memory || !memory->read || !memory->write || !invoke)
        return -1;
    if (cpu) cpu->gpr[29] -= 0x18u;
    if (memory->write(memory->opaque, 0x800C3CC0u, 4, mode) ||
        memory->write(memory->opaque, 0x800C3CBCu, 4, 1))
        return -1;
    if (cpu) {
        if (memory->write(memory->opaque, cpu->gpr[29]+0x14u, 4, cpu->gpr[31]) ||
            memory->write(memory->opaque, cpu->gpr[29]+0x10u, 4, cpu->gpr[16]))
            return -1;
        if (mode != 2) cpu->gpr[16] = 0x800C0000u;
    }
    if (mode == 4) {
        if (memory->write(memory->opaque, 0x800C3CBCu, 4, 5)) return -1;
        goto done;
    }
    if (mode == 2) {
        for (unsigned vector = 0; vector < 2; vector++) {
            uint32_t source = 0x800D30A0u + vector * 8;
            uint32_t dest = 0x8006F99Cu + vector * 16;
            if (memory->read(memory->opaque, source, 2, &value) ||
                memory->write(memory->opaque, dest, 4, (value & 0xFFFFu) << 16) ||
                memory->read(memory->opaque, source + 2, 2, &value) ||
                memory->read(memory->opaque, source + 4, 2, &last) ||
                memory->write(memory->opaque, dest + 4, 4, (value & 0xFFFFu) << 16) ||
                memory->write(memory->opaque, dest + 8, 4, (last & 0xFFFFu) << 16))
                return -1;
        }
        goto done;
    }
    for (unsigned root = 0; root < 2; root++) {
        uint32_t address = 0x800C3680u + root * 4;
        if (cpu) cpu->gpr[16] = 0x800C0000u;
        if (memory->read(memory->opaque, address, 4, &task))
            return -1;
        if (cpu) cpu->gpr[4] = task;
        if (task) {
            if (task > UINT32_MAX - 12 ||
                memory->read(memory->opaque, task + 12, 4, &callback))
                return -1;
            if (cpu) {
                cpu->gpr[31] = root ? 0x800BC3E0u : 0x800BC3B8u;
            }
            if (invoke(callback_context, callback, task)) return -1;
            /* A callback can restore s0 from guest bytes changed by aliases. */
            if (cpu) address = cpu->gpr[16] + 0x3680u + root * 4;
            if (memory->write(memory->opaque, address, 4, 0)) return -1;
        }
    }
done:
    if (cpu) {
        if (memory->read(memory->opaque, cpu->gpr[29]+0x14u, 4, &value)) return -1;
        cpu->gpr[31] = value;
        if (memory->read(memory->opaque, cpu->gpr[29]+0x10u, 4, &value)) return -1;
        cpu->gpr[16] = value;
        cpu->gpr[29] += 0x18u;
    }
    return 0;
}

int PcPortBattleTargetSetup(const PcPortMipsBus *memory, uint32_t mode,
                          int (*invoke)(void *, uint32_t, uint32_t),
                          void *callback_context) {
    return setup(memory,mode,invoke,callback_context,NULL);
}

struct FrameCall {
    PcPortMipsCpu *cpu;
    int (*invoke)(void *, PcPortMipsCpu *, uint32_t);
    void *context;
};
static int frame_invoke(void *opaque,uint32_t callback,uint32_t task) {
    struct FrameCall *call=opaque;
    (void)task;
    return call->invoke(call->context,call->cpu,callback);
}
int PcPortBattleTargetSetupFrame(PcPortMipsCpu *cpu,
        int (*invoke)(void *, PcPortMipsCpu *, uint32_t), void *context) {
    if (!cpu || !invoke || cpu->gpr[29]<0x18u || (cpu->gpr[29]&7u)) return -1;
    struct FrameCall call={cpu,invoke,context};
    return setup(&cpu->bus,cpu->gpr[4],frame_invoke,&call,cpu);
}
