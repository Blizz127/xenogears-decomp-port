#include "message_controller_bindings.inc"

/* Retail file-1 controller at 0x801E6CE8..0x801E71D4.
 * Typed record accesses retain the original PSYQ scheduling of field loads. */
xbcf1_s32 xeno_battle_command_file1_controller(
    xbcf1_u16 arg0, xbcf1_u8 arg1, xbcf1_u16 arg2)
{
    xbcf1_u16 flags = arg2;
    xbcf1_u16 x = XBCF1_CONTROL(D_800D3278)->x;
    xbcf1_u16 y = XBCF1_CONTROL(D_800D3278)->y;
    xbcf1_u32 text_units = XBCF1_CONTROL(D_800D3278)->units;
    xbcf1_u16 layout_width = text_units * 12 + 24;
    xbcf1_u16 row_layout;
    xbcf1_u8 alternate_path = 0;
    xbcf1_s32 complete = 0;

    void *selected_string;
    xbcf1_u8 *restore_control;
    int i;

    if (flags == 0)
        flags = XBCF1_CONTROL(D_800D3278)->flags;

    if (XBCF1_CONTROL(D_800D3278)->phase == 0) {
        xbcf1_u16 sentinel_y;
        xeno_battle_command_file1_cycle_9C1C = 4;
        sentinel_y = XBCF1_CONTROL(D_800D3278)->y;

        if (sentinel_y == 0x7FFF) {
            y = 0x10;
            if (flags & 4)
                y = 0x8C;
        }

        if (XBCF1_CONTROL(D_800D3278)->rows >= 5)
            row_layout = 4;
        else
            row_layout = XBCF1_CONTROL(D_800D3278)->rows;
        row_layout = row_layout * 13 + 20;

        if (XBCF1_CONTROL(D_800D3278)->x == 0x7FFF)
            x = 0xA0 - ((xbcf1_u16)layout_width >> 1);

        if ((flags & 8) == 0) {
            if ((flags & 2) == 0 && arg1 != 0xFF) {
                layout_width += 0x40;
                alternate_path = 1;
            }

            if (XBCF1_CONTROL(D_800D3278)->x == 0x7FFF)
                x = 0xA0 - ((xbcf1_u16)layout_width >> 1);

            if ((xbcf1_u8)alternate_path == 0) {
                func_8008F8F4(0, (xbcf1_u16)x, (xbcf1_u16)y,
                              (xbcf1_u16)layout_width,
                              (xbcf1_u16)row_layout,
                              (((xbcf1_u16)flags >> 4) ^ 1) & 1, 1);

                while (XBCF1_U8(D_800D2D28, 0xBF) == 0)
                    func_800716D8();
            } else {
                xbcf1_u16 local_flags = flags;
                xbcf1_u16 local_x = (xbcf1_u16)x;
                xbcf1_u16 local_y = (xbcf1_u16)y;
                xbcf1_u16 local_width = (xbcf1_u16)layout_width;

                xeno_battle_command_file1_helper_6750(
                    arg1, local_flags, local_x, local_y, local_width);
                func_8008F8F4(0, local_x, local_y, local_width,
                              (xbcf1_u16)row_layout,
                              ((local_flags >> 4) ^ 1) & 1, 1);

                while (XBCF1_U8(D_800D2D28, 0xBF) == 0)
                    func_800716D8();

                XBCF1_U8(D_800D2D28, 0xC8) = 1;
                if ((flags & 1) == 0)
                    x += 0x40;
            }
        }

        func_80032F54(D_800D2DAC,
                      0x380,
                      0x100,
                      (xbcf1_u16)x + 12,
                      y + 8,
                      XBCF1_CONTROL(D_800D3278)->units * 3,
                      (xeno_battle_command_file1_y_9C34 = y + 8,
                       (xeno_battle_command_file1_x_9C30 = (xbcf1_u16)x + 12,
                        XBCF1_CONTROL(D_800D3278)->rows)));

        XBCF1_U8(D_800D2DAC, 0x58) = 4;
        XBCF1_U16(D_800D2DAC, 0x10) |= 2;
        func_80034614(D_800D2DAC);
        selected_string = GetStringEntry(D_800D3340, arg0);
        func_80034714(D_800D2DAC, selected_string);
        XBCF1_U8(D_800D2D28, 0xC9) = 1;
        XBCF1_CONTROL(D_800D3278)->phase = 1;
        func_800716D8();
    }

    if (XBCF1_U16(D_800D2DAC, 0x10) & 8) {
        if ((flags & 8) == 0) {
            xbcf1_s32 helper_x = XBCF1_S16(D_800D2DAC, 0x00) * 4;
            xbcf1_s32 helper_y = XBCF1_S16(D_800D2DAC, 0x02) * 14;

            /* Retail uses ADDU here.  Unsigned arithmetic gives the same
             * modulo-2^32 result even if saved_x/saved_y are poisoned. */
            helper_x = (xbcf1_s32)(
                (xbcf1_u32)helper_x +
                (xbcf1_u32)xeno_battle_command_file1_x_9C30 + 2u);
            helper_y = (xbcf1_s32)(
                (xbcf1_u32)helper_y +
                (xbcf1_u32)xeno_battle_command_file1_y_9C34 + 5u);
            xeno_battle_command_file1_helper_5B00(helper_x, helper_y);
        }

        XBCF1_U8(D_800D2D28, 0xCF) = 1;
        if (D_800D3014 == 4) {
            func_800345E0(D_800D2DAC);
            XBCF1_U8(D_800D2D28, 0xCF) = 0;
            XBCF1_U8(D_800D2D28, 0x9E) = 0;
        }
    }

    if ((XBCF1_U16(D_800D2DAC, 0x10) & 4) == 0) {
        XBCF1_U8(D_800D2D28, 0xC9) = 0;
        func_800346D4(D_800D2DAC);
        func_800716D8();
        XBCF1_U8(D_800D2D28, 0xC8) = 0;
        if ((flags & 8) == 0)
            func_8008FA60(0);
        complete = 1;

        XBCF1_CONTROL(D_800D3278)->phase = 0;
        restore_control = D_800D3278;
        for (i = 0; i < 5; i++)
            XBCF1_U16(restore_control, 0x7F6 + i * 2) =
                xeno_battle_command_file1_defaults_9C10[i];
    }

    return complete;
}
