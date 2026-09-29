#include <assert.h>
#include <stdint.h>
#include <stdio.h>

typedef uint8_t u8;
typedef uint32_t u32;

static unsigned trace, present, blocked, alternate, matrix, status;
static u32 actor_expected, target_expected;

static unsigned read_target(unsigned kind, u32 target, unsigned value) {
    assert(target == target_expected);
    trace = trace * 10 + kind;
    return value;
}

static unsigned read_matrix(u32 actor, u32 target) {
    assert(actor == actor_expected);
    return read_target(4, target, matrix);
}

#define XBT_ELIGIBILITY_CUSTOM_BINDINGS
#define XBT_PRESENT(target) read_target(1, target, present)
#define XBT_BLOCKED(target) read_target(2, target, blocked)
#define XBT_ALTERNATE(target) read_target(3, target, alternate)
#define XBT_MATRIX(actor, target) read_matrix(actor, target)
#define XBT_NORMAL_STATUS(target) read_target(5, target, status)
#define XBT_ALTERNATE_STATUS(target) read_target(6, target, status)
#include "../../src/battle/target_eligibility_impl.inc"

int main(void) {
    unsigned cases = 0;
    for (unsigned mode = 0; mode < 5; mode++) {
        for (status = 0; status < 65536; status++) {
            u32 actor = 0xABCDEF00u | (status & 255);
            u32 target = 0x12345600u | ((status >> 8) & 255);
            actor_expected = actor & 255;
            target_expected = target & 255;
            present = mode != 0;
            blocked = mode == 1;
            alternate = mode == 4;
            matrix = mode == 2;
            trace = 0;
            unsigned result = func_80083FF4(actor, target);
            unsigned expected = mode >= 3 && (status & 0xC001) == 0;
            const unsigned traces[] = {1, 12, 1234, 12345, 1236};
            assert(result == expected);
            assert(trace == traces[mode]);
            cases++;
        }
    }
    printf("ELIGIBILITY BINDINGS PASS %u cases\n", cases);
    return 0;
}
