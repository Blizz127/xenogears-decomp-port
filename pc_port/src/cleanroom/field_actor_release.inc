#include "system/memory.h"

extern s32 D_800ADBFC;
void func_800230A8(void *sprite);

void func_8008083C(int index)
{
    const volatile s32 *count = &D_800ADBFC;
    if (index >= *count)
        return;

    const volatile FieldActor *records = g_FieldActors;
    volatile ActorData *actor = (volatile ActorData *)(uintptr_t)records[index].pActorData;
    if ((u32)actor->flags134 & 0x80u)
        HeapFree((void *)(uintptr_t)actor->unk110);

    /* The flag group at 0x12C occupies one word in the actor record. */
    typedef u32 FieldActorReleaseFlags __attribute__((__may_alias__));
    const volatile FieldActorReleaseFlags *flags =
        (const volatile FieldActorReleaseFlags *)((const volatile u8 *)actor + 0x12C);
    if (*flags & 0x1000u)
        HeapFree((void *)(uintptr_t)actor->unk114);

    records = g_FieldActors;
    if ((u16)records[index].status & 0x2000u)
        HeapFree((void *)(uintptr_t)actor->unk118);
    if (actor->unk124 != -1)
        HeapFree((void *)(uintptr_t)actor->unk120);

    HeapFree((void *)actor);
    records = g_FieldActors;
    HeapFree((void *)(uintptr_t)records[index].pShadow);
    records = g_FieldActors;
    func_800230A8((void *)(uintptr_t)records[index].pSpriteData);
}
