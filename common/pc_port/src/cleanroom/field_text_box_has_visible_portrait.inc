int func_8009C538(int targetId)
{
    for (int slot = 0; slot < 4; ++slot) {
        const volatile FieldTextBox *window = &g_FieldTextBoxes[slot];

        if (window->visibility == 0 &&
            window->portrait.shouldRenderPortrait == 1 &&
            window->portrait.portraitID == targetId) {
            return -1;
        }
    }

    return 0;
}
