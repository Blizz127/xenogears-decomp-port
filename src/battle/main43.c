#include "common.h"


#ifndef XENO_PC_PORT
extern int ControllerGetType(int controllerIndex);
extern int ControllerPopState(void);
extern void ControllerResetState(void);
extern int func_80036410(void);
extern void GraphicsDrawPauseLetters(int x, int y);
extern void SoundMuteAllSpuChannels(void);
extern void SoundEnableAllSpuChannels(void);
extern void func_8008A144(void);
extern u16 g_C1ButtonStatePressedOnce;
extern u16 g_C1ButtonStateReleased;
extern s32* D_8005917C;
extern s32 D_80059488;
extern u8* D_800D2D28;
extern u8* D_800D3278;
extern u8 D_800C3444;
extern u8 D_800C48EA;
extern u8 D_800CCC58;
extern u8 D_800D3014;

typedef struct {
    u8 unk0[0x26];
    s16 timer;
    u8 active;
    u8 unk29[0xF];
} BattleTimer38;

/* Battle input handler for the D_800C3E4C == 2 mode.  First wait for a
 * controller (drawing the pause letters and muting once while none is
 * connected), run the texture-strip animation (func_8008A144) and tick the
 * 16 active D_800D3278 timers down to zero when D_800D2D28 +0xCA is set.
 * Then drain the pad queue: released 0x20 outside a pause asks
 * D_800D3014 = 4; released 0x800 toggles the pause when D_800CCC58 allows
 * it; pressed 0x4 + 0x8 together during a pause (with *D_8005917C live) sets
 * D_800C48EA and resumes.  The loop repeats while paused. */
void func_8008A3EC(u8 unused) {
    u8 shown;
    u8 waiting;
    s32 saved;
    s32 i;

    waiting = 1;
    shown = 0;
    do {
        if (ControllerGetType(0) == 0) {
            if (shown == 0) {
                GraphicsDrawPauseLetters(0x88, 0x64);
                GraphicsDrawPauseLetters(0x88, 0x144);
                SoundMuteAllSpuChannels();
                shown++;
                saved = D_80059488;
            }
        } else {
            waiting = 0;
            if (shown != 0) {
                SoundEnableAllSpuChannels();
                D_80059488 = saved;
            }
        }
    } while (waiting);
    func_8008A144();
    if (D_800D2D28[0xCA] != 0) {
        for (i = 0; i < 16; i++) {
            if (((BattleTimer38*)D_800D3278)[i].active != 0) {
                if (--((BattleTimer38*)D_800D3278)[i].timer < 0) {
                    ((BattleTimer38*)D_800D3278)[i].timer = 0;
                }
            }
        }
    }
    do {
        if (D_800D2D28[0xCF] == 0) {
            D_800D3014 = 0xFF;
        }
        if (func_80036410() != 0) {
            ControllerResetState();
            continue;
        }
        while (ControllerPopState() != 0) {
            if (D_800C3444 != 0) {
                if (*D_8005917C != -1 && (g_C1ButtonStatePressedOnce & 4) && (g_C1ButtonStatePressedOnce & 8)) {
                    D_800C48EA = 1;
                    goto resume;
                }
            } else if (g_C1ButtonStateReleased & 0x20) {
                D_800D3014 = 4;
                break;
            }
            if (g_C1ButtonStateReleased & 0x800) {
                if (D_800CCC58 == 0) {
                    break;
                }
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
        }
    } while (D_800C3444 != 0);
}
extern u32 D_800D32F8[];
extern u8* D_800D32F8__0 asm("D_800D32F8");
typedef struct {
    s32 a;
    s32 b;
} A684Pair;
extern A684Pair D_800CDCD0__p[] asm("D_800CDCD0");
extern A684Pair D_800CDCB8__p[] asm("D_800CDCB8");
extern u8 D_800C3EB0__b[] asm("D_800C3EB0");
/* Per-member battle record pointer (D_800D32F8 holds three of them). */
#define A684_REC(i) ((u8*)D_800D32F8[i])
extern void func_801DF270(void);
extern void func_801DF4C0(void);

/* Pause handling, then the per-member countdown pair (read through the
 * D_800CDCD0/D_800CDCB8 pair arrays, written through D_800C3EB0 + 0x9E20 /
 * 0x9E08).  Retail's codegen depends on: the u8 `all` and the `&=`-then-if
 * tail (all = t in the branch delay slot); record reads as separate
 * A684_REC expressions, not one shared local, so the third read gets its
 * own register (a1); and the D_800C3EB0 base read inside the loop, so
 * loop.c hoists it after the constant 1. */
void func_8008A684(u8 unused) {
    u8* w;
    u8 shown;
    u8 waiting;
    s32 saved;
    u8 all;
    s32 i;
    u8* first;

    all = 1;
    waiting = 1;
    shown = 0;
    do {
        if (ControllerGetType(0) == 0) {
            if (shown == 0) {
                GraphicsDrawPauseLetters(0x88, 0x64);
                GraphicsDrawPauseLetters(0x88, 0x144);
                SoundMuteAllSpuChannels();
                shown++;
                saved = D_80059488;
            }
        } else {
            waiting = 0;
            if (shown != 0) {
                SoundEnableAllSpuChannels();
                D_80059488 = saved;
            }
        }
    } while (waiting);
    if (D_800D2D28[0xCF] == 0) {
        D_800D3014 = 0xFF;
    }
    do {
        if (func_80036410() != 0) {
            ControllerResetState();
            continue;
        }
        while (ControllerPopState() != 0) {
            if (g_C1ButtonStateReleased & 0x20) {
                D_800D3014 = 4;
                break;
            }
            if (g_C1ButtonStateReleased & 0x800) {
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
        }
    } while (D_800C3444 != 0);
    if (D_800D2D28[0xA0] != 0 && A684_REC(0)[0x15F9] != 0) {
        for (i = 0; i < 3; i++) {
            if (A684_REC(i)[0x15FA] == 0) {
                if (D_800CDCD0__p[i].a == 0) {
                    A684_REC(i)[0x15FA] = 1;
                } else {
                    w = (u8*)(i * 8 + (s32)D_800C3EB0__b);
                    *(s32*)(w + 0x9E20) = D_800CDCD0__p[i].a - 1;
                    *(s32*)(w + 0x9E08) = D_800CDCB8__p[i].a + 1;
                }
            }
            if (A684_REC(i)[0x15FB] == 0) {
                if (D_800CDCD0__p[i].b == 0) {
                    A684_REC(i)[0x15FB] = 1;
                } else {
                    w = (u8*)(i * 8 + (s32)D_800C3EB0__b);
                    *(s32*)(w + 0x9E24) = D_800CDCD0__p[i].b - 1;
                    *(s32*)(w + 0x9E0C) = D_800CDCB8__p[i].b + 1;
                }
            }
            first = D_800D32F8__0;
            all &= A684_REC(i)[0x15FA];
            if (first[0x15F8] != 0) {
                all &= A684_REC(i)[0x15FB];
            }
        }
        if (all == 0) {
            func_801DF270();
            func_801DF4C0();
        } else {
            first[0x15F9] = 0;
        }
    }
}
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
/* 0x801DF270 / 0x801DF4C0 live in the module loaded at 0x801DE000 and have no
 * C body: func_8008A684 runs their retail bytes through BATTLE_GUEST_CALL0
 * instead of linking generated no-op stubs. */
#include "battle_guest_call.h"
extern int ControllerGetType(int controllerIndex);
extern int ControllerPopState(void);
extern void ControllerResetState(void);
extern int func_80036410(void);
extern void GraphicsDrawPauseLetters(int x, int y);
extern void SoundMuteAllSpuChannels(void);
extern void SoundEnableAllSpuChannels(void);
extern void func_8008A144(void);
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
            /* Retail 8008A984/8008A98C: jal with no argument set-up. */
            BATTLE_GUEST_CALL0(0x801DF270u);
            BATTLE_GUEST_CALL0(0x801DF4C0u);
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
