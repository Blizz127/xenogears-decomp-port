int func_800806E4(void)
{
    for (int slot = 0; slot < 4; slot++) {
        if (*(const volatile u16 *)&g_FieldTextBoxes[slot].order == 0)
            return slot;
    }
    return 0xffff;
}

int func_80080720(void)
{
    for (int slot = 0; slot < 4; slot++) {
        if (*(const volatile u16 *)&g_FieldTextBoxes[slot].order == 0xffff)
            return 0;
    }
    return -1;
}

int func_80080760(void)
{
    int selected = 0xffff;
    unsigned int highest = 0;
    for (int slot = 0; slot < 4; slot++) {
        unsigned int order = *(const volatile u16 *)&g_FieldTextBoxes[slot].order;
        if (order != 0xffff && order >= highest) {
            selected = slot;
            highest = order;
        }
    }
    return selected;
}
