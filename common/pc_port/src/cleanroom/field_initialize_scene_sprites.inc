void func_800ABD18(void)
{
    RECT window = {0, 0, 255, 255};

    for (int column = 0; column < 5; ++column) {
        for (int buffer = 0; buffer < 2; ++buffer) {
            unsigned short page = GetTPage(1, 0, 640 + 64 * column, 0);
            SetDrawMode(&D_800B0188.drModes[column][buffer], 0, 0, page, &window);
        }

        SPRT *first = &D_800B0188.sprites[column][0];
        volatile SPRT *sprite = first;
        SetSprt(first);
        sprite->x0 = 128 * column;
        sprite->r0 = 128;
        sprite->g0 = 128;
        sprite->b0 = 128;
        sprite->y0 = 0;
        sprite->u0 = 0;
        sprite->v0 = 0;
        sprite->w = 128;
        sprite->h = 224;
        SetSemiTrans(first, 0);
        sprite->clut = (unsigned short)GetClut(0, 232);

        /* Sample four packet words before updating the second buffer. The
         * fences retain the word access order without assuming alignment. */
        unsigned int words[4];
        const unsigned char *source = (const unsigned char *)first;
        unsigned char *destination = (unsigned char *)&D_800B0188.sprites[column][1];
        for (int i = 0; i < 4; ++i) {
            __builtin_memcpy(&words[i], source + i * 4, 4);
            __atomic_signal_fence(__ATOMIC_SEQ_CST);
        }
        for (int i = 0; i < 4; ++i) {
            __builtin_memcpy(destination + i * 4, &words[i], 4);
            __atomic_signal_fence(__ATOMIC_SEQ_CST);
        }
        __builtin_memcpy(destination + 16, source + 16, 4);
    }
}
