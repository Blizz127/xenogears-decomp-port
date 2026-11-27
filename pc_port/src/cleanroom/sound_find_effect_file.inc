/* SPDX-License-Identifier: MIT */
void *func_8003864C(void *reference, s16 id)
{
    if (reference)
        id = (s16)((const SoundFile *)reference)->sedId;

    for (SoundFile *file = g_SoundSedsLinkedList; file;
         file = SOUND_PSX_TO_PTR(SoundFile, file->pNext)) {
        if ((int)file->sedId == (int)id)
            return file;
    }
    return NULL;
}
