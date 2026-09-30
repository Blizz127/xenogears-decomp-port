#include "common.h"


#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/battle/nonmatchings/main43", func_8008A3EC);
INCLUDE_ASM("asm/battle/nonmatchings/main43", func_8008A684);
#endif


#ifndef XENO_PC_PORT
extern u8 D_800C3E4C;
#endif
extern void func_8008A684(u8 v);
extern void func_8008A274(u8 v);
extern void func_8008A3EC(u8 v);
extern u32 D_8005919C;
extern void func_80039DB8(u32 v);
#ifndef XENO_PC_PORT
extern u8 D_800D366C;
#endif

#ifndef XENO_PC_PORT
extern u8* D_800D2D28;
#endif
#ifndef XENO_PC_PORT
extern u8* D_800D3278;
#endif
#ifndef XENO_PC_PORT
extern u32 D_800D32F8[];
#endif
#ifndef XENO_PC_PORT
extern s32 D_800CDCB8[];
#endif
#ifndef XENO_PC_PORT
extern s32 D_800CDCD0[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800C3444;
#endif
#ifndef XENO_PC_PORT
extern u8 D_800C48EA;
#endif
#ifndef XENO_PC_PORT
extern u8 D_800CCC58;
#endif
#ifndef XENO_PC_PORT
extern u8 D_800D3014;
#endif

#ifdef XENO_PC_PORT
extern int ControllerGetType(int controllerIndex);
extern int ControllerPopState(void);
extern void ControllerResetState(void);
extern int func_80036410(void);
extern void GraphicsDrawPauseLetters(int x, int y);
extern void SoundMuteAllSpuChannels(void);
extern void SoundEnableAllSpuChannels(void);
extern void func_8008A144(void);
extern void func_801DF270(void);
extern void func_801DF4C0(void);
extern void PcPort_PadVblankPump(void);
extern u16 g_C1ButtonStatePressedOnce;
extern u16 g_C1ButtonStateReleased;
extern s32* D_8005917C;
extern s32 D_80059488;

/* Port-only bodies; the matching build still assembles retail bytes.  Both
 * are battle input handlers whose Start-pause spins on the pad queue without
 * a Vsync (retail's vblank IRQ refilled it).  The port has no IRQ, so each
 * poll pumps the emulated vblank, as runtime_bridge does for the interpreted
 * bodies; see PcPort_PadVblankPump. */

/* Wait for a controller: while none is connected draw the pause letters once
 * and mute; on reconnect restore the SPU and the saved D_80059488. */
static void func_8008A3EC_wait_pad(s32* saved)
{
    u8 shown = 0;

    for (;;) {
        PcPort_PadVblankPump();
        if (ControllerGetType(0) != 0) {
            if (shown != 0) {
                SoundEnableAllSpuChannels();
                D_80059488 = *saved;
            }
            return;
        }
        if (shown == 0) {
            GraphicsDrawPauseLetters(0x88, 0x64);
            GraphicsDrawPauseLetters(0x88, 0x144);
            SoundMuteAllSpuChannels();
            shown++;
            *saved = D_80059488;
        }
    }
}

/* func_8008A3EC.s: input handler for the D_800C3E4C == 2 battle mode. */
void func_8008A3EC(u8 unused) {
    /* Retail leaves this register unset until a pause saves the volume; the
     * port seeds it with the current value (see func_80089CCC). */
    s32 saved = D_80059488;

    (void)unused;
    func_8008A3EC_wait_pad(&saved);
    func_8008A144();
    if (D_800D2D28[0xCA] != 0) {
        u8* t = D_800D3278;
        s32 i;

        for (i = 0; i < 16; i++, t += 0x38) {
            if (t[0x28] != 0) {
                s16 v = (s16)(*(u16*)(t + 0x26) - 1);

                *(s16*)(t + 0x26) = v;
                if (v < 0)
                    *(s16*)(t + 0x26) = 0;
            }
        }
    }
    do {
        if (D_800D2D28[0xCF] == 0)
            D_800D3014 = 0xFF;
        PcPort_PadVblankPump();
        if (func_80036410() != 0) {
            ControllerResetState();
            continue;
        }
        for (;;) {
            if (ControllerPopState() == 0)
                break;
            if (D_800C3444 != 0) {
                if (*D_8005917C != -1 && (g_C1ButtonStatePressedOnce & 4) &&
                    (g_C1ButtonStatePressedOnce & 8)) {
                    D_800C48EA = 1;
                    goto resume;
                }
            } else if (g_C1ButtonStateReleased & 0x20) {
                D_800D3014 = 4;
                break;
            }
            if (!(g_C1ButtonStateReleased & 0x800)) {
                PcPort_PadVblankPump();
                continue;
            }
            if (D_800CCC58 == 0)
                break;
            if (D_800C3444 == 0) {
                SoundMuteAllSpuChannels();
                GraphicsDrawPauseLetters(0x88, 0x64);
                GraphicsDrawPauseLetters(0x88, 0x144);
                saved = D_80059488;
                D_800C3444 = 1;
                break;
            }
        resume:
            SoundEnableAllSpuChannels();
            D_80059488 = saved;
            D_800C3444 = 0;
            break;
        }
    } while (D_800C3444 != 0);
}

/* func_8008A684.s: input handler for the D_800C3E4C == 0 battle mode, then
 * the per-character countdown pair at D_800CDCB8/D_800CDCD0 for the three
 * records of D_800D32F8 (flags +0x15F8..+0x15FB). */
void func_8008A684(u8 unused) {
    s32 saved = D_80059488;

    (void)unused;
    func_8008A3EC_wait_pad(&saved);
    if (D_800D2D28[0xCF] == 0)
        D_800D3014 = 0xFF;
    do {
        PcPort_PadVblankPump();
        if (func_80036410() != 0) {
            ControllerResetState();
            continue;
        }
        for (;;) {
            u16 released;

            if (ControllerPopState() == 0)
                break;
            released = g_C1ButtonStateReleased;
            if (released & 0x20) {
                D_800D3014 = 4;
                break;
            }
            if (!(released & 0x800)) {
                PcPort_PadVblankPump();
                continue;
            }
            if (D_800C3444 == 0) {
                SoundMuteAllSpuChannels();
                GraphicsDrawPauseLetters(0x88, 0x64);
                GraphicsDrawPauseLetters(0x88, 0x144);
                saved = D_80059488;
                D_800C3444 = 1;
                break;
            }
            SoundEnableAllSpuChannels();
            D_80059488 = saved;
            D_800C3444 = 0;
            break;
        }
    } while (D_800C3444 != 0);

    if (D_800D2D28[0xA0] != 0 &&
        ((u8*)PSX_ADDR(D_800D32F8[0]))[0x15F9] != 0) {
        u32 all = 1;
        u8* first = NULL;
        s32 i;

        for (i = 0; i < 3; i++) {
            u8* rec = (u8*)PSX_ADDR(D_800D32F8[i]);

            if (rec[0x15FA] == 0) {
                if (D_800CDCD0[i * 2] == 0) {
                    rec[0x15FA] = 1;
                } else {
                    D_800CDCD0[i * 2]--;
                    D_800CDCB8[i * 2]++;
                }
            }
            rec = (u8*)PSX_ADDR(D_800D32F8[i]);
            if (rec[0x15FB] == 0) {
                if (D_800CDCD0[i * 2 + 1] == 0) {
                    rec[0x15FB] = 1;
                } else {
                    D_800CDCD0[i * 2 + 1]--;
                    D_800CDCB8[i * 2 + 1]++;
                }
            }
            rec = (u8*)PSX_ADDR(D_800D32F8[i]);
            first = (u8*)PSX_ADDR(D_800D32F8[0]);
            all &= rec[0x15FA];
            if (first[0x15F8] != 0)
                all &= rec[0x15FB];
        }
        if (all == 0) {
            func_801DF270();
            func_801DF4C0();
        } else {
            first[0x15F9] = 0;
        }
    }
}
#endif /* XENO_PC_PORT */


/* func_8008A9C0.s */
void func_8008A9C0(u8 v) {
    switch (D_800C3E4C) {
    case 0:
        func_8008A684(v);
        break;
    case 1:
        func_8008A274(v);
        break;
    case 2:
        func_8008A3EC(v);
        break;
    default:
        break;
    }
}
/* func_8008AA40.s */
void func_8008AA40(u8 a) {
#ifdef XENO_PC_PORT
    /* The interpreter stores a 32-bit bank address in the shared selector.
     * Retail 8008AA44/8008AA50 load that word then its +0x14 halfword.
     * Translate RAM aliases; native bank allocations retain their address. */
    u32 bank = D_8005919C;
    u8* p = (bank < 0x200000u || (bank & 0xFFE00000u) == 0x80000000u ||
             (bank & 0xFFE00000u) == 0xA0000000u)
                ? PSX_ADDR(bank) : (void*)(uintptr_t)bank;
    u32 v = *(u16*)(p + 0x14);
#else
    u32 v = *(u16*)(D_8005919C + 0x14);
#endif

    func_80039DB8((v << 16) | (a & 0xFF));
}
/* Retail 8008AA74 accepts the argument word and masks it at 8008AA8C,
 * only when forwarding to 8008AA40. Keep caller declarations word-sized. */
void func_8008AA74(u32 v) {
    if (D_800D366C != 0) {
        func_8008AA40(v & 0xFF);
    }
}
