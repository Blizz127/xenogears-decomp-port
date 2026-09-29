/* Production ControllerInit / func_8003611C (src/slus_006.64/system/controller.c,
 * XENO_PC_PORT branch) against the independent retail-byte oracle
 * (controller_init_layout_oracle.py -> controller_init_layout_expected.h). */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "controller_init_layout_expected.h"

typedef unsigned char u8;
typedef unsigned short u16;

extern void ControllerInit(void);
extern short ControllerRemapButtonState(short buttonState);

/* Globals the kept controller.c functions touch (retail: main-exe .sdata/.bss). */
u8 g_C1Buffer[0x44];
u8 D_8005A1BC[16];
u8 D_8005938C;
int D_80059390;
unsigned char g_ControllerButtonMappings[8];
unsigned short g_ControllerButtonMasks[8];
int g_padCommEnable;
unsigned g_ControllerNumStates = 7;
int g_ControllerCurStateWriteIndex = 3, g_ControllerCurStateReadIndex = 4;
int g_ControllerIsStateStackFull = 1, D_80050200;
short D_800594EC = 1, D_800594E8 = 1, D_800594E0 = 1, D_800594DC = 1;
short D_800595CC = 1, D_800595C8 = 1;
unsigned short g_C1ButtonState = 1, g_C2ButtonState = 1;
unsigned short g_C1ButtonStatePressedOnce = 1, g_C2ButtonStatePressedOnce = 1;
unsigned short g_C1ButtonStateReleased = 1, g_C2ButtonStateReleased = 1;

static u8* s_pad[2];
static int s_pad_calls;
void PsyX_Pad_InitPad(int slot, unsigned char* padData)
{
    assert(slot == 0 || slot == 1);
    s_pad[slot] = padData;
    s_pad_calls++;
}

#define CHECK(cond, ...) do { if (!(cond)) { fprintf(stderr, "CONTROLLER INIT FAIL " __VA_ARGS__); \
    fputc('\n', stderr); return 1; } } while (0)

int main(void)
{
    memcpy(D_8005A1BC, kSeedAct, 16);
    memcpy(g_ControllerButtonMappings, kSdataMappings, 8);
    memcpy(g_ControllerButtonMasks, kSdataMasks, sizeof(kSdataMasks));
    D_8005938C = SEED_8005938C;
    D_80059390 = (int)SEED_80059390;

    ControllerInit();

    for (int i = 0; i < 8; i++)
        CHECK(g_ControllerButtonMappings[i] == kRetailMappings[i],
              "mapping[%d]=%u retail=%u", i, g_ControllerButtonMappings[i], kRetailMappings[i]);
    for (int i = 0; i < 16; i++)
        CHECK(D_8005A1BC[i] == kRetailAct[i], "actuator[%d]=%u retail=%u", i, D_8005A1BC[i], kRetailAct[i]);
    CHECK(D_8005938C == RETAIL_8005938C, "D_8005938C=%u", D_8005938C);
    CHECK((unsigned)D_80059390 == RETAIL_80059390, "D_80059390=%#x", D_80059390);
    CHECK(s_pad_calls == 2 && s_pad[0] == g_C1Buffer && s_pad[1] == g_C1Buffer + 0x22,
          "InitPAD buffers not g_C1Buffer / +0x22");
    CHECK(g_padCommEnable == 1, "StartPAD equivalent not enabled");
    CHECK(g_ControllerNumStates == 0 && g_ControllerIsStateStackFull == 0 && D_80050200 == 1,
          "ControllerResetState not run");

    /* The observable consequence: USA layout. Physical Circle (0x20) reaches
     * the game as Cross (0x40), Triangle (0x10) as Square (0x80), and back;
     * shoulders pass through. */
    CHECK((u16)ControllerRemapButtonState(0x20) == 0x40, "Circle -> %#x", (u16)ControllerRemapButtonState(0x20));
    CHECK((u16)ControllerRemapButtonState(0x40) == 0x20, "Cross -> %#x", (u16)ControllerRemapButtonState(0x40));
    CHECK((u16)ControllerRemapButtonState(0x10) == 0x80, "Triangle -> %#x", (u16)ControllerRemapButtonState(0x10));
    CHECK((u16)ControllerRemapButtonState(0x80) == 0x10, "Square -> %#x", (u16)ControllerRemapButtonState(0x80));
    CHECK((u16)ControllerRemapButtonState(0x0C) == 0x0C, "L1|R1 -> %#x", (u16)ControllerRemapButtonState(0x0C));
    puts("CONTROLLER INIT RETAIL PASS mappings/actuators/flags/pad buffers + USA remap");
    return 0;
}
