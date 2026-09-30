/* Retail84B40: callback-driven selection, with owner reloads across calls. */
u32 func_80084B40(u32 arg0) {
    u32 selected = ((u8 *)D_800C3EAC)[0x2E8];
    u32 status = 2;
    u32 actor = arg0 & 0xFF;
    u32 offset = actor * 64;
    D_800D3014 = 8;
    do {
        while (D_800D3014 == 8) {
            u16 own_mask = func_80089C08(arg0 & 0xFF);
            u16 target_mask = func_80089C08(selected & 0xFF);
            func_800BC404((own_mask | target_mask) & 0xFFFF);
            target_mask = func_80089C08(selected & 0xFF);
            func_800BCD98(target_mask & 0xFFFF);
            func_800716D8();
        }
        switch (D_800D3014) {
        case 4:
        case 6:
        case 7: {
            u8 *row = (u8 *)D_800C3EAC;
            status = 1;
            row += offset;
            row[0x3C] = selected;
            break;
        }
        case 5: {
            u16 own_mask = func_80089C08(actor);
            u8 *row = (u8 *)D_800C3EAC;
            u16 target_mask;
            row += offset;
            target_mask = func_80089C08(row[0x3C]);
            func_800BC404((own_mask | target_mask) & 0xFFFF);
            {
                u8 *current_row = (u8 *)D_800C3EAC;
                current_row += offset;
                target_mask = func_80089C08(current_row[0x3C]);
                status = 0;
                func_800BCD98(target_mask & 0xFFFF);
            }
            break;
        }
        case 0:
            selected = func_80084854(selected & 0xFF, 0);
            break;
        case 1:
            selected = func_80084854(selected & 0xFF, 1);
            break;
        case 2:
            selected = func_80084854(selected & 0xFF, 2);
            break;
        case 3:
            selected = func_80084854(selected & 0xFF, 3);
            break;
        }
        D_800D3014 = 8;
    } while ((status & 0xFF) == 2);
    return status & 0xFF;
}
