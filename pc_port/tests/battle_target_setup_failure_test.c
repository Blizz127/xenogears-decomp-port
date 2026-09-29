#include "battle_target_setup.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

struct Operation { char kind; uint32_t address, value; unsigned width; };
struct Tape {
    const struct Operation *ops;
    unsigned count, cursor, fail, stores, callbacks;
    uint32_t checksum;
};
static int operation(struct Tape *t, char kind, uint32_t address,
                     unsigned width, uint32_t *value) {
    assert(t->cursor < t->count && t->cursor <= t->fail);
    const struct Operation *op = &t->ops[t->cursor++];
    assert(op->kind == kind && op->address == address && op->width == width);
    if (kind != 'r') assert(*value == op->value);
    /* A failing callback may already have effects; the caller cannot undo it. */
    if (kind == 'c') t->callbacks++;
    if (t->cursor - 1 == t->fail) return -1;
    if (kind == 'w') {
        t->stores++;
        t->checksum = t->checksum * 33u + address + *value;
    }
    if (kind == 'r') *value = op->value;
    return 0;
}
static int read_memory(void *opaque, uint32_t a, unsigned w, uint32_t *v) {
    return operation(opaque, 'r', a, w, v);
}
static int write_memory(void *opaque, uint32_t a, unsigned w, uint32_t v) {
    return operation(opaque, 'w', a, w, &v);
}
static int invoke(void *opaque, uint32_t code, uint32_t task) {
    return operation(opaque, 'c', code, 0, &task);
}
static int invoke_frame(void *opaque, PcPortMipsCpu *cpu, uint32_t code) {
    assert(cpu->gpr[29]==0x801FEFE8u && cpu->gpr[16]==0x800C0000u);
    assert(cpu->gpr[31]==(code==0x80010000u?0x800BC3B8u:0x800BC3E0u));
    int result=invoke(opaque,code,cpu->gpr[4]);
    if (result) {
        cpu->gpr[4]=0xBAD00004; cpu->gpr[16]=0xBAD00010;
        cpu->gpr[29]=0xBAD0001D; cpu->gpr[31]=0xBAD0001F;
    }
    return result;
}
static void run_variant(uint32_t mode, const struct Operation *ops, unsigned count, unsigned frame) {
    for (unsigned fail = 0; fail <= count; fail++) {
        struct Tape tape = {.ops=ops, .count=count, .fail=fail};
        PcPortMipsBus memory = {.opaque=&tape, .read=read_memory, .write=write_memory};
        PcPortMipsCpu cpu={.bus=memory};
        cpu.gpr[4]=mode; cpu.gpr[29]=0x801FF000;
        cpu.gpr[31]=0xFFFFFFFC; cpu.gpr[16]=0xCAFEBABE;
        int result=frame?PcPortBattleTargetSetupFrame(&cpu,invoke_frame,&tape):
            PcPortBattleTargetSetup(&memory,mode,invoke,&tape);
        assert(result ==
               (fail == count ? 0 : -1));
        if (frame && fail<count && ops[fail].kind=='c') {
            assert(cpu.gpr[4]==0xBAD00004u && cpu.gpr[16]==0xBAD00010u);
            assert(cpu.gpr[29]==0xBAD0001Du && cpu.gpr[31]==0xBAD0001Fu);
        } else if (frame && mode!=2 && mode!=4) {
            uint32_t expected_a0=mode;
            for (unsigned i=0;i<tape.cursor && i!=fail;i++)
                if (ops[i].kind=='r' && (ops[i].address==0x800C3680u || ops[i].address==0x800C3684u))
                    expected_a0=ops[i].value;
            assert(cpu.gpr[4]==expected_a0);
        }
        if (frame && fail==count)
            assert(cpu.gpr[29]==0x801FF000u && cpu.gpr[31]==0xFFFFFFFCu && cpu.gpr[16]==0xCAFEBABEu);
        assert(tape.cursor == (fail == count ? count : fail + 1));
        unsigned stores=0, callbacks=0;
        uint32_t checksum=0;
        for (unsigned i=0; i<tape.cursor; i++) {
            if (ops[i].kind == 'c') callbacks++;
            if (ops[i].kind == 'w' && i != fail) {
                stores++;
                checksum=checksum*33u+ops[i].address+ops[i].value;
            }
        }
        assert(tape.stores==stores && tape.callbacks==callbacks && tape.checksum==checksum);
    }
}
static void run(uint32_t mode, const struct Operation *ops, unsigned count) {
    run_variant(mode,ops,count,0);
    struct Operation framed[32];
    assert(count+4<=32);
    memcpy(framed,ops,2*sizeof(*ops));
    framed[2]=(struct Operation){'w',0x801FEFFC,0xFFFFFFFC,4};
    framed[3]=(struct Operation){'w',0x801FEFF8,0xCAFEBABE,4};
    memcpy(framed+4,ops+2,(count-2)*sizeof(*ops));
    framed[count+2]=(struct Operation){'r',0x801FEFFC,0xFFFFFFFC,4};
    framed[count+3]=(struct Operation){'r',0x801FEFF8,0xCAFEBABE,4};
    run_variant(mode,framed,count+4,1);
}
int main(void) {
    const struct Operation tasks[] = {
        {'w',0x800C3CC0,1,4}, {'w',0x800C3CBC,1,4},
        {'r',0x800C3680,0x41001000,4}, {'r',0x4100100C,0x80010000,4},
        {'c',0x80010000,0x41001000,0}, {'w',0x800C3680,0,4},
        {'r',0x800C3684,0xA0100020,4}, {'r',0xA010002C,0x80010004,4},
        {'c',0x80010004,0xA0100020,0}, {'w',0x800C3684,0,4}
    };
    const struct Operation vectors[] = {
        {'w',0x800C3CC0,2,4}, {'w',0x800C3CBC,1,4},
        {'r',0x800D30A0,0x8000,2}, {'w',0x8006F99C,0x80000000,4},
        {'r',0x800D30A2,0xFFFF,2}, {'r',0x800D30A4,1,2},
        {'w',0x8006F9A0,0xFFFF0000,4}, {'w',0x8006F9A4,0x10000,4},
        {'r',0x800D30A8,0x7FFF,2}, {'w',0x8006F9AC,0x7FFF0000,4},
        {'r',0x800D30AA,2,2}, {'r',0x800D30AC,0,2},
        {'w',0x8006F9B0,0x20000,4}, {'w',0x8006F9B4,0,4}
    };
    const struct Operation mode4[] = {
        {'w',0x800C3CC0,4,4}, {'w',0x800C3CBC,1,4}, {'w',0x800C3CBC,5,4}
    };
    const struct Operation empty[] = {
        {'w',0x800C3CC0,0,4}, {'w',0x800C3CBC,1,4},
        {'r',0x800C3680,0,4}, {'r',0x800C3684,0,4}
    };
    run(1,tasks,sizeof(tasks)/sizeof(tasks[0]));
    run(2,vectors,sizeof(vectors)/sizeof(vectors[0]));
    run(4,mode4,sizeof(mode4)/sizeof(mode4[0]));
    run(0,empty,sizeof(empty)/sizeof(empty[0]));
    PcPortMipsBus invalid = {0};
    assert(PcPortBattleTargetSetup(NULL,1,invoke,NULL)==-1);
    assert(PcPortBattleTargetSetup(&invalid,1,invoke,NULL)==-1);
    invalid.read=read_memory; invalid.write=write_memory;
    assert(PcPortBattleTargetSetup(&invalid,1,NULL,NULL)==-1);
    assert(PcPortBattleTargetSetupFrame(NULL,invoke_frame,NULL)==-1);
    PcPortMipsCpu cpu={0};
    assert(PcPortBattleTargetSetupFrame(&cpu,invoke_frame,NULL)==-1);
    cpu.gpr[29]=0x801FF001;
    assert(PcPortBattleTargetSetupFrame(&cpu,invoke_frame,NULL)==-1);
    puts("TARGET SETUP native failure prefixes PASS 86 tapes and 6 invalid APIs");
}
