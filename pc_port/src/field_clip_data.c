#include "field_clip_control.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

extern uint32_t func_801E6910(uint8_t* obj, int32_t index, uint32_t* flags);
extern void func_801DEF10(uint8_t* root, uint8_t* pose);
extern void func_801DFE8C(uint8_t* pool, uint8_t* root);
extern void func_801DF52C(uint8_t* pool, uint8_t* root, int32_t index, int32_t mask);
extern uint32_t func_801DF7F4(uint8_t* pool, uint8_t* root, uint8_t* pose,
                              int32_t loop, int32_t tag);
extern uint32_t func_801DF0B4(uint8_t* pool, uint8_t* root, uint8_t* pose,
                              int32_t duration, int32_t absolute,
                              int32_t loop, int32_t tag);
extern uint32_t D_801E85CC;
extern void func_801E5C74(uint8_t* object, uint8_t* data, int32_t loop);
extern void func_801E6974(uint8_t* object, uint8_t* pool, uint8_t* node,
                          int32_t flags, int32_t mode, int32_t tag, int32_t loop,
                          int32_t sx, int32_t sy, int32_t sz, int32_t ex,
                          int32_t ey, int32_t ez, int32_t duration);
extern void func_801E6D94(uint8_t* object, uint8_t* node, int32_t flags);
extern uint32_t func_801E6830(uint8_t* object, int32_t selector, uint16_t* mask);
extern uint32_t D_801E8670[10];
extern int32_t func_801E632C(uint8_t* object);
extern int32_t func_801DC848(uint8_t* root, int32_t scale);
extern int32_t func_801DC5C0(uint8_t* root, int32_t scale);

/* Object/data paths in retail E39F0, through continuation E5974.
 * Packed memory accesses preserve overlapping script/object access ordering.
 * The caller owns address translation; these are packed native addresses. */
static uint16_t half(const void* p)
{
    uint16_t value;
    memcpy(&value, p, sizeof(value));
    return value;
}
static uint32_t word(const void* p)
{
    uint32_t value;
    memcpy(&value, p, sizeof(value));
    return value;
}
static void put_half(void* p, uint16_t value)
{
    memcpy(p, &value, sizeof(value));
}
static void put_word(void* p, uint32_t value)
{
    memcpy(p, &value, sizeof(value));
}
static uint16_t take(PcPortFieldClipControl* state)
{
    uint16_t value = half((void*)(uintptr_t)state->stream);
    state->stream += 2u;
    return value;
}
static uint32_t relative(uint32_t start, uint16_t displacement)
{
    return start + (uint32_t)(int32_t)(int16_t)displacement;
}

int PcPort_FieldClipDataStep(PcPortFieldClipControl* state)
{
    uint32_t start = state->stream;
    uint16_t instruction = half((void*)(uintptr_t)start);
    unsigned opcode = instruction & 255;
    unsigned parameter = instruction >> 8;
    uint8_t* object = state->object;
    switch (opcode) {
    case 0x08: case 0x0a: case 0x0b: case 0x0c: case 0x0d: case 0x0e: case 0x10: case 0x11: case 0x18: case 0x19: case 0x1d: case 0x1e: case 0x1f: case 0x23: case 0x24: case 0x25: case 0x26: case 0x27: case 0x2e: case 0x30: case 0x31:
    case 0x13: case 0x32: case 0x33: case 0x34: case 0x36: case 0x37:
    case 0x3b: case 0x48: case 0x49: case 0x50: case 0x54:
    case 0x55: case 0x56: case 0x5e: case 0x5f: case 0x63:
    case 0x64: case 0x6d:
    case 0x4b: case 0x4c: case 0x4d: case 0x4e:
    case 0x5c: case 0x5d: case 0x6b:
        break;
    default:
        return 0; /* Not a successful instruction skip. */
    }
    state->operand = instruction;
    state->stream = start + 2u;
    switch (opcode) {
    case 0x08: {
        uint8_t* root = (uint8_t*)(uintptr_t)word(object + 4);
        func_801DFE8C(state->pool, root);
        (void)func_801E632C(object);
        break;
    }
    case 0x0a: {
        uint8_t* root = (uint8_t*)(uintptr_t)word(object + 4);
        func_801DF52C(state->pool, root, (int32_t)parameter, 7);
        break;
    }
    case 0x0b: {
        uint8_t* root = (uint8_t*)(uintptr_t)word(object + 4);
        uint32_t count = half(root + 0x0a);
        func_801DFE8C(state->pool, root);
        for (uint32_t i = 1; i < count; ++i) {
            uint8_t* node = root + i * 124u;
            put_half(node + 0x4f, 0);
            put_half(node + 0x51, 0);
            put_half(node + 0x53, 0);
            put_word(node + 0x57, 0);
            put_word(node + 0x5b, 0);
            put_word(node + 0x5f, 0);
            node[0x7c] = 1;
            node[0x7d] = 1;
        }
        break;
    }
    case 0x0d: case 0x0e: {
        uint8_t* root = (uint8_t*)(uintptr_t)word(object + 4);
        func_801DF52C(state->pool, root, (int32_t)parameter,
                      opcode == 0x0d ? 1 : 2);
        break;
    }
    case 0x10: {
        uint32_t flags;
        uint8_t* root = (uint8_t*)(uintptr_t)word(object + 4);
        uint8_t* pose = (uint8_t*)(uintptr_t)func_801E6910(object, parameter, &flags);
        func_801DEF10(root, pose);
        break;
    }
    case 0x11: {
        uint32_t flags;
        uint16_t next = take(state);
        uint8_t* root = (uint8_t*)(uintptr_t)word(object + 4);
        uint8_t* pose = (uint8_t*)(uintptr_t)func_801E6910(object, parameter, &flags);
        if (flags != 0) break;
        (void)func_801DF7F4(state->pool, root, pose, next >> 8, 0x11);
        {
            int32_t scale = (int16_t)half(object + 0x1c);
            int32_t height = (int16_t)half(root + 0x50);
            int32_t value = (int16_t)half(pose + 0x10);
            int32_t product = (scale * height) >> 12;
            int32_t result = (product * value) >> 12;
            if (result < 0) result = -result;
            put_half(object + 0x8e, (uint16_t)result);
        }
        func_801E5C74(object, pose, next >> 8);
        break;
    }
    case 0x13: {
        /* Retail E3FD4..E4068 and shared E44D4 tail. Unlike 11, this
         * instruction uses the pose even when lookup returns flags. */
        uint16_t control = take(state);
        state->operand = control;
        uint16_t blend = take(state);
        state->operand = blend;
        uint32_t flags;
        uint8_t* pose = (uint8_t*)(uintptr_t)func_801E6910(
            object, control & 0xff, &flags);
        uint8_t* root = (uint8_t*)(uintptr_t)word(object + 4);
        if (D_801E85CC != 0)
            func_801DEF10(root, pose);
        else
            (void)func_801DF0B4(state->pool, root, pose, blend >> 8,
                                parameter, blend & 0xff, control >> 8);
        state->limit = -1;
        (void)func_801E632C(object);
        break;
    }
    case 0x18: {
        uint32_t flags;
        uint16_t next = take(state);
        uint8_t* pose = (uint8_t*)(uintptr_t)func_801E6910(object, parameter, &flags);
        func_801E5C74(object, pose, (int16_t)next);
        break;
    }
    case 0x19:
        (void)func_801E632C(object);
        break;
    case 0x1f: {
        /* The selector is relative to the E39F0 entry object (stack +E0),
         * even after an earlier 1F changed the active object in s4. */
        uint32_t slot = func_801E6830(state->origin, parameter,
                                     &state->operand) & 0xffu;
        if (slot >= 10) {
            fprintf(stderr, "[obj-ovly] retail clip opcode 1F slot outside native registry: %u ip=%08x\n",
                    slot, start);
            abort();
        }
        if (D_801E8670[slot] != 0)
            state->object = (uint8_t*)(uintptr_t)D_801E8670[slot];
        break;
    }
    case 0x23: {
        int32_t index = (int16_t)take(state);
        uint8_t* root = (uint8_t*)(uintptr_t)word(object + 4);
        func_801E6D94(object, root + index * 124, (int32_t)parameter);
        break;
    }
    case 0x25: {
        /* Retail E4810: resolve the selector before reading orientation;
         * the resolver may change memory shared with the remaining script. */
        uint16_t selector = take(state);
        state->operand = selector;
        (void)func_801E6830(object, selector & 0xff, &state->operand);
        uint16_t value_x = take(state);
        uint16_t value_y = take(state);
        uint16_t value_z = take(state);
        for (unsigned slot = 0; slot < 8; ++slot) {
            if (((state->operand >> slot) & 1u) == 0) continue;
            uint8_t* target = (uint8_t*)(uintptr_t)D_801E8670[slot];
            if (!target) continue;
            put_half(target + 0x5e, selector >> 8);
            target[0x5c] = object[0x20];
            target[0x5d] = (uint8_t)(parameter & 2u);
            target[0x36] = 1;
            if ((parameter & 1u) == 0) {
                put_half(target + 0x6a, value_x);
                put_half(target + 0x6c, value_y);
                put_half(target + 0x6e, value_z);
            } else {
                /* The alternate branch is the GTE/vector path below this
                 * handler; do not silently substitute host math for it. */
                fprintf(stderr, "[obj-ovly] retail clip opcode 25 GTE branch is not yet ported\n");
                abort();
            }
        }
        break;
    }
    case 0x26: {
        (void)func_801E6830(object, parameter, &state->operand);
        for (unsigned slot = 0; slot < 8; ++slot) {
            if (((state->operand >> slot) & 1u) == 0) continue;
            uint8_t* target = (uint8_t*)(uintptr_t)D_801E8670[slot];
            if (target) target[0x5c] = 0xff;
        }
        break;
    }
    case 0x27: {
        uint8_t* root = (uint8_t*)(uintptr_t)word(object + 4);
        int32_t scale = (int16_t)half(object + 0x1c);
        if (object[0x37] != 0)
            (void)func_801DC848(root, scale);
        else
            (void)func_801DC5C0(root, scale);
        break;
    }
    case 0x1d: {
        uint16_t w0 = take(state);
        state->limit = -1;
        state->operand = w0;
        uint16_t w1 = take(state);
        state->operand = w1;
        int32_t values[7];
        uint8_t* root = (uint8_t*)(uintptr_t)word(object + 4);
        uint8_t* node = root + ((uint32_t)parameter * 124u);
        for (unsigned i = 0; i < 7; ++i) values[i] = (int16_t)take(state);
        func_801E6974(object, state->pool, node, w0 & 0xff, w0 >> 8,
                      w1 & 0xff, w1 >> 8, values[0], values[1], values[2],
                      values[3], values[4], values[5], values[6]);
        break;
    }
    case 0x0c:
        for (unsigned offset = 0x70; offset <= 0x86; offset += 2)
            put_half(object + offset, 0);
        break;
    case 0x1e:
        object[0x37] = (uint8_t)parameter;
        break;
    case 0x24:
        if (object) object[0x34] = parameter & 1;
        break;
    case 0x2e:
        put_half(object + 0x48, half(object + 0x8e));
        state->operand = take(state);
        put_word(object + 0x4c, parameter ? relative(start, state->operand) : 0);
        break;
    case 0x30:
        put_half((void*)(uintptr_t)state->stream, 0);
        state->stream += 2u;
        break;
    case 0x32:
        state->operand = take(state);
        state->stream = relative(start, state->operand);
        break;
    case 0x31: {
        uint16_t displacement = take(state);
        uint32_t loop = relative(start, displacement);
        uint16_t header = half((void*)(uintptr_t)loop);
        uint16_t count = (uint16_t)(half((void*)(uintptr_t)(loop + 2u)) + 1u);
        put_half((void*)(uintptr_t)(loop + 2u), count);
        state->operand = count;
        if ((int16_t)count < (int32_t)(header >> 8))
            state->stream = loop + 4u;
        break;
    }
    case 0x33: case 0x34: case 0x3b:
        state->operand = take(state);
        break;
    case 0x36:
        put_half(object + 0x44, 0);
        put_half(object + 0x46, take(state));
        state->operand = take(state);
        put_word(object + 0x50, parameter ? relative(start, state->operand) : 0);
        break;
    case 0x37:
        state->operand = take(state);
        put_word(object + 0x54, parameter ? relative(start, state->operand) : 0);
        break;
    case 0x48:
        object[0x36] = (uint8_t)parameter;
        break;
    case 0x49:
        for (unsigned axis = 0; axis < 3; ++axis) {
            uint8_t* root = (void*)(uintptr_t)word(object + 4);
            int32_t value = (int16_t)take(state);
            put_word(root + 0x5c + axis * 4, (uint32_t)value);
        }
        break;
    case 0x50:
        put_half(object + 0x58, 0);
        for (unsigned axis = 0; axis < 3; ++axis)
            put_half(object + 0x88 + axis * 2, take(state));
        break;
    case 0x4b: case 0x4c: case 0x4d: case 0x4e: {
        unsigned offset = opcode < 0x4d ? 0x7c : 0x82;
        int add = opcode == 0x4c || opcode == 0x4e;
        for (unsigned axis = 0; axis < 3; ++axis) {
            uint16_t previous = add ? half(object + offset + axis * 2) : 0;
            uint16_t value = take(state);
            put_half(object + offset + axis * 2, (uint16_t)(previous + value));
        }
        break;
    }
    case 0x54: case 0x55: {
        uint8_t* root = (void*)(uintptr_t)word(object + 4);
        int32_t scale = (int16_t)half(object + 0x1c);
        int32_t height = (int16_t)half(root + 0x50);
        int32_t product = (scale * height) >> 12;
        int32_t value = (int16_t)take(state);
        /* MULT/MFLO wraps before the second arithmetic right shift. */
        uint32_t result = (uint32_t)((int32_t)((uint32_t)product *
                                             (uint32_t)value) >> 12);
        if (opcode == 0x55) result += half(object + 0x8e);
        put_half(object + 0x8e, (uint16_t)result);
        break;
    }
    case 0x56: {
        uint16_t previous = half(object + 0x8e);
        put_half(object + 0x8e, (uint16_t)(previous + take(state)));
        break;
    }
    case 0x5e: case 0x5f:
        state->operand = take(state);
        put_half(object + (opcode == 0x5e ? 0x1c : 0x4a), state->operand);
        break;
    case 0x5c: {
        state->operand = take(state);
        uint8_t* root = (void*)(uintptr_t)word(object + 4);
        unsigned axis;
        for (axis = 0; axis < 3; ++axis) {
            uint32_t target = (uint32_t)(int32_t)(int16_t)half(object + 0x88 + axis * 2);
            if (target != word(root + 0x5c + axis * 4)) break;
        }
        if (axis == 3) state->stream = relative(start, state->operand);
        break;
    }
    case 0x5d: case 0x6b: {
        state->operand = take(state);
        uint32_t offset = (uint32_t)(int32_t)(int16_t)state->operand * 124u;
        uint8_t* node = (void*)(uintptr_t)(word(object + 4) + offset);
        if (opcode == 0x5d) put_half(node + 0x52, (uint16_t)parameter);
        else node[6] = (uint8_t)parameter;
        break;
    }
    case 0x63: {
        uint16_t displacement = take(state);
        put_half(object + 0x98, 0);
        put_half(object + 0x9a, 0xffff);
        put_half(object + 0x9c, 0);
        put_half(object + 0x9e, (uint16_t)parameter);
        put_word(object + 0xa0, state->stream);
        state->stream = relative(start, displacement);
        state->operand = displacement;
        break;
    }
    case 0x64:
        put_half(object + 0x3e, take(state));
        break;
    case 0x6d:
        object[0x38] = parameter & 1;
        break;
    }
    return 1;
}
