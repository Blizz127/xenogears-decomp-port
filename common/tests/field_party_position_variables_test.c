#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>

typedef int32_t s32;

static s32 variables[60];
static s32 calls[3];
static unsigned callCount;

static s32 FieldScriptVMGetVariableValue(s32 id)
{
    assert(callCount < 3);
    assert(id >= 0 && id < 60);
    calls[callCount++] = id;
    return variables[id];
}

#include "field_party_position_variables.inc"

int main(void)
{
    static const s32 ids[][3] = {
        {42, 44, 46}, {48, 50, 52}, {54, 56, 58}
    };
    static const s32 invalid[] = {INT32_MIN, -1, 3, INT32_MAX};
    s32 result[3] = {0};
    s32 last = 0;

    for (unsigned slot = 0; slot < 3; ++slot) {
        variables[ids[slot][0]] = INT32_MIN + (s32)slot;
        variables[ids[slot][1]] = INT32_MAX - (s32)slot;
        variables[ids[slot][2]] = -70000 - (s32)slot;
        callCount = 0;
        func_800A0158(slot, &result[0], &result[1], &result[2]);
        assert(callCount == 3);
        for (unsigned i = 0; i < 3; ++i) {
            assert(calls[i] == ids[slot][i]);
            assert(result[i] == variables[ids[slot][i]]);
        }
    }

    callCount = 0;
    func_800A0158(1, &last, &last, &last);
    assert(callCount == 3 && last == variables[52]);

    callCount = 0;
    func_800A0158(0, &variables[44], &variables[46], &last);
    assert(callCount == 3);
    assert(variables[44] == INT32_MIN && variables[46] == INT32_MIN);
    assert(last == INT32_MIN);

    callCount = 0;
    for (unsigned i = 0; i < sizeof invalid / sizeof invalid[0]; ++i) {
        func_800A0158(invalid[i], NULL, NULL, NULL);
        func_800A0158(invalid[i], &last, &last, &last);
        assert(callCount == 0 && last == INT32_MIN);
    }

    puts("PASS: selectors, signed outputs, call order, aliases and invalid inputs");
    return 0;
}
