void func_8007E114(int index, int first, int second, int third, s32 fourth)
{
    /* The four fields at 0xAC begin inside the halfword array at 0x18. */
    enum { firstParameter = (0xAC - 0x18) / sizeof(short) };
    u_short parameters[4] = {
        (u_short)first, (u_short)second, (u_short)third, (u_short)fourth
    };

    memcpy(&g_FieldTextBoxes[index]._pad[firstParameter], parameters, sizeof(parameters));
}

int func_800807B4(void)
{
    int selected = TEXT_BOX_UNINITIALIZED;
    int index;

    for (index = 0; index < 4; ++index) {
        FieldTextBox *window = &g_FieldTextBoxes[index];
        if (window->order != TEXT_BOX_UNINITIALIZED) {
            ++window->order;
        }
        if (selected == TEXT_BOX_UNINITIALIZED && window->order == TEXT_BOX_UNINITIALIZED) {
            selected = index;
        }
    }
    if (selected != TEXT_BOX_UNINITIALIZED) {
        g_FieldTextBoxes[selected].order = 0;
    }
    return selected;
}
