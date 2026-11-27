#include <stdint.h>

typedef uint16_t FieldPlacementHalf __attribute__((__aligned__(1), __may_alias__));
typedef uint32_t FieldPlacementWord __attribute__((__aligned__(1), __may_alias__));

static uintptr_t fieldPlacementRow(FieldActor *actors, int index)
{
    uint32_t bytes = (uint32_t)index * (uint32_t)sizeof(FieldActor);
    return (uintptr_t)actors + (uintptr_t)(intptr_t)(int32_t)bytes;
}

static uint32_t fieldPlacementBinding(FieldActor *actors, int index, unsigned member)
{
    return *(const volatile FieldPlacementWord *)(fieldPlacementRow(actors, index) + member);
}

static void fieldPlacementCopyHalf(uint32_t destination, uint32_t source, unsigned offset)
{
    uint16_t value = *(const volatile FieldPlacementHalf *)(uintptr_t)(source + offset);
    *(volatile FieldPlacementHalf *)(uintptr_t)(destination + offset) = value;
}

static void fieldPlacementCopyWord(uint32_t destination, uint32_t source, unsigned offset)
{
    uint32_t value = *(const volatile FieldPlacementWord *)(uintptr_t)(source + offset);
    *(volatile FieldPlacementWord *)(uintptr_t)(destination + offset) = value;
}

static inline void CopySpriteFieldU16(FieldActor *actors, int dst, int src, int offset)
{
    unsigned member = __builtin_offsetof(FieldActor, pSpriteData);
    uint32_t source = fieldPlacementBinding(actors, src, member);
    uint32_t destination = fieldPlacementBinding(actors, dst, member);
    fieldPlacementCopyHalf(destination, source, (uint32_t)offset);
}

static inline void CopySpriteFieldS32(FieldActor *actors, int dst, int src, int offset)
{
    unsigned member = __builtin_offsetof(FieldActor, pSpriteData);
    uint32_t source = fieldPlacementBinding(actors, src, member);
    uint32_t destination = fieldPlacementBinding(actors, dst, member);
    fieldPlacementCopyWord(destination, source, (uint32_t)offset);
}

void FieldActorCopyPlacement(int dst, int src)
{
    FieldActor *actors = g_FieldActors;
    unsigned binding = __builtin_offsetof(FieldActor, pActorData);
    uint32_t source = fieldPlacementBinding(actors, src, binding);
    uint32_t destination = fieldPlacementBinding(actors, dst, binding);
    for (unsigned part = 0; part < 4; ++part)
        fieldPlacementCopyHalf(destination, source, __builtin_offsetof(ActorData, walkmeshTriIds) + 2u * part);
    fieldPlacementCopyHalf(destination, source, __builtin_offsetof(ActorData, walkmeshId));

    for (unsigned part = 0; part < 3; ++part)
        fieldPlacementCopyWord(destination, source, __builtin_offsetof(ActorData, curTriNormal) + 4u * part);
    for (unsigned part = 0; part < 3; ++part)
        fieldPlacementCopyWord(destination, source, __builtin_offsetof(ActorData, position) + 4u * part);

    fieldPlacementCopyHalf(destination, source, __builtin_offsetof(ActorData, unkEC));
    fieldPlacementCopyHalf(destination, source, __builtin_offsetof(ActorData, curYPos));
    fieldPlacementCopyWord(destination, source, __builtin_offsetof(ActorData, curWalkmeshTriMaterial));

    CopySpriteFieldU16(g_FieldActors, dst, src, 0x84);
    for (unsigned component = 0; component < 3; ++component)
        CopySpriteFieldS32(g_FieldActors, dst, src, (int)(4u * component));

    for (unsigned axis = 0; axis < 3; ++axis) {
        actors = g_FieldActors;
        unsigned offset = __builtin_offsetof(FieldActor, transformMatrix.t) + 4u * axis;
        uint32_t value = *(const volatile FieldPlacementWord *)(fieldPlacementRow(actors, src) + offset);
        *(volatile FieldPlacementWord *)(fieldPlacementRow(actors, dst) + offset) = value;
    }
}
