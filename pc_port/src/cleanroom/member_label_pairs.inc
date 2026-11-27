void func_801C59E0(MenuString *records, u8 *indices, s32 offset, s32 count) {
    if (count <= 0)
        return;

    for (u32 first = 0; first < (u32)count; first += 2) {
        for (unsigned side = 0; side < 2; ++side) {
            u32 index = first + side;
            void *entry = GetStringEntry(g_Menu->unk2E0, indices[index]);
            void *work = g_Menu->unk4E0[0].pVramBuffer;
            records[index].width = (u8)SystemRenderStringEntry(entry, work, 24, side);
        }

        s32 row = (s32)((u32)offset + first) / 4;
        RECT rect;
        rect.x = 320 + ((first / 2) & 1u) * 32;
        rect.y = (s16)((u32)row * 13u);
        rect.w = 28;
        rect.h = 13;
        records[first].vramDest = rect;
        records[first + 1].vramDest = rect;

        func_801C57A0(&records[first], (s32)first, offset, 0);
        func_801C57A0(&records[first + 1], (s32)(first + 1), offset, 0);
        LoadImage(&records[first].vramDest, g_Menu->unk4E0[0].pVramBuffer);
        DrawSync(0);
    }
}
