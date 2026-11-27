void GameCheckAndHandleSoftReset(void)
{
    const u_short chord = CTRL_BTN_SELECT | CTRL_BTN_START | CTRL_BTN_L1 | CTRL_BTN_R1;
    if (*(const volatile u_short *)&g_C1ButtonState == chord) {
        GameSoftReset();
    }
}

void HeapResetUser(void)
{
    HeapChangeCurrentUser(HEAP_USER_UNKNOWN, NULL);
}

void func_80019C2C(void)
{
    PCinit();
    int handle = PCcreate(&D_800180FC, 0);
    PCwrite(handle, (char *)PSX_ADDR(0x80000000u), 0x200000);
    PCclose(handle);
}
