#ifndef XENO_MENU_STRING_PACKET_FIELDS_H
#define XENO_MENU_STRING_PACKET_FIELDS_H

static inline void MenuStringInitializePackets(MenuString *string, int index, s32 yOffset, u32 attributes)
{
    volatile MenuString *const record = string;
    const u8 style = (u8)attributes;

    for (int bank = 0; bank < 2; ++bank) {
        POLY_FT4 *const packet = &string->polys[bank];
        volatile POLY_FT4 *const fields = packet;
        SetPolyFT4(packet);
        SetSemiTrans(packet, 0);
        SetShadeTex(packet, 0);
        fields->r0 = 128;
        fields->g0 = 128;
        fields->b0 = 128;

        if (!style) {
            record->unk7C = (u8)((u32)index & 1u);
            fields->tpage = (u16)GetTPage(0, 0, 320, 0);
            const u32 sum = (u32)index + (u32)yOffset;
            const s32 signedSum = sum <= 0x7fffffffu ? (s32)sum : -1 - (s32)~sum;
            const u32 u = ((u32)(index / 2) & 1u) * 128u;
            const u32 v = (u32)(signedSum / 4) * 13u;

            fields->u0 = (u8)u;
            fields->v0 = (u8)v;
            const u8 upperWidth = record->width;
            fields->u1 = (u8)(u + upperWidth);
            fields->v1 = (u8)v;
            fields->u2 = (u8)u;
            fields->v2 = (u8)(v + 13u);
            const u8 lowerWidth = record->width;
            fields->u3 = (u8)(u + lowerWidth);
            fields->v3 = (u8)(v + 13u);
        } else {
            u16 pageFlag = 0;
            if (!(style & 0x80u)) {
                SetSemiTrans(packet, 1);
                fields->r0 = 32;
                fields->g0 = 32;
                fields->b0 = 32;
                pageFlag = 32;
            }
            record->unk7C = (u8)((attributes & 127u) + 255u);
            const u16 page = (u16)GetTPage(0, 0, 384, 128);
            fields->tpage = (u16)(page | pageFlag);
            const u32 u = ((u32)index & 1u) * 96u;
            const u32 v = (u32)(index / 2) * 13u + (u32)yOffset;

            fields->u0 = (u8)u;
            fields->v0 = (u8)v;
            const u8 upperWidth = record->width;
            fields->v1 = (u8)v;
            fields->u2 = (u8)u;
            fields->v2 = (u8)(v + 13u);
            fields->u1 = (u8)(u + upperWidth);
            const u8 lowerWidth = record->width;
            fields->v3 = (u8)(v + 13u);
            fields->u3 = (u8)(u + lowerWidth);
        }
        const u16 palette = record->unk7C ? *(const volatile u16 *)&g_SystemPalette2
                                          : *(const volatile u16 *)&g_SystemPalette1;
        fields->clut = palette;
    }
    record->unk7F = 0;
}

#endif
