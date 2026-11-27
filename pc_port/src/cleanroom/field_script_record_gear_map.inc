extern int g_GameSceneMapNum;
extern void FieldGearMapWrite(int address, int value) __asm__("FieldScriptMemoryWriteU16");

void func_8009FD10(int partyMemberIndex)
{
    if ((unsigned int)partyMemberIndex >= 3u) return;

    int address = 0x2a + 6 * partyMemberIndex;
    const volatile int* currentMap = &g_GameSceneMapNum;
    int field = *currentMap & 0xfff;
    FieldGearMapWrite(address, field);
    FieldGearMapWrite(address + 2, 0);
    FieldGearMapWrite(address + 4, 0);
}
