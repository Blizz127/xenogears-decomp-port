#include "common.h"


#ifndef XENO_PC_PORT
/* func_80089CCC.s, byte-exact: the battle's debug-mode input handler (the
 * twin of main43's func_8008A3EC).  First wait for a controller, drawing the
 * pause letters and muting once while none is connected and restoring the
 * SPU and D_80059488 on reconnect.  Then, while paused (D_800C3444), keep
 * polling: func_80036410() resets the pad state; otherwise drain the pad
 * queue.  A set D_800C48EA or D_800C3EAC +0x2DB gives command 0xFF.  When
 * paused, pressed 4 + 8 together (with *D_8005917C live) sets D_800C48EA
 * and resumes.  Otherwise pressed 0x2000/0x4000/0x8000/0x1000 select
 * commands 0-3 (sfx 0x4C, D_800C3E28 history shifted), released
 * 0x20/0x40/0x80/0x10 commands 4-7 (sfx 0x4D/0x4E).  Released 1 gives
 * command 0xC and, when *D_8005917C is live, resets three party slots
 * (D_800D32A0/A8/B0 = 0x1C, D_800CCE30 records +0 = 4 and +0x1A = 0xFF).
 * Released 2 does nothing while *D_8005917C is -1; otherwise it gives
 * command 0xB, cycles D_8005959C mod 5 and loads the debug font once.
 * Released 0x100 gives command 0xD.  Released 0x800 gives command 0xE
 * and toggles the pause when D_800CCC58 allows it.  The last command
 * goes to D_800D3014.
 * Codegen (gcc 2.6.3, main42's BattleGcc263):
 * - Retail places the command 0-7/0xD handlers and the pause block ahead
 *   of the ControllerPopState loop.  That is loop.c's
 *   find_and_verify_loops moving each `{ ...; break; }` block that exits
 *   the inner loop to the barrier after `ControllerResetState();
 *   continue;` (-ds/-dL dumps).  Consistent with gcc's rule (a branch
 *   around the block, no jumps inside it), the released 1/2, `||` and
 *   `goto resume` blocks stay in place.  So the source is a plain do/while
 *   around a while loop, as in func_8008A3EC.
 * - D_800C48EA is read and set through an unsized array view: loop.c then
 *   keeps its address in s5 (la + 0(s5), frame 0x50) as retail; as a
 *   scalar it is addressed by %hi/%lo at each use (frame 0x48, 1140 vs
 *   1144).
 * - `++D_8005959C >= 5` tests the incremented value as retail (addiu, sb,
 *   andi, sltiu); a separate D_8005959C++ statement reloads it (+2 insns).
 * - saved is only set by a disconnect or a pause, as in retail: a resume
 *   without either reads it uninitialised (retail: whatever s3 held). */
extern int ControllerGetType(int controllerIndex);
extern int ControllerPopState(void);
extern void ControllerResetState(void);
extern int func_80036410(void);
extern void GraphicsDrawPauseLetters(int x, int y);
extern void SoundMuteAllSpuChannels(void);
extern void SoundEnableAllSpuChannels(void);
extern void* FontLoadFont(int sx, int sy, int w, int h, s32 f1, s32 f2, s32 f3, s32 f4, s32 f5, s32 f6, s32 f7);
extern void func_8003747C(void*);
extern void func_8008AA74(u32 v);
extern u16 g_C1ButtonStatePressedOnce;
extern u16 g_C1ButtonStateReleased;
extern s32* D_8005917C;
extern s32 D_80059488;
extern u8 D_8005959C;
extern u8 D_800C3444;
extern u8 D_800C48EA__a[] asm("D_800C48EA");
extern u8 D_800CCC58;
extern u8 D_800C3E28[];
extern u8 D_800CCE30[];
extern u8 D_800C3AA0;
extern u8 D_800D32A0;
extern u8 D_800D32A8;
extern u8 D_800D32B0;
extern u8 D_800D3014;
extern u8* D_800C3EAC;

void func_80089CCC(u8 unused) {
    u8 cmd;
    u8 shown;
    u8 waiting;
    s32 saved;

    cmd = 8;
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
    do {
        if (func_80036410() != 0) {
            ControllerResetState();
            continue;
        }
        while (ControllerPopState() != 0) {
            if (D_800C48EA__a[0] != 0 || D_800C3EAC[0x2DB] != 0) {
                cmd = 0xFF;
                break;
            }
            if (D_800C3444 != 0) {
                if (*D_8005917C != -1 && (g_C1ButtonStatePressedOnce & 4) && (g_C1ButtonStatePressedOnce & 8)) {
                    D_800C48EA__a[0] = 1;
                    goto resume;
                }
            } else {
                if (g_C1ButtonStatePressedOnce & 0x2000) {
                    func_8008AA74(0x4C);
                    cmd = 0;
                    D_800C3E28[0] = D_800C3E28[1];
                    D_800C3E28[1] = 0;
                    break;
                }
                if (g_C1ButtonStatePressedOnce & 0x4000) {
                    func_8008AA74(0x4C);
                    cmd = 1;
                    D_800C3E28[0] = D_800C3E28[1];
                    D_800C3E28[1] = 1;
                    break;
                }
                if (g_C1ButtonStatePressedOnce & 0x8000) {
                    func_8008AA74(0x4C);
                    cmd = 2;
                    D_800C3E28[0] = D_800C3E28[1];
                    D_800C3E28[1] = 2;
                    break;
                }
                if (g_C1ButtonStatePressedOnce & 0x1000) {
                    func_8008AA74(0x4C);
                    cmd = 3;
                    D_800C3E28[0] = D_800C3E28[1];
                    D_800C3E28[1] = 3;
                    break;
                }
                if (g_C1ButtonStateReleased & 0x20) {
                    cmd = 4;
                    func_8008AA74(0x4D);
                    break;
                }
                if (g_C1ButtonStateReleased & 0x40) {
                    cmd = 5;
                    func_8008AA74(0x4E);
                    break;
                }
                if (g_C1ButtonStateReleased & 0x80) {
                    cmd = 6;
                    func_8008AA74(0x4D);
                    break;
                }
                if (g_C1ButtonStateReleased & 0x10) {
                    cmd = 7;
                    func_8008AA74(0x4D);
                    break;
                }
                if (g_C1ButtonStateReleased & 1) {
                    cmd = 0xC;
                    if (*D_8005917C == -1) {
                        break;
                    }
                    D_800D32A0 = 0x1C;
                    D_800D32A8 = 0x1C;
                    D_800D32B0 = 0x1C;
                    D_800CCE30[0] = 4;
                    D_800CCE30[0x170] = 4;
                    D_800CCE30[0x2E0] = 4;
                    D_800CCE30[0x1A] = 0xFF;
                    D_800CCE30[0x18A] = 0xFF;
                    D_800CCE30[0x2FA] = 0xFF;
                    break;
                }
                if (g_C1ButtonStateReleased & 2) {
                    if (*D_8005917C == -1) {
                        break;
                    }
                    cmd = 0xB;
                    if (++D_8005959C >= 5) {
                        D_8005959C = 0;
                    }
                    if (D_800C3AA0 != 0) {
                        break;
                    }
                    func_8003747C((void*)0x80200000);
                    FontLoadFont(0x10, 0x10, 0x140, 0x100, 0x3E8, 0, 0x340, 0, 0x340, 0x20, 0);
                    D_800C3AA0++;
                    break;
                }
                if (g_C1ButtonStateReleased & 0x100) {
                    cmd = 0xD;
                    break;
                }
            }
            if (g_C1ButtonStateReleased & 0x800) {
                cmd = 0xE;
                if (D_800CCC58 == 0) {
                    break;
                }
                if (D_800C3444 == 0) {
                    GraphicsDrawPauseLetters(0x88, 0x64);
                    GraphicsDrawPauseLetters(0x88, 0x144);
                    SoundMuteAllSpuChannels();
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
    D_800D3014 = cmd;
}
extern u8* D_800D2D28;
extern u8* D_800C3EA4;
extern void LoadImage(void* rect, void* p);
typedef struct {
    u8 pad[0x8950];
    s16 rects[4][4];
    u32 data[4][0x18C];
} BattleTexAnim;

/* Every other frame (D_800D2D28 +0xAA parity), upload the next column of the
 * four animated texture strips of the D_800C3EA4 work block: rect i at
 * +0x8950 + i * 8 from strip i (0x18C words each, from +0x8970) at the
 * D_800D2D28 +0x30 column, which then advances by 4 and wraps past 0xC6. */
void func_8008A144(void) {
    D_800D2D28[0xAA]++;
    if (D_800D2D28[0xAA] & 1) {
        LoadImage(((BattleTexAnim*)D_800C3EA4)->rects[0], &((BattleTexAnim*)D_800C3EA4)->data[0][*(s32*)(D_800D2D28 + 0x30)]);
        LoadImage(((BattleTexAnim*)D_800C3EA4)->rects[1], &((BattleTexAnim*)D_800C3EA4)->data[1][*(s32*)(D_800D2D28 + 0x30)]);
        LoadImage(((BattleTexAnim*)D_800C3EA4)->rects[2], &((BattleTexAnim*)D_800C3EA4)->data[2][*(s32*)(D_800D2D28 + 0x30)]);
        LoadImage(((BattleTexAnim*)D_800C3EA4)->rects[3], &((BattleTexAnim*)D_800C3EA4)->data[3][*(s32*)(D_800D2D28 + 0x30)]);
        *(u32*)(D_800D2D28 + 0x30) += 4;
        if (*(u32*)(D_800D2D28 + 0x30) >= 0xC7) {
            *(u32*)(D_800D2D28 + 0x30) = 0;
        }
    }
}
#endif


#ifndef XENO_PC_PORT
extern u8 D_800CCC58;
#endif
#ifndef XENO_PC_PORT
extern u8* D_800D2D28;
#endif
#ifndef XENO_PC_PORT
extern u8* D_800C3EA4;
#endif
#ifndef XENO_PC_PORT
extern u8 D_800D39D4;
#endif
#ifndef XENO_PC_PORT
extern s32 D_800D3288;
#endif
extern void func_8007171C(void);
extern void func_80089CCC(u8 v);
extern void func_8008A144(void);

#ifndef XENO_PC_PORT
extern u8 D_800C3444;
#endif
#ifndef XENO_PC_PORT
extern u8 D_800C48EA;
#endif
#ifndef XENO_PC_PORT
extern u8 D_800C3E28[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800CCE30[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800C3AA0;
#endif
#ifndef XENO_PC_PORT
extern u8 D_800D32A0;
#endif
#ifndef XENO_PC_PORT
extern u8 D_800D32A8;
#endif
#ifndef XENO_PC_PORT
extern u8 D_800D32B0;
#endif
#ifndef XENO_PC_PORT
extern u8 D_800D3014;
#endif
#ifndef XENO_PC_PORT
extern u8* D_800C3EAC;
#endif

#ifdef XENO_PC_PORT
extern int ControllerGetType(int controllerIndex);
extern int ControllerPopState(void);
extern void ControllerResetState(void);
extern int func_80036410(void);
extern void GraphicsDrawPauseLetters(int x, int y);
extern void SoundMuteAllSpuChannels(void);
extern void SoundEnableAllSpuChannels(void);
extern void func_8003747C(void* p);
extern void* FontLoadFont(int sx, int sy, int w, int h, s32 f1, s32 f2, s32 f3,
                          s32 f4, s32 f5, s32 f6, s32 f7);
extern int LoadImage(void* rect, void* data);
extern void func_8008AA74(u32 v);
extern void PcPort_PadVblankPump(void);
extern u16 g_C1ButtonStatePressedOnce;
extern u16 g_C1ButtonStateReleased;
extern s32* D_8005917C;
extern s32 D_80059488;
extern u8 D_8005959C;

/* These bodies are for the port and the differential harnesses; the matching
 * build still assembles retail bytes for both. */

/* func_8008A144.s: every other frame, upload the next column of four
 * animated texture strips out of the battle work area. */
void func_8008A144(void) {
    D_800D2D28[0xAA]++;
    if (D_800D2D28[0xAA] & 1) {
        u32 col = *(u32*)(D_800D2D28 + 0x30) * 4u;

        LoadImage(D_800C3EA4 + 0x8950, D_800C3EA4 + col + 0x8970);
        col = *(u32*)(D_800D2D28 + 0x30) * 4u;
        LoadImage(D_800C3EA4 + 0x8958, D_800C3EA4 + col + 0x8FA0);
        col = *(u32*)(D_800D2D28 + 0x30) * 4u;
        LoadImage(D_800C3EA4 + 0x8960, D_800C3EA4 + col + 0x95D0);
        col = *(u32*)(D_800D2D28 + 0x30) * 4u;
        LoadImage(D_800C3EA4 + 0x8968, D_800C3EA4 + col + 0x9C00);
        *(u32*)(D_800D2D28 + 0x30) += 4;
        if (*(u32*)(D_800D2D28 + 0x30) >= 0xC7)
            *(u32*)(D_800D2D28 + 0x30) = 0;
    }
}

/* The pad-disconnect wait shared by the three battle input handlers: while
 * no controller is present, draw the pause letters once and mute; on
 * reconnect restore the SPU and the saved D_80059488. */
static void func_80089CCC_wait_pad(s32* saved)
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

/* func_80089CCC.s: the battle's debug-mode input handler.  Drains the pad
 * queue into the command byte D_800D3014 and implements Start-pause.  While
 * paused (D_800C3444) retail spins polling the queue without a Vsync; the
 * retail vblank IRQ refilled it.  The port has no IRQ, so the poll pumps the
 * emulated vblank, exactly as runtime_bridge does for the interpreted body. */
void func_80089CCC(u8 unused) {
    u8* s0 = D_800C3E28;
    u8* s2 = D_800CCE30;
    u8 cmd = 8;
    /* Retail leaves this register unset until a pause saves the volume; the
     * port seeds it with the current value so a resume without a pause
     * restores what is already there instead of stack garbage. */
    s32 saved = D_80059488;

    (void)unused;
    func_80089CCC_wait_pad(&saved);
    do {
        PcPort_PadVblankPump();
        if (func_80036410() != 0) {
            ControllerResetState();
            goto next;
        }
    pop:
        if (ControllerPopState() == 0)
            goto next;
        if (D_800C48EA != 0 || D_800C3EAC[0x2DB] != 0) {
            cmd = 0xFF;
            goto next;
        }
        if (D_800C3444 != 0) {
            if (*D_8005917C != -1 && (g_C1ButtonStatePressedOnce & 4) &&
                (g_C1ButtonStatePressedOnce & 8)) {
                D_800C48EA = 1;
                goto resume;
            }
            goto start;
        }
        {
            u16 pressed = g_C1ButtonStatePressedOnce;
            u16 released;

            if (pressed & 0x2000) {
                func_8008AA74(0x4C);
                cmd = 0;
                s0[0] = s0[1];
                s0[1] = 0;
                goto next;
            }
            if (pressed & 0x4000) {
                func_8008AA74(0x4C);
                cmd = 1;
                s0[0] = s0[1];
                s0[1] = 1;
                goto next;
            }
            if (pressed & 0x8000) {
                func_8008AA74(0x4C);
                cmd = 2;
                s0[0] = s0[1];
                s0[1] = 2;
                goto next;
            }
            if (pressed & 0x1000) {
                func_8008AA74(0x4C);
                cmd = 3;
                s0[0] = s0[1];
                s0[1] = 3;
                goto next;
            }
            released = g_C1ButtonStateReleased;
            if (released & 0x20) {
                cmd = 4;
                func_8008AA74(0x4D);
                goto next;
            }
            if (released & 0x40) {
                cmd = 5;
                func_8008AA74(0x4E);
                goto next;
            }
            if (released & 0x80) {
                cmd = 6;
                func_8008AA74(0x4D);
                goto next;
            }
            if (released & 0x10) {
                cmd = 7;
                func_8008AA74(0x4D);
                goto next;
            }
            if (released & 1) {
                cmd = 0xC;
                if (*D_8005917C == -1)
                    goto next;
                D_800D32A0 = 0x1C;
                D_800D32A8 = 0x1C;
                D_800D32B0 = 0x1C;
                s2[0] = 4;
                s2[0x170] = 4;
                s2[0x2E0] = 4;
                s2[0x1A] = 0xFF;
                s2[0x18A] = 0xFF;
                s2[0x2FA] = 0xFF;
                goto next;
            }
            if (released & 2) {
                if (*D_8005917C == -1)
                    goto next;
                D_8005959C++;
                cmd = 0xB;
                if (D_8005959C >= 5)
                    D_8005959C = 0;
                if (D_800C3AA0 != 0)
                    goto next;
                func_8003747C((void*)0x80200000);
                FontLoadFont(0x10, 0x10, 0x140, 0x100, 0x3E8, 0, 0x340, 0,
                             0x340, 0x20, 0);
                D_800C3AA0++;
                goto next;
            }
            if (released & 0x100) {
                cmd = 0xD;
                goto next;
            }
        }
    start:
        if (!(g_C1ButtonStateReleased & 0x800)) {
            PcPort_PadVblankPump();
            goto pop;
        }
        cmd = 0xE;
        if (D_800CCC58 == 0)
            goto next;
        if (D_800C3444 == 0) {
            GraphicsDrawPauseLetters(0x88, 0x64);
            GraphicsDrawPauseLetters(0x88, 0x144);
            SoundMuteAllSpuChannels();
            saved = D_80059488;
            D_800C3444 = 1;
            goto next;
        }
    resume:
        SoundEnableAllSpuChannels();
        D_80059488 = saved;
        D_800C3444 = 0;
    next:;
    } while (D_800C3444 != 0);
    D_800D3014 = cmd;
}
#endif /* XENO_PC_PORT */


/* func_8008A274.s */
void func_8008A274(u8 v) {
    if (D_800CCC58 != 0) {
        func_8007171C();
    }
    if ((v & 0xFF) == 0) {
        func_80089CCC(0);
    }
    D_800D2D28[0xA9] += 6;
    D_800D2D28[0xAB] += 1;
    if (*(u8*)(D_800C3EA4 + 0x6415) != 0) {
        if (*(u8*)(D_800C3EA4 + 0x6416) == 0) {
            *(s32*)(D_800C3EA4 + 0x6410) += 4;
            if (*(s32*)(D_800C3EA4 + 0x6410) >= 0x81) {
                *(s32*)(D_800C3EA4 + 0x6410) = 0x7C;
                *(u8*)(D_800C3EA4 + 0x6416) = 1;
            }
        } else {
            *(s32*)(D_800C3EA4 + 0x6410) -= 4;
            if (*(s32*)(D_800C3EA4 + 0x6410) < 0) {
                *(s32*)(D_800C3EA4 + 0x6410) = 4;
                *(u8*)(D_800C3EA4 + 0x6416) = 0;
            }
        }
    }
    switch (D_800D39D4) {
    case 1:
    case 3:
        D_800D3288 += 1;
        break;
    case 2:
    case 4:
        D_800D3288 -= 1;
        break;
    default:
        break;
    }
    func_8008A144();
}
