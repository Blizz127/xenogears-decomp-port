#include <stdint.h>

void func_800A2714(void)
{
    if (!g_GamePartySkinsInitialized) {
        return;
    }

    for (int i = 0; i < D_800ADBFC; ++i) {
        ArchiveSetIndex(4, 0);
        ActorData *actor = (ActorData *)(uintptr_t)g_FieldActors[i].pActorData;
        int file = actor->unk124;
        if (file != -1) {
            g_FieldScriptVMCurActor = actor;
            uint32_t size = (uint32_t)ArchiveDecodeAlignedSize(file, actor);
            void *buffer = HeapAlloc(size + 8u, 0);
            actor = g_FieldScriptVMCurActor;
            actor->unk120 = (uint32_t)(uintptr_t)buffer;
            ArchiveReadFileToBuffer(actor->unk124, buffer, 0, 128);
            ArchiveCdDataSync(0);
        }
    }

    for (int i = 0; i < D_800ADBFC; ++i) {
        FieldActor *record = &g_FieldActors[i];
        ActorData *actor = (ActorData *)(uintptr_t)record->pActorData;
        if (actor->unk124 != -1) {
            SpriteSetSpecialAnimFile((void *)(uintptr_t)record->pSpriteData,
                                     (void *)(uintptr_t)actor->unk120);
        }
    }

    func_800A3C8C();
    if (g_FieldEffects.distortion.isActive) {
        FieldDistortionInitialize(1);
    }
    FieldScriptMemoryWriteU16(16, 0);
    FieldScriptWritePartyMemberIDs();
    for (int i = 0; i < D_800ADBFC; ++i) {
        func_80072254(i);
    }
}
