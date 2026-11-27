#include <string.h>

void func_8003B930(AudioManager *manager)
{
    SoundPsxAddress allocation = manager->unk_Manager_0x4;
    manager->unk_Manager_0x4 = 0;

    while (allocation) {
        const u8 *payload = SOUND_PSX_TO_PTR(u8, allocation);
        SoundPsxAddress successor;
        memcpy(&successor, payload + sizeof(SoundPsxAddress), sizeof(successor));
        SoundHeapFree(SOUND_PSX_TO_PTR(void, allocation));
        allocation = successor;
    }
}
