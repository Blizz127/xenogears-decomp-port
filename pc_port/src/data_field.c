#include "common.h"
#include "field/main.h"
#include "field/particles.h"
/* Retail tables below are zero storage filled at startup from the user's
 * disc/field.bin by the load table at the end of this file (retail_data.h). */
/* Retail field.bin [800AEB68,800AF278): 117 texture rectangles followed
 * by 109 placement records. Interior symbols must share this storage;
 * separate zero-filled placeholders cannot represent these tables. */
u16 g_FieldOverlayLayoutData[226][4];
_Static_assert(sizeof(g_FieldOverlayLayoutData) == 0x710, "retail overlay layout size");
asm(".globl D_800AEB68\n.set D_800AEB68, g_FieldOverlayLayoutData + 0");
asm(".globl D_800AEB6A\n.set D_800AEB6A, g_FieldOverlayLayoutData + 2");
asm(".globl D_800AEB6C\n.set D_800AEB6C, g_FieldOverlayLayoutData + 4");
asm(".globl D_800AEB6E\n.set D_800AEB6E, g_FieldOverlayLayoutData + 6");
asm(".globl D_800AEF10\n.set D_800AEF10, g_FieldOverlayLayoutData + 0x3A8");
asm(".globl D_800AEF14\n.set D_800AEF14, g_FieldOverlayLayoutData + 0x3AC");
asm(".globl D_800AEF16\n.set D_800AEF16, g_FieldOverlayLayoutData + 0x3AE");


/*
 * The field image-transfer ring is one contiguous retail BSS object.  The
 * host stub generator cannot infer that relationship when it emits one
 * placeholder per undefined symbol: D_800AF5E8 is a 0x100-byte table of
 * 32 eight-byte RECTs, followed by 0x40-byte source-X and source-Y tables,
 * and the ring index at +0x180.  Keep the retail offsets explicit so a RECT
 * write can never overlap either table on the LP64 host.
 */
unsigned char g_FieldMoveImageBss[0x182] __attribute__((aligned(8)));
asm(".globl D_800AF5E8\n.set D_800AF5E8, g_FieldMoveImageBss + 0x000");
asm(".globl D_800AF6E8\n.set D_800AF6E8, g_FieldMoveImageBss + 0x100");
asm(".globl D_800AF728\n.set D_800AF728, g_FieldMoveImageBss + 0x140");
asm(".globl D_800AF768\n.set D_800AF768, g_FieldMoveImageBss + 0x180");

/* Retail stores the three party skin-buffer pointers contiguously at
 * 0x8005A414, 0x8005A418, and 0x8005A41C.  Several field routines name the
 * table while the title transition names slots 1 and 2 directly.  The host
 * stub generator used to create three unrelated objects for those names,
 * leaving func_800A73E8 with NULL pointers after
 * FieldPartyAllocateSkinDataBuffers populated the table.
 *
 * The PC port is deliberately built as LP64, so use host pointer strides for
 * the aliases.  The retail-address names describe identity, not host byte
 * spacing. */
_Static_assert(sizeof(void*) == 8, "xeno-port field data requires LP64 pointers");
void* g_PartyDataBuffers[3];
asm(".globl D_8005A418\n.set D_8005A418, g_PartyDataBuffers + 8");
asm(".globl D_8005A41C\n.set D_8005A41C, g_PartyDataBuffers + 16");

/* Field overlay .rodata debug string @0x8006FC48 (asm/field/data/0.rodata.s
 * line 160): the "POLYCHECK %d\n" printf format used by func_80084158's
 * interaction-region branch. The sibling "HITOFF\n" string (D_8006FC58) is
 * already covered by the zeroed stub set; this one is newly referenced. */
char D_8006FC48[14];

/*
 * data_field.c - migrated field overlay initialized data for the Xenogears PC port.
 *
 * D_800ADC24: scene-palette mask/data table (field overlay .data).
 * D_800ADC44: UI texture descriptor table (field overlay .data).
 * Source: asm/field/data/3DF78.data.s label D_800ADC44 (file offset 0x3E154).
 * Field overlay base = 0x8006FAF0; RAM = 0x800ADC44.
 *
 * Full symbol size = 0x6C (108 bytes) = 54 halfwords = 9 entries × 6 halfwords.
 * FieldLoadUITextures reads the first 8 entries (48 halfwords / 96 bytes);
 * the 9th entry is part of the symbol but used elsewhere.
 *
 * Format per entry: {timX, timY, clutX, clutY, clutW, clutH}.
 * Entry 0: {672, 448, 0, 251, 0, 0}
 *
 * Defining this symbol removes it from the auto-generated zeroed data-stub set,
 * so FieldLoadUITextures gets real texture descriptors instead of zeros.
 * This does NOT affect the matching/decomp build (which links the asm .data
 * section directly); it only provides real data for the PC port.
 */

/* `func_80074108` uses the first eight halfwords as scene-flag masks while it
 * builds the 0x80-colour CLUT copied from D_800AFC08.  Keep the complete
 * 0x20-byte retail symbol contiguous: the trailing palette words are part of
 * the same data object at 0x800ADC24. */
unsigned short D_800ADC24[16];

unsigned short D_800ADC44[54];

/* Party-sprite VRAM save/restore coordinate tables (retail .data @ 800ADCB0 /
 * 800ADCC8, source asm/field/data/3DF78.data.s, file offsets 0x3E1C0/0x3E1D8).
 * Six {x, y} halfword pairs each.
 *
 * func_800799D4 (the field menu opener, src/field/main/misc4.c) blits six
 * 0x40 x 0x20 rects from D_800ADCB0 out to D_800ADCC8 before running MenuMain
 * and back again afterwards, so the menu can reuse the party sprites' VRAM.
 * These sat in the 0x30-byte gap between the already-migrated D_800ADC44 and
 * g_FieldData_800ADCE0, so the stub generator zero-filled them: every blit
 * became MoveImage({0,0,64,32}, 0, 0) -- a no-op self-copy of the top-left of
 * VRAM -- and the party sprite pages were neither saved nor restored across a
 * menu.  Party members whose frames are pre-uploaded (the frameHeader & 0x8000
 * path of func_8001DAE8) therefore came back from the menu textured with
 * whatever the menu left behind, or with 0x0000 texels, which the PSX blender
 * treats as fully transparent -- the character renders its quads and shows
 * nothing.  Retail values restore the real behaviour.
 *
 * Field side: the five 0x40-wide pages at y=0xE0 plus one at (0x100, 0x1E0).
 * Backup side: the scratch VRAM column at x=0x2C0, y=0x00..0xA0. */
unsigned short D_800ADCB0[12];

unsigned short D_800ADCC8[12];

/* 8-way angle LUTs (retail .data @ 800AEA34 / 800AEA44). */
unsigned short g_FieldAngleToDirectionLUT[8];

unsigned short D_800AEA44[8];

/* Field particle shapes: 21 records of 12 halfwords at 800AF27C..800AF474.
 * The first record's named halfwords are views into ONE table, not separate
 * zero-filled globals. Source: asm/field/data/3DF78.data.s and disc/field.bin.
 * The following eight bytes are the particle camera-relative direction LUT. */
u16 g_FieldParticleShapes[21][12];
_Static_assert(sizeof(g_FieldParticleShapes) == 0x1F8, "retail particle shape table");
#define PARTICLE_SHAPE_ALIAS(name, offset) \
    asm(".globl " #name "\n.set " #name ", g_FieldParticleShapes + " #offset)
PARTICLE_SHAPE_ALIAS(D_800AF27C, 0x00);
PARTICLE_SHAPE_ALIAS(D_800AF27E, 0x02);
PARTICLE_SHAPE_ALIAS(D_800AF280, 0x04);
PARTICLE_SHAPE_ALIAS(D_800AF282, 0x06);
PARTICLE_SHAPE_ALIAS(D_800AF284, 0x08);
PARTICLE_SHAPE_ALIAS(D_800AF286, 0x0A);
PARTICLE_SHAPE_ALIAS(D_800AF288, 0x0C);
PARTICLE_SHAPE_ALIAS(D_800AF28A, 0x0E);
PARTICLE_SHAPE_ALIAS(D_800AF28C, 0x10);
PARTICLE_SHAPE_ALIAS(D_800AF28E, 0x12);
PARTICLE_SHAPE_ALIAS(D_800AF290, 0x14);
PARTICLE_SHAPE_ALIAS(D_800AF292, 0x16);
#undef PARTICLE_SHAPE_ALIAS
u8 D_800AF474[8];

unsigned short D_800AEA54[8];

/* Relative 8-way yaw offsets per camera direction (8 dirs × 8 facings).
 * Consumed by func_8009B708 as signed steps; each unit → (step<<25)/duration. */
short D_800AEA64[64];

/* FaceId → archive-pair table (dir 4 entries +0x46). 90 faces × 2 bytes. */
unsigned char D_800AE1E0[180];

/* Portrait VRAM dest per load slot: {x,y,clutX,clutY} × 2 TIMs × 4 slots. */
short D_800AEAE4[32];

/* Distortion capture-strip source rectangles: 15 {x,y} pairs. */
unsigned short D_800AEB24[30];

/* Portrait TIM buffer pointers + round-robin slot index (retail .data zeros). */
s32 D_800ADB0C = 0;
void* D_800ADB10 = NULL;
void* D_800ADB14 = NULL;

unsigned short g_FieldData_800ADCE0[0x1B0 / 2] __attribute__((aligned(8)));

#define FIELD_DATA_ALIAS(name, offset) \
    asm(".globl " #name "\n.set " #name ", g_FieldData_800ADCE0 + " #offset)

FIELD_DATA_ALIAS(D_800ADCE0, 0x000);
FIELD_DATA_ALIAS(D_800ADD28, 0x048);
FIELD_DATA_ALIAS(D_800ADD70, 0x090);
FIELD_DATA_ALIAS(D_800ADDB8, 0x0D8);
FIELD_DATA_ALIAS(D_800ADE00, 0x120);
FIELD_DATA_ALIAS(D_800ADE02, 0x122);
FIELD_DATA_ALIAS(D_800ADE04, 0x124);
FIELD_DATA_ALIAS(D_800ADE06, 0x126);
FIELD_DATA_ALIAS(D_800ADE08, 0x128);
FIELD_DATA_ALIAS(D_800ADE0A, 0x12A);
FIELD_DATA_ALIAS(D_800ADE30, 0x150);
FIELD_DATA_ALIAS(D_800ADE70, 0x190);

#undef FIELD_DATA_ALIAS

u32 D_800ADE90 = 0;
u32 D_800ADE94 = 0;
u32 D_800ADE98 = 0;

unsigned short g_FieldData_800ADE9C[0xB8 / 2] __attribute__((aligned(8)));

#define FIELD_DATA_ALIAS(name, offset) \
    asm(".globl " #name "\n.set " #name ", g_FieldData_800ADE9C + " #offset)

FIELD_DATA_ALIAS(D_800ADE9C, 0x000);
FIELD_DATA_ALIAS(D_800ADEDC, 0x040);
FIELD_DATA_ALIAS(D_800ADEDE, 0x042);
FIELD_DATA_ALIAS(D_800ADEE0, 0x044);
FIELD_DATA_ALIAS(D_800ADEE2, 0x046);
FIELD_DATA_ALIAS(D_800ADF04, 0x068);
FIELD_DATA_ALIAS(D_800ADF06, 0x06A);
FIELD_DATA_ALIAS(D_800ADF08, 0x06C);
FIELD_DATA_ALIAS(D_800ADF0A, 0x06E);
FIELD_DATA_ALIAS(D_800ADF34, 0x098);

#undef FIELD_DATA_ALIAS

u16 g_FieldData_800ADF54[8] __attribute__((aligned(8)));

#define FIELD_DATA_ALIAS(name, offset) \
    asm(".globl " #name "\n.set " #name ", g_FieldData_800ADF54 + " #offset)

FIELD_DATA_ALIAS(D_800ADF54, 0x000);
FIELD_DATA_ALIAS(D_800ADF56, 0x002);

#undef FIELD_DATA_ALIAS

u16 g_FieldData_800ADF64[0x68 / 2] __attribute__((aligned(8)));

#define FIELD_DATA_ALIAS(name, offset) \
    asm(".globl " #name "\n.set " #name ", g_FieldData_800ADF64 + " #offset)

FIELD_DATA_ALIAS(D_800ADF64, 0x000);
FIELD_DATA_ALIAS(D_800ADF68, 0x004);
FIELD_DATA_ALIAS(D_800ADF88, 0x024);
FIELD_DATA_ALIAS(D_800ADFA8, 0x044);
FIELD_DATA_ALIAS(D_800ADFB8, 0x054);
FIELD_DATA_ALIAS(D_800ADFC4, 0x060);

#undef FIELD_DATA_ALIAS

unsigned char g_FieldData_800ADFCC[0x94] __attribute__((aligned(8)));

#define FIELD_DATA_ALIAS(name, offset) \
    asm(".globl " #name "\n.set " #name ", g_FieldData_800ADFCC + " #offset)

FIELD_DATA_ALIAS(D_800ADFCC, 0x000);
FIELD_DATA_ALIAS(D_800ADFCD, 0x001);

#undef FIELD_DATA_ALIAS

asm(".data\n"
    ".balign 8\n"
    ".globl g_FieldScriptVMHandlers\n"
    "g_FieldScriptVMHandlers:\n");

#define FIELD_VM_HANDLER(name) asm(".quad " #name "\n");
FIELD_VM_HANDLER(func_800A1B70)
FIELD_VM_HANDLER(FieldScriptVMHandlerJmp)
FIELD_VM_HANDLER(FieldScriptVMHandlerConditionalJmp)
FIELD_VM_HANDLER(func_8009C104)
FIELD_VM_HANDLER(func_800A1A8C)
FIELD_VM_HANDLER(func_800A17F4)
FIELD_VM_HANDLER(func_800A1730)
FIELD_VM_HANDLER(func_8009EB78)
FIELD_VM_HANDLER(func_8009ED68)
FIELD_VM_HANDLER(func_8009F0A0)
FIELD_VM_HANDLER(FieldScriptHandleTriggerZone2D)
FIELD_VM_HANDLER(func_800A1624)
FIELD_VM_HANDLER(func_8009F5A8)
FIELD_VM_HANDLER(func_800A18B8)
FIELD_VM_HANDLER(func_80092404)
FIELD_VM_HANDLER(func_800923E4)
FIELD_VM_HANDLER(func_80098C00)
FIELD_VM_HANDLER(func_80098C3C)
FIELD_VM_HANDLER(func_80093200)
FIELD_VM_HANDLER(FieldScriptVMHandlerNop)
FIELD_VM_HANDLER(FieldScriptVMHandlerDisableRandomEncounters)
FIELD_VM_HANDLER(func_80093C6C)
FIELD_VM_HANDLER(func_800A08B8)
FIELD_VM_HANDLER(func_8009E91C)
FIELD_VM_HANDLER(func_8009E83C)
FIELD_VM_HANDLER(func_8009E4BC)
FIELD_VM_HANDLER(func_8009E428)
FIELD_VM_HANDLER(func_8009E35C)
FIELD_VM_HANDLER(func_8009E2C8)
FIELD_VM_HANDLER(func_8009E248)
FIELD_VM_HANDLER(func_8009E208)
FIELD_VM_HANDLER(func_8009E1A0)
FIELD_VM_HANDLER(func_8009E10C)
FIELD_VM_HANDLER(func_8009E094)
FIELD_VM_HANDLER(FieldScriptVMHandlerShowActor)
FIELD_VM_HANDLER(FieldScriptVMHandlerHideActor)
FIELD_VM_HANDLER(FieldScriptVMHandlerShowActorById)
FIELD_VM_HANDLER(FieldScriptVMHandlerHideActorById)
FIELD_VM_HANDLER(FieldScriptVMHandlerSleep)
FIELD_VM_HANDLER(func_8009DC4C)
FIELD_VM_HANDLER(FieldScriptVMHandlerEnableActorVM)
FIELD_VM_HANDLER(func_8009DAC4)
FIELD_VM_HANDLER(FieldScriptVMHandlerDisableDialogActivation)
FIELD_VM_HANDLER(FieldScriptVMHandlerEnableDialogActivation)
FIELD_VM_HANDLER(FieldScriptVMHandlerPlayAnimation)
FIELD_VM_HANDLER(FieldScriptVMHandlerGetActorPosition)
FIELD_VM_HANDLER(FieldScriptVMHandlerGetActorDirection)
FIELD_VM_HANDLER(FieldScriptVMWriteCurCharacterID)
FIELD_VM_HANDLER(FieldScriptVMWritePartyLeaderCharacterID)
FIELD_VM_HANDLER(func_800961A0)
FIELD_VM_HANDLER(func_800961C8)
FIELD_VM_HANDLER(func_800961F0)
FIELD_VM_HANDLER(func_80096214)
FIELD_VM_HANDLER(FieldScriptVMHandlerVariableAssign)
FIELD_VM_HANDLER(FieldScriptVMHandlerVariableSetTrue)
FIELD_VM_HANDLER(FieldScriptVMHandlerVariableSetFalse)
FIELD_VM_HANDLER(FieldScriptVMHandlerVariableAdd)
FIELD_VM_HANDLER(FieldScriptVMHandlerVariableSub)
FIELD_VM_HANDLER(FieldScriptVMHandlerVariableSetBit)
FIELD_VM_HANDLER(FieldScriptVMHandlerVariableUnsetBit)
FIELD_VM_HANDLER(FieldScriptVMHandlerIncVariable)
FIELD_VM_HANDLER(FieldScriptVMHandlerDecVariable)
FIELD_VM_HANDLER(FieldScriptVMHandlerVariableAND)
FIELD_VM_HANDLER(FieldScriptVMHandlerVariableOR)
FIELD_VM_HANDLER(FieldScriptVMHandlerVariableXOR)
FIELD_VM_HANDLER(FieldScriptVMHandlerLShiftVariable)
FIELD_VM_HANDLER(FieldScriptVMHandlerRShiftVariable)
FIELD_VM_HANDLER(FieldScriptVMHandlerRandVariable)
FIELD_VM_HANDLER(func_80098184)
FIELD_VM_HANDLER(func_80097864)
FIELD_VM_HANDLER(func_80092808)
FIELD_VM_HANDLER(func_80092EA0)
FIELD_VM_HANDLER(func_80093CD0)
FIELD_VM_HANDLER(func_80093D48)
FIELD_VM_HANDLER(func_80099980)
FIELD_VM_HANDLER(func_80098430)
FIELD_VM_HANDLER(func_800979F0)
FIELD_VM_HANDLER(func_80097954)
FIELD_VM_HANDLER(func_80098370)
FIELD_VM_HANDLER(func_80098274)
FIELD_VM_HANDLER(func_800977A4)
FIELD_VM_HANDLER(func_800976A8)
FIELD_VM_HANDLER(func_800980FC)
FIELD_VM_HANDLER(func_80098038)
FIELD_VM_HANDLER(func_800975C0)
FIELD_VM_HANDLER(func_8009749C)
FIELD_VM_HANDLER(func_80093014)
FIELD_VM_HANDLER(func_80099214)
FIELD_VM_HANDLER(func_80094918)
FIELD_VM_HANDLER(func_8009F4CC)
FIELD_VM_HANDLER(func_8009524C)
FIELD_VM_HANDLER(func_80095284)
FIELD_VM_HANDLER(func_800A0228)
FIELD_VM_HANDLER(func_8009A174)
FIELD_VM_HANDLER(func_8009A1AC)
FIELD_VM_HANDLER(func_8009AD6C)
FIELD_VM_HANDLER(FieldScriptResetCameraTargetMovement)
FIELD_VM_HANDLER(FieldScriptSetCameraTargetMovementFrom)
FIELD_VM_HANDLER(FieldScriptSetCameraTargetMovementDestToActor)
FIELD_VM_HANDLER(FieldScriptSetCameraTargetMovementDest)
FIELD_VM_HANDLER(FieldScriptResetCameraPosMovement)
FIELD_VM_HANDLER(FieldScriptSetCameraPosMovementFrom)
FIELD_VM_HANDLER(FieldScriptSetCameraPosMovementDestToActor)
FIELD_VM_HANDLER(FieldScriptSetActorDirection)
FIELD_VM_HANDLER(func_8009AC34)
FIELD_VM_HANDLER(FieldScriptVMHandlerSetCurActorRotation)
FIELD_VM_HANDLER(func_8009ACB4)
FIELD_VM_HANDLER(FieldScriptRotateActorClockwise)
FIELD_VM_HANDLER(FieldScriptRotateActorCounterClockwise)
FIELD_VM_HANDLER(FieldScriptCos)
FIELD_VM_HANDLER(FieldScriptSin)
FIELD_VM_HANDLER(func_8009A2A8)
FIELD_VM_HANDLER(func_8009A1E4)
FIELD_VM_HANDLER(func_80093568)
FIELD_VM_HANDLER(func_8008F724)
FIELD_VM_HANDLER(func_80086C34)
FIELD_VM_HANDLER(func_8008F668)
FIELD_VM_HANDLER(func_8008F76C)
FIELD_VM_HANDLER(func_80093A68)
FIELD_VM_HANDLER(func_80093A98)
FIELD_VM_HANDLER(func_800973A4)
FIELD_VM_HANDLER(FieldScriptVMHandlerRestoreHp)
FIELD_VM_HANDLER(FieldScriptVMHandlerRestoreMp)
FIELD_VM_HANDLER(FieldScriptVMHandlerDecreasePartyHp)
FIELD_VM_HANDLER(FieldScriptVMHandlerIncreasePartyMp)
FIELD_VM_HANDLER(FieldScriptVMHandlerDecreasePartyMp)
FIELD_VM_HANDLER(func_80097108)
FIELD_VM_HANDLER(func_80095300)
FIELD_VM_HANDLER(func_80092664)
FIELD_VM_HANDLER(func_800926C8)
FIELD_VM_HANDLER(func_80093664)
FIELD_VM_HANDLER(func_80092768)
FIELD_VM_HANDLER(FieldScriptCheckScenarioFlagsLessThan)
FIELD_VM_HANDLER(FieldScriptCheckScenarioFlagsGreaterThan)
FIELD_VM_HANDLER(FieldScriptCheckScenarioFlagsEqual)
FIELD_VM_HANDLER(FieldScriptSetScenarioFlags)
FIELD_VM_HANDLER(FieldScriptGetScenarioFlags)
FIELD_VM_HANDLER(FieldScriptCheckActorDistance)
FIELD_VM_HANDLER(FieldScriptCheckActorOnScreen)
FIELD_VM_HANDLER(func_800962C0)
FIELD_VM_HANDLER(func_8009631C)
FIELD_VM_HANDLER(func_8009640C)
FIELD_VM_HANDLER(FieldScriptCheckGoldAmount)
FIELD_VM_HANDLER(FieldScriptIncreaseGold)
FIELD_VM_HANDLER(FieldScriptDecreaseGold)
FIELD_VM_HANDLER(FieldScriptCheckPartyMember)
FIELD_VM_HANDLER(func_800A19B0)
FIELD_VM_HANDLER(func_800A1364)
FIELD_VM_HANDLER(func_800945D4)
FIELD_VM_HANDLER(func_80094650)
FIELD_VM_HANDLER(func_8009468C)
FIELD_VM_HANDLER(FieldScriptSetDollySet)
FIELD_VM_HANDLER(func_800932D0)
FIELD_VM_HANDLER(func_8008FB98)
FIELD_VM_HANDLER(func_8008FC4C)
FIELD_VM_HANDLER(FieldScriptSetCameraInterpolationStep)
FIELD_VM_HANDLER(func_8009BB0C)
FIELD_VM_HANDLER(func_8009A34C)
FIELD_VM_HANDLER(func_8009B9A0)
FIELD_VM_HANDLER(func_8009BA0C)
FIELD_VM_HANDLER(func_8009BA7C)
FIELD_VM_HANDLER(FieldScriptSetDollyStop)
FIELD_VM_HANDLER(func_8009A58C)
FIELD_VM_HANDLER(FieldScriptSetCameraPosMovementDest)
FIELD_VM_HANDLER(func_8009A490)
FIELD_VM_HANDLER(FieldScriptWriteCameraDirection)
FIELD_VM_HANDLER(func_80097410)
FIELD_VM_HANDLER(func_8009F5F4)
FIELD_VM_HANDLER(FieldScriptVMHandlerMulVariableWithRand)
FIELD_VM_HANDLER(func_8009BC98)
FIELD_VM_HANDLER(func_8009ACEC)
FIELD_VM_HANDLER(FieldScriptResetCameraMovements)
FIELD_VM_HANDLER(FieldScriptStartCameraMovement)
FIELD_VM_HANDLER(FieldScriptWriteCameraTweenTarget)
FIELD_VM_HANDLER(FieldScriptWriteCameraTweenPosition)
FIELD_VM_HANDLER(func_80090C20)
FIELD_VM_HANDLER(func_80090CB8)
FIELD_VM_HANDLER(func_80090D50)
FIELD_VM_HANDLER(func_8009A5E0)
FIELD_VM_HANDLER(FieldScriptFadeOut)
FIELD_VM_HANDLER(FieldScriptFadeIn)
FIELD_VM_HANDLER(func_8009B8E4)
FIELD_VM_HANDLER(func_8009B6AC)
FIELD_VM_HANDLER(func_8009ADDC)
FIELD_VM_HANDLER(func_8009AE0C)
FIELD_VM_HANDLER(func_80096534)
FIELD_VM_HANDLER(func_800965A8)
FIELD_VM_HANDLER(func_800965F4)
FIELD_VM_HANDLER(func_800A0D3C)
FIELD_VM_HANDLER(func_80094A5C)
FIELD_VM_HANDLER(func_80094ACC)
FIELD_VM_HANDLER(func_80094B3C)
FIELD_VM_HANDLER(func_80094BAC)
FIELD_VM_HANDLER(func_80094C1C)
FIELD_VM_HANDLER(func_80094C8C)
FIELD_VM_HANDLER(func_800972F4)
FIELD_VM_HANDLER(func_80093E30)
FIELD_VM_HANDLER(func_80093FC0)
FIELD_VM_HANDLER(func_800A1E9C)
FIELD_VM_HANDLER(func_8009B824)
FIELD_VM_HANDLER(func_8009B884)
FIELD_VM_HANDLER(FieldScriptCheckTriggerZone2D)
FIELD_VM_HANDLER(FieldScriptAtan2)
FIELD_VM_HANDLER(FieldScriptCheckTriggerZone)
FIELD_VM_HANDLER(FieldScriptHandleTriggerZone)
FIELD_VM_HANDLER(func_8009DA70)
FIELD_VM_HANDLER(func_8009DA98)
FIELD_VM_HANDLER(func_8009CE48)
FIELD_VM_HANDLER(func_8009CEE0)
FIELD_VM_HANDLER(func_8009CF70)
FIELD_VM_HANDLER(func_8009C0B4)
FIELD_VM_HANDLER(func_8009C0DC)
FIELD_VM_HANDLER(func_8009C01C)
FIELD_VM_HANDLER(FieldScriptVMHandlerSetControllerBtnMask)
FIELD_VM_HANDLER(func_800925A0)
FIELD_VM_HANDLER(func_800946BC)
FIELD_VM_HANDLER(func_80094710)
FIELD_VM_HANDLER(func_80094764)
FIELD_VM_HANDLER(func_800921E8)
FIELD_VM_HANDLER(func_80091F84)
FIELD_VM_HANDLER(func_80092044)
FIELD_VM_HANDLER(func_80091E00)
FIELD_VM_HANDLER(FieldScriptVMHandlerVariableMul)
FIELD_VM_HANDLER(FieldScriptVMHandlerVariableDiv)
FIELD_VM_HANDLER(func_80091E98)
FIELD_VM_HANDLER(func_80091BBC)
FIELD_VM_HANDLER(func_80096150)
FIELD_VM_HANDLER(func_80096178)
FIELD_VM_HANDLER(func_80091AD4)
FIELD_VM_HANDLER(func_80091944)
FIELD_VM_HANDLER(func_80091A08)
FIELD_VM_HANDLER(func_80091A78)
FIELD_VM_HANDLER(func_80094158)
FIELD_VM_HANDLER(func_800943AC)
FIELD_VM_HANDLER(func_80092DFC)
FIELD_VM_HANDLER(func_800910C0)
FIELD_VM_HANDLER(func_80091318)
FIELD_VM_HANDLER(FieldScriptWriteCameraMovementParameter)
FIELD_VM_HANDLER(FieldScriptSetCameraMovementParameter)
FIELD_VM_HANDLER(FieldScriptWaitForCameraMovement)
FIELD_VM_HANDLER(func_80090DEC)
FIELD_VM_HANDLER(func_8008B248)
FIELD_VM_HANDLER(func_8008F90C)
FIELD_VM_HANDLER(func_80090E70)
FIELD_VM_HANDLER(func_8009BE9C)
FIELD_VM_HANDLER(func_8009C12C)
FIELD_VM_HANDLER(func_8008E8C8)
FIELD_VM_HANDLER(func_8008E85C)
FIELD_VM_HANDLER(func_8008E59C)
FIELD_VM_HANDLER(FieldScriptSetParentActor)
FIELD_VM_HANDLER(func_800947B0)
FIELD_VM_HANDLER(func_8008D780)
FIELD_VM_HANDLER(func_8009BF8C)
FIELD_VM_HANDLER(FieldScriptVMHandlerNop)
FIELD_VM_HANDLER(FieldScriptVM2Run)
FIELD_VM_HANDLER(FieldScriptVMHandlerNop)

asm(".balign 8\n"
    ".globl g_FieldScriptVMHandlers2\n"
    "g_FieldScriptVMHandlers2:\n");

FIELD_VM_HANDLER(func_8008D2D8)
FIELD_VM_HANDLER(func_8009F424)
FIELD_VM_HANDLER(func_80095B3C)
FIELD_VM_HANDLER(func_8008D0F4)
FIELD_VM_HANDLER(func_8008D26C)
FIELD_VM_HANDLER(func_80095CC4)
FIELD_VM_HANDLER(func_80095D6C)
FIELD_VM_HANDLER(func_8008D604)
FIELD_VM_HANDLER(func_8008D180)
FIELD_VM_HANDLER(func_8008D078)
FIELD_VM_HANDLER(func_8008D684)
FIELD_VM_HANDLER(func_8008D700)
FIELD_VM_HANDLER(func_8008CFEC)
FIELD_VM_HANDLER(func_8008CF9C)
FIELD_VM_HANDLER(func_8008C84C)
FIELD_VM_HANDLER(func_8008C938)
FIELD_VM_HANDLER(func_8008CA60)
FIELD_VM_HANDLER(func_8008CB4C)
FIELD_VM_HANDLER(func_8008CC74)
FIELD_VM_HANDLER(func_8008CD48)
FIELD_VM_HANDLER(func_8008CDD4)
FIELD_VM_HANDLER(func_800A14F0)
FIELD_VM_HANDLER(func_8008C7D8)
FIELD_VM_HANDLER(func_8009AA00)
FIELD_VM_HANDLER(func_8008BDD8)
FIELD_VM_HANDLER(func_8008C334)
FIELD_VM_HANDLER(func_8008B894)
FIELD_VM_HANDLER(func_8008B5D4)
FIELD_VM_HANDLER(func_80098A7C)
FIELD_VM_HANDLER(func_800984EC)
FIELD_VM_HANDLER(func_8009FB98)
FIELD_VM_HANDLER(func_8009FDD4)
FIELD_VM_HANDLER(func_8009FE4C)
FIELD_VM_HANDLER(func_800A06E8)
FIELD_VM_HANDLER(func_8009B664)
FIELD_VM_HANDLER(func_8009B398)
FIELD_VM_HANDLER(func_8009B210)
FIELD_VM_HANDLER(func_8008D5C8)
FIELD_VM_HANDLER(func_8008B2F0)
FIELD_VM_HANDLER(func_8008B328)
FIELD_VM_HANDLER(func_8008E4EC)
FIELD_VM_HANDLER(func_8008E518)
FIELD_VM_HANDLER(func_8008E544)
FIELD_VM_HANDLER(func_8008E570)
FIELD_VM_HANDLER(FieldScriptWriteActorFlags1)
FIELD_VM_HANDLER(FieldScriptWriteActorFlags2)
FIELD_VM_HANDLER(FieldScriptWriteActorFlags3)
FIELD_VM_HANDLER(FieldScriptWriteActorFlags4)
FIELD_VM_HANDLER(func_8008E3E8)
FIELD_VM_HANDLER(func_8008E414)
FIELD_VM_HANDLER(func_8008E440)
FIELD_VM_HANDLER(func_8008E46C)
FIELD_VM_HANDLER(func_8008E298)
FIELD_VM_HANDLER(func_8008E2EC)
FIELD_VM_HANDLER(func_8008E340)
FIELD_VM_HANDLER(func_8008E394)
FIELD_VM_HANDLER(FieldScriptWriteActorDistance)
FIELD_VM_HANDLER(func_8008D230)
FIELD_VM_HANDLER(func_8008CED0)
FIELD_VM_HANDLER(func_8008CE64)
FIELD_VM_HANDLER(func_8008B180)
FIELD_VM_HANDLER(func_8008AEC8)
FIELD_VM_HANDLER(func_8008AFD8)
FIELD_VM_HANDLER(func_8008B0E8)
FIELD_VM_HANDLER(func_80092148)
FIELD_VM_HANDLER(FieldScriptPartyMemberRideGear)
FIELD_VM_HANDLER(FieldScriptPartyMemberDisembarkGear)
FIELD_VM_HANDLER(func_8009B15C)
FIELD_VM_HANDLER(func_8009B184)
FIELD_VM_HANDLER(func_8009A0FC)
FIELD_VM_HANDLER(func_8008AE5C)
FIELD_VM_HANDLER(func_8008B144)
FIELD_VM_HANDLER(func_8008B518)
FIELD_VM_HANDLER(func_8008DAFC)
FIELD_VM_HANDLER(func_8008ACE8)
FIELD_VM_HANDLER(func_8008A9AC)
FIELD_VM_HANDLER(func_8008A974)
FIELD_VM_HANDLER(func_8008A93C)
FIELD_VM_HANDLER(func_8008AA60)
FIELD_VM_HANDLER(func_80093BB0)
FIELD_VM_HANDLER(func_80093BD4)
FIELD_VM_HANDLER(FieldScriptEnableCompass)
FIELD_VM_HANDLER(FieldScriptDisableCompass)
FIELD_VM_HANDLER(func_80093AC8)
FIELD_VM_HANDLER(func_80093B10)
FIELD_VM_HANDLER(func_80093740)
FIELD_VM_HANDLER(func_80093930)
FIELD_VM_HANDLER(func_800937E0)
FIELD_VM_HANDLER(func_80093824)
FIELD_VM_HANDLER(func_800939A0)
FIELD_VM_HANDLER(func_80093A04)
FIELD_VM_HANDLER(func_8008B210)
FIELD_VM_HANDLER(func_800A0FD8)
FIELD_VM_HANDLER(func_8008F6AC)
FIELD_VM_HANDLER(func_8008F2D8)
FIELD_VM_HANDLER(func_8008F1C8)
FIELD_VM_HANDLER(func_8008EC30)
FIELD_VM_HANDLER(func_8008E9F8)
FIELD_VM_HANDLER(func_8008F444)
FIELD_VM_HANDLER(func_8008F4A0)
FIELD_VM_HANDLER(func_8008F5E4)
FIELD_VM_HANDLER(func_8008F4FC)
FIELD_VM_HANDLER(func_8008F558)
FIELD_VM_HANDLER(func_8008EE14)
FIELD_VM_HANDLER(func_80092C20)
FIELD_VM_HANDLER(func_8008A6E0)
FIELD_VM_HANDLER(func_8008A604)
FIELD_VM_HANDLER(func_8008A640)
FIELD_VM_HANDLER(func_8008A5A0)
FIELD_VM_HANDLER(func_8008FB28)
FIELD_VM_HANDLER(func_8008FABC)
FIELD_VM_HANDLER(func_8008B45C)
FIELD_VM_HANDLER(func_80089F54)
FIELD_VM_HANDLER(func_8009899C)
FIELD_VM_HANDLER(func_800988B8)
FIELD_VM_HANDLER(func_8009861C)
FIELD_VM_HANDLER(func_800985BC)
FIELD_VM_HANDLER(func_800989F0)
FIELD_VM_HANDLER(func_80098738)
FIELD_VM_HANDLER(func_8008A2E8)
FIELD_VM_HANDLER(func_8008A4F0)
FIELD_VM_HANDLER(func_8008A4E8)
FIELD_VM_HANDLER(func_8008A4E0)
FIELD_VM_HANDLER(func_8008A518)
FIELD_VM_HANDLER(func_8008A500)
FIELD_VM_HANDLER(func_8008A508)
FIELD_VM_HANDLER(func_8008A510)
FIELD_VM_HANDLER(func_8008A244)
FIELD_VM_HANDLER(func_80089FD0)
FIELD_VM_HANDLER(func_8008A08C)
FIELD_VM_HANDLER(func_8008A148)
FIELD_VM_HANDLER(func_80092FB4)
FIELD_VM_HANDLER(func_800933F8)
FIELD_VM_HANDLER(func_8008A2A0)
FIELD_VM_HANDLER(func_80089F94)
FIELD_VM_HANDLER(func_800936E4)
FIELD_VM_HANDLER(func_80089BF0)
FIELD_VM_HANDLER(func_80089DCC)
FIELD_VM_HANDLER(func_80089F18)
FIELD_VM_HANDLER(func_80089B54)
FIELD_VM_HANDLER(func_8008F3D0)
FIELD_VM_HANDLER(func_8008F394)
FIELD_VM_HANDLER(func_8008F348)
FIELD_VM_HANDLER(func_80088790)
FIELD_VM_HANDLER(FieldScriptInitializeParticleBank)
FIELD_VM_HANDLER(FieldScriptSetParticleBankPosition)
FIELD_VM_HANDLER(FieldScriptSetParticleBankPhysics)
FIELD_VM_HANDLER(FieldScriptSetParticleBankParameters)
FIELD_VM_HANDLER(FieldScriptSetParticleBankScale)
FIELD_VM_HANDLER(FieldScriptSetParticleBankColor)
FIELD_VM_HANDLER(FieldScriptParticlesInitialize)
FIELD_VM_HANDLER(FieldScriptStopParticleActor)
FIELD_VM_HANDLER(func_800884CC)
FIELD_VM_HANDLER(func_8008848C)
FIELD_VM_HANDLER(func_8008F0B4)
FIELD_VM_HANDLER(func_8008EF5C)
FIELD_VM_HANDLER(func_8008EFA0)
FIELD_VM_HANDLER(func_8008F070)
FIELD_VM_HANDLER(func_8008EFE4)
FIELD_VM_HANDLER(func_800883D4)
FIELD_VM_HANDLER(func_8008EA58)
FIELD_VM_HANDLER(FieldScriptSetCharacterGear)
FIELD_VM_HANDLER(func_8008825C)
FIELD_VM_HANDLER(func_800881E8)
FIELD_VM_HANDLER(func_80088198)
FIELD_VM_HANDLER(func_80088C1C)
FIELD_VM_HANDLER(func_800888A4)
FIELD_VM_HANDLER(func_800889BC)
FIELD_VM_HANDLER(FieldScriptWriteCurCameraTarget)
FIELD_VM_HANDLER(FieldScriptWriteCurCameraPosition)
FIELD_VM_HANDLER(func_8008DB2C)
FIELD_VM_HANDLER(FieldScriptVMHandlerIncreasePartyGearHp)
FIELD_VM_HANDLER(FieldScriptVMHandlerDecreasePartyGearHp)
FIELD_VM_HANDLER(FieldScriptVMHandlerWritePartyMemberHp)
FIELD_VM_HANDLER(func_80096AF4)
FIELD_VM_HANDLER(func_8008800C)
FIELD_VM_HANDLER(func_8008AACC)
FIELD_VM_HANDLER(func_80087FD4)
FIELD_VM_HANDLER(FieldScriptVMHandlerSetPartyMemberHp)
FIELD_VM_HANDLER(FieldScriptVMHandlerSetPartyMemberMp)
FIELD_VM_HANDLER(FieldScriptVMHandlerWritePartyMemberMp)
FIELD_VM_HANDLER(func_80087FA4)
FIELD_VM_HANDLER(func_80087E98)
FIELD_VM_HANDLER(func_80087E5C)
FIELD_VM_HANDLER(func_80087DE0)
FIELD_VM_HANDLER(func_80087B5C)
FIELD_VM_HANDLER(func_80087C34)
FIELD_VM_HANDLER(func_80087D30)
FIELD_VM_HANDLER(func_80087D80)
FIELD_VM_HANDLER(func_80088B68)
FIELD_VM_HANDLER(func_80087C0C)
FIELD_VM_HANDLER(func_80087848)
FIELD_VM_HANDLER(func_80087800)
FIELD_VM_HANDLER(func_80088508)
FIELD_VM_HANDLER(func_80088674)
FIELD_VM_HANDLER(func_8009E014)
FIELD_VM_HANDLER(func_8009DF78)
FIELD_VM_HANDLER(func_80086F7C)
FIELD_VM_HANDLER(func_8008BC80)
FIELD_VM_HANDLER(func_800882B8)
FIELD_VM_HANDLER(func_80088CF8)
FIELD_VM_HANDLER(func_80088D18)
FIELD_VM_HANDLER(func_800A0EE8)
FIELD_VM_HANDLER(func_800A0EB0)
FIELD_VM_HANDLER(func_800A0E54)
FIELD_VM_HANDLER(func_800A0DFC)
FIELD_VM_HANDLER(func_800A0DC0)
FIELD_VM_HANDLER(func_80093888)
FIELD_VM_HANDLER(func_8008764C)
FIELD_VM_HANDLER(func_8008754C)
FIELD_VM_HANDLER(func_8008752C)
FIELD_VM_HANDLER(func_80087420)
FIELD_VM_HANDLER(func_80086FD0)
FIELD_VM_HANDLER(func_80087960)
FIELD_VM_HANDLER(func_800879D0)
FIELD_VM_HANDLER(func_80087AB8)
FIELD_VM_HANDLER(func_80087A40)
FIELD_VM_HANDLER(func_80087A7C)
FIELD_VM_HANDLER(func_80093790)
FIELD_VM_HANDLER(FieldScriptVMHandlerRestoreCharacterHpAndMp)
FIELD_VM_HANDLER(func_800873C4)
FIELD_VM_HANDLER(func_800871B0)
FIELD_VM_HANDLER(func_80087148)
FIELD_VM_HANDLER(func_80086E1C)
FIELD_VM_HANDLER(func_80086DE0)
FIELD_VM_HANDLER(FieldScriptCopyGear)
FIELD_VM_HANDLER(func_80086D4C)
#undef FIELD_VM_HANDLER

ZoomFadeEffect g_FieldZoomFadeEffect;
s32 D_800ADB98;
s32 D_800ADC0C;
s32 D_800B14A4;
s32 g_FieldPixelIndex;
u_char g_FieldParticleStatuses[NUM_PARTICLES];
short g_FieldParticleActorIDs[NUM_PARTICLES];
ParticleBank g_FieldDefaultParticleBanks[NUM_PARTICLE_BANKS];
ParticleBank* g_FieldParticleBanks[NUM_PARTICLES];
s32 g_FieldParticleBankIndex;
s32 g_FieldParticleCurActor;
s32 D_800AF278;
s32 D_800AF858;

/* FE60 transition state (retail 0x800AF76C..0x800AF858).
 *
 * The retail pointer slots are four bytes apart, but native pointers are eight
 * bytes wide.  Keep those slots as independent host objects so writes cannot
 * overlap, while preserving the genuinely contiguous 0xD0-byte GPU packet
 * span beginning at D_800AF788.  AC3AC/AC99C depend on the packet offsets.
 */
void* D_800AF76C;
void* D_800AF770;
void* D_800AF774;
s32 D_800AF778;
s32 D_800AF77C;
s32 D_800AF780;
void* D_800AF784;
unsigned char g_FieldTransitionPackets[0xD0] __attribute__((aligned(8)));

/*
 * Field overlay BSS work areas.
 *
 * Several decompiled field routines address these as independent symbols while
 * the original MIPS code also walks across the gaps as contiguous storage. Keep
 * the native port layout faithful by backing the known labels with shared blocks
 * instead of letting the auto-stubber allocate each label separately.
 */
unsigned char g_FieldBss_800B20A8[0x424] __attribute__((aligned(8)));
unsigned char g_FieldBss_800AFC60[0xC0] __attribute__((aligned(8)));
unsigned char g_FieldBss_800AEB60[0x08] __attribute__((aligned(8)));
unsigned char g_FieldBss_800AFE9C[0x110] __attribute__((aligned(8)));
unsigned char g_FieldBss_800AFB20[0x108] __attribute__((aligned(8)));
unsigned char g_FieldBss_800AF85C[0x2A0] __attribute__((aligned(8)));
/* Retail keeps D_800B004C..D_800B0052 as one packed RECT.  Host-side
 * auto-stubs used to allocate each label separately, so StoreImage/LoadImage
 * read a zero-height rectangle through pointer arithmetic. */
unsigned char g_FieldBss_800B004C[0x08] __attribute__((aligned(8)));
unsigned char g_FieldBss_800B007C[0x40] __attribute__((aligned(8)));
/* Portrait load slots at 0x800B06A4: 3×{faceId,state,dualTim} stride 6. */
unsigned char g_FieldBss_800B06A4[0x18] __attribute__((aligned(8)));
/* Portrait CD stream queue: need 3 host StreamDataQueueEntry (16B on x64). */
unsigned char g_FieldBss_800B00C8[0x40] __attribute__((aligned(8)));
unsigned char g_FieldBss_800B06BC[0x1734] __attribute__((aligned(8)));
unsigned char g_FieldBss_800B1DF0[0x384] __attribute__((aligned(8)));
unsigned char g_FieldBss_800C2688[0x1270] __attribute__((aligned(8)));
unsigned char g_FieldBss_800C38F8[0x974] __attribute__((aligned(8)));

/*
 * Field state save/restore scratch.
 *
 * The field overlay treats D_8005A4E4 as a packed save image used by
 * func_800A3F4C/func_800A3474, with D_800AFC50 as the walking cursor. These
 * are overlay-owned in field.elf even though the addresses sit in the main
 * executable's low BSS range, so the PC port must not fall back to unrelated
 * auto-stub symbols from that address neighborhood.
 */
s32 D_8005A408[3];
unsigned char D_8005A4E4[0x10000] __attribute__((aligned(8)));
unsigned char* D_800AFC50;

#define FIELD_BSS_ALIAS(name, block, offset) \
    asm(".globl " #name "\n.set " #name ", " #block " + " #offset)

FIELD_BSS_ALIAS(g_FieldEffects, g_FieldBss_800B20A8, 0x000);
asm(".globl g_FieldBss_800B2174\n.set g_FieldBss_800B2174, g_FieldBss_800B20A8 + 0x0FC");

FIELD_BSS_ALIAS(D_800AEB60, g_FieldBss_800AEB60, 0x000);
FIELD_BSS_ALIAS(D_800AEB64, g_FieldBss_800AEB60, 0x004);

FIELD_BSS_ALIAS(D_800AF788, g_FieldTransitionPackets, 0x000);
FIELD_BSS_ALIAS(D_800AF7F0, g_FieldTransitionPackets, 0x068);
FIELD_BSS_ALIAS(D_800AF824, g_FieldTransitionPackets, 0x09C);

FIELD_BSS_ALIAS(D_800AFB20, g_FieldBss_800AFB20, 0x000);
FIELD_BSS_ALIAS(D_800AFB24, g_FieldBss_800AFB20, 0x004);
FIELD_BSS_ALIAS(D_800AFB34, g_FieldBss_800AFB20, 0x014);
FIELD_BSS_ALIAS(D_800AFB44, g_FieldBss_800AFB20, 0x024);
FIELD_BSS_ALIAS(D_800AFB54, g_FieldBss_800AFB20, 0x034);
FIELD_BSS_ALIAS(D_800AFC08, g_FieldBss_800AFB20, 0x0E8);

FIELD_BSS_ALIAS(D_800B004C, g_FieldBss_800B004C, 0x000);
FIELD_BSS_ALIAS(D_800B004E, g_FieldBss_800B004C, 0x002);
FIELD_BSS_ALIAS(D_800B0050, g_FieldBss_800B004C, 0x004);
FIELD_BSS_ALIAS(D_800B0052, g_FieldBss_800B004C, 0x006);

FIELD_BSS_ALIAS(D_800AF85C, g_FieldBss_800AF85C, 0x000);
FIELD_BSS_ALIAS(D_800AF87C, g_FieldBss_800AF85C, 0x020);
/* Camera vector block (retail 0x800AF880..0x800AF8EC). Offsets must be
 * psx_addr - 0x800AF85C. The previous values were derived from the wrong
 * base (0x800AF82C), shifting the named vectors +0x30 and colliding
 * g_CameraEye2 with the D_800AF8E0 shake offsets. */
FIELD_BSS_ALIAS(g_CameraEye, g_FieldBss_800AF85C, 0x024);  /* 0x800AF880 */
FIELD_BSS_ALIAS(g_CameraAt, g_FieldBss_800AF85C, 0x034);   /* 0x800AF890 */
FIELD_BSS_ALIAS(g_CameraUp, g_FieldBss_800AF85C, 0x044);   /* 0x800AF8A0 */
FIELD_BSS_ALIAS(g_CameraEye2, g_FieldBss_800AF85C, 0x054); /* 0x800AF8B0 */
FIELD_BSS_ALIAS(g_CameraAt2, g_FieldBss_800AF85C, 0x064);  /* 0x800AF8C0 */
FIELD_BSS_ALIAS(D_800AF8D0, g_FieldBss_800AF85C, 0x074);   /* 0x800AF8D0: second up vector, pairs with eye2/at2 */
FIELD_BSS_ALIAS(D_800AF8E0, g_FieldBss_800AF85C, 0x084);
FIELD_BSS_ALIAS(D_800AF8E4, g_FieldBss_800AF85C, 0x088);
FIELD_BSS_ALIAS(D_800AF8E8, g_FieldBss_800AF85C, 0x08C);
/* Retail saves/restores 0x1C8 bytes starting at g_CameraEye. Keep scripted
 * camera state in that same region: independent generated stubs lose mode
 * and movement state across battle, stranding EF camera waits on return. */
FIELD_BSS_ALIAS(g_CamAtMovementFrom, g_FieldBss_800AF85C, 0x094);
FIELD_BSS_ALIAS(g_CamAtMovementTo, g_FieldBss_800AF85C, 0x0A4);
FIELD_BSS_ALIAS(g_CamEyeMovementFrom, g_FieldBss_800AF85C, 0x0B4);
FIELD_BSS_ALIAS(g_CamEyeMovementTo, g_FieldBss_800AF85C, 0x0C4);
FIELD_BSS_ALIAS(D_800AF930, g_FieldBss_800AF85C, 0x0D4);
FIELD_BSS_ALIAS(g_FieldCameraMode, g_FieldBss_800AF85C, 0x0D8);
FIELD_BSS_ALIAS(D_800AF936, g_FieldBss_800AF85C, 0x0DA);
FIELD_BSS_ALIAS(D_800AF938, g_FieldBss_800AF85C, 0x0DC);
FIELD_BSS_ALIAS(D_800AF93A, g_FieldBss_800AF85C, 0x0DE);
FIELD_BSS_ALIAS(g_CamMovementFlags, g_FieldBss_800AF85C, 0x0E0);
FIELD_BSS_ALIAS(g_CamAtMovementDuration, g_FieldBss_800AF85C, 0x0E2);
FIELD_BSS_ALIAS(g_CamAtMovementCurrent, g_FieldBss_800AF85C, 0x0E4);
FIELD_BSS_ALIAS(g_CamAtMovementDelta, g_FieldBss_800AF85C, 0x0F4);
FIELD_BSS_ALIAS(g_CamEyeMovementDuration, g_FieldBss_800AF85C, 0x104);
FIELD_BSS_ALIAS(g_CamEyeMovementCurrent, g_FieldBss_800AF85C, 0x108);
FIELD_BSS_ALIAS(g_CamEyeMovementDelta, g_FieldBss_800AF85C, 0x118);
FIELD_BSS_ALIAS(g_CamInterpolation, g_FieldBss_800AF85C, 0x128);
FIELD_BSS_ALIAS(g_Scene, g_FieldBss_800AF85C, 0x134); /* 0x800AF990; worldToScreen at +0xD4 == g_CameraEye+0x1E4 */
FIELD_BSS_ALIAS(g_WorldScale, g_FieldBss_800AF85C, 0x268); /* 0x800AFAC4; was 0x298, same wrong-base (0x800AF82C) error as the camera vectors */

FIELD_BSS_ALIAS(D_800AFC60, g_FieldBss_800AFC60, 0x000);
FIELD_BSS_ALIAS(D_800AFC64, g_FieldBss_800AFC60, 0x004);
FIELD_BSS_ALIAS(D_800AFC68, g_FieldBss_800AFC60, 0x008);
FIELD_BSS_ALIAS(D_800AFC6C, g_FieldBss_800AFC60, 0x00C);
FIELD_BSS_ALIAS(D_800AFC70, g_FieldBss_800AFC60, 0x010);
FIELD_BSS_ALIAS(D_800AFC74, g_FieldBss_800AFC60, 0x014);
FIELD_BSS_ALIAS(D_800AFC78, g_FieldBss_800AFC60, 0x018);
FIELD_BSS_ALIAS(g_FieldScriptMaxInstructionCount, g_FieldBss_800AFC60, 0x01C);
FIELD_BSS_ALIAS(D_800AFC80, g_FieldBss_800AFC60, 0x020);
FIELD_BSS_ALIAS(D_800AFC84, g_FieldBss_800AFC60, 0x024);
FIELD_BSS_ALIAS(D_800AFD04, g_FieldBss_800AFC60, 0x0A4);
FIELD_BSS_ALIAS(D_800AFD08, g_FieldBss_800AFC60, 0x0A8);
FIELD_BSS_ALIAS(D_800AFD0C, g_FieldBss_800AFC60, 0x0AC);
FIELD_BSS_ALIAS(D_800AFD10, g_FieldBss_800AFC60, 0x0B0);
FIELD_BSS_ALIAS(D_800AFD14, g_FieldBss_800AFC60, 0x0B4);
FIELD_BSS_ALIAS(D_800AFD18, g_FieldBss_800AFC60, 0x0B8);
FIELD_BSS_ALIAS(D_800AFD1C, g_FieldBss_800AFC60, 0x0BC);

FIELD_BSS_ALIAS(D_800AFE9C, g_FieldBss_800AFE9C, 0x000);
FIELD_BSS_ALIAS(D_800AFEA0, g_FieldBss_800AFE9C, 0x004);
FIELD_BSS_ALIAS(D_800AFEA4, g_FieldBss_800AFE9C, 0x008);
FIELD_BSS_ALIAS(D_800AFEA8, g_FieldBss_800AFE9C, 0x00C);

FIELD_BSS_ALIAS(D_800B007C, g_FieldBss_800B007C, 0x000);
FIELD_BSS_ALIAS(D_800B0080, g_FieldBss_800B007C, 0x004);
FIELD_BSS_ALIAS(D_800B0082, g_FieldBss_800B007C, 0x006);
FIELD_BSS_ALIAS(D_800B0084, g_FieldBss_800B007C, 0x008);
FIELD_BSS_ALIAS(D_800B0086, g_FieldBss_800B007C, 0x00A);
FIELD_BSS_ALIAS(D_800B0088, g_FieldBss_800B007C, 0x00C);
FIELD_BSS_ALIAS(D_800B008A, g_FieldBss_800B007C, 0x00E);
FIELD_BSS_ALIAS(D_800B008C, g_FieldBss_800B007C, 0x010);
FIELD_BSS_ALIAS(D_800B008E, g_FieldBss_800B007C, 0x012);
FIELD_BSS_ALIAS(D_800B0090, g_FieldBss_800B007C, 0x014);
FIELD_BSS_ALIAS(D_800B0094, g_FieldBss_800B007C, 0x018);
FIELD_BSS_ALIAS(D_800B0098, g_FieldBss_800B007C, 0x01C);
/* Horizon color / fade block (retail 0x800B00A0..0x800B00B2). Must stay
 * contiguous: func_8002709C walks RGB triplets at +0/+4/+8 from D_800B00A0. */
FIELD_BSS_ALIAS(D_800B00A0, g_FieldBss_800B007C, 0x024);
FIELD_BSS_ALIAS(D_800B00A1, g_FieldBss_800B007C, 0x025);
FIELD_BSS_ALIAS(D_800B00A2, g_FieldBss_800B007C, 0x026);
FIELD_BSS_ALIAS(D_800B00A4, g_FieldBss_800B007C, 0x028);
FIELD_BSS_ALIAS(D_800B00A5, g_FieldBss_800B007C, 0x029);
FIELD_BSS_ALIAS(D_800B00A6, g_FieldBss_800B007C, 0x02A);
FIELD_BSS_ALIAS(D_800B00A8, g_FieldBss_800B007C, 0x02C);
FIELD_BSS_ALIAS(D_800B00A9, g_FieldBss_800B007C, 0x02D);
FIELD_BSS_ALIAS(D_800B00AA, g_FieldBss_800B007C, 0x02E);
FIELD_BSS_ALIAS(D_800B00AC, g_FieldBss_800B007C, 0x030);
FIELD_BSS_ALIAS(D_800B00AE, g_FieldBss_800B007C, 0x032);
FIELD_BSS_ALIAS(D_800B00B0, g_FieldBss_800B007C, 0x034);
FIELD_BSS_ALIAS(D_800B00B2, g_FieldBss_800B007C, 0x036);

FIELD_BSS_ALIAS(D_800B06A4, g_FieldBss_800B06A4, 0x000);
FIELD_BSS_ALIAS(D_800B06A6, g_FieldBss_800B06A4, 0x002);
FIELD_BSS_ALIAS(D_800B06A8, g_FieldBss_800B06A4, 0x004);

FIELD_BSS_ALIAS(D_800B00C8, g_FieldBss_800B00C8, 0x000);
FIELD_BSS_ALIAS(D_800B00CC, g_FieldBss_800B00C8, 0x004);
FIELD_BSS_ALIAS(D_800B00D0, g_FieldBss_800B00C8, 0x008);
FIELD_BSS_ALIAS(D_800B00D4, g_FieldBss_800B00C8, 0x00C);

FIELD_BSS_ALIAS(D_800B06BC, g_FieldBss_800B06BC, 0x000);
FIELD_BSS_ALIAS(D_800B0DBC, g_FieldBss_800B06BC, 0x700);
FIELD_BSS_ALIAS(D_800B0F7C, g_FieldBss_800B06BC, 0x8C0);
FIELD_BSS_ALIAS(D_800B0FEC, g_FieldBss_800B06BC, 0x930);
FIELD_BSS_ALIAS(D_800B14AC, g_FieldBss_800B06BC, 0xDF0);
FIELD_BSS_ALIAS(D_800B14F0, g_FieldBss_800B06BC, 0xE34);
FIELD_BSS_ALIAS(D_800B14F4, g_FieldBss_800B06BC, 0xE38);
FIELD_BSS_ALIAS(D_800B14F8, g_FieldBss_800B06BC, 0xE3C);
FIELD_BSS_ALIAS(D_800B14FA, g_FieldBss_800B06BC, 0xE3E);
FIELD_BSS_ALIAS(D_800B14FC, g_FieldBss_800B06BC, 0xE40);
FIELD_BSS_ALIAS(D_800B1500, g_FieldBss_800B06BC, 0xE44);
FIELD_BSS_ALIAS(D_800B1502, g_FieldBss_800B06BC, 0xE46);
FIELD_BSS_ALIAS(D_800B1504, g_FieldBss_800B06BC, 0xE48);
FIELD_BSS_ALIAS(D_800B1506, g_FieldBss_800B06BC, 0xE4A);
FIELD_BSS_ALIAS(D_800B1510, g_FieldBss_800B06BC, 0xE54);
FIELD_BSS_ALIAS(D_800B1514, g_FieldBss_800B06BC, 0xE58);
FIELD_BSS_ALIAS(D_800B1518, g_FieldBss_800B06BC, 0xE5C);
FIELD_BSS_ALIAS(D_800B1520, g_FieldBss_800B06BC, 0xE64);
FIELD_BSS_ALIAS(D_800B1530, g_FieldBss_800B06BC, 0xE74);
FIELD_BSS_ALIAS(D_800B1534, g_FieldBss_800B06BC, 0xE78);

FIELD_BSS_ALIAS(D_800B1DF0, g_FieldBss_800B1DF0, 0x000);
FIELD_BSS_ALIAS(D_800B1DF4, g_FieldBss_800B1DF0, 0x004);
FIELD_BSS_ALIAS(D_800B1E00, g_FieldBss_800B1DF0, 0x010);
FIELD_BSS_ALIAS(D_800B1E18, g_FieldBss_800B1DF0, 0x028);
FIELD_BSS_ALIAS(D_800B1E24, g_FieldBss_800B1DF0, 0x034);
FIELD_BSS_ALIAS(D_800B1F74, g_FieldBss_800B1DF0, 0x184);
FIELD_BSS_ALIAS(D_800B1F78, g_FieldBss_800B1DF0, 0x188);
FIELD_BSS_ALIAS(D_800B1F7A, g_FieldBss_800B1DF0, 0x18A);

FIELD_BSS_ALIAS(D_800B2174, g_FieldBss_800B2174, 0x000);
FIELD_BSS_ALIAS(g_FieldControl, g_FieldBss_800B2174, 0x002);
FIELD_BSS_ALIAS(D_800B217C, g_FieldBss_800B2174, 0x008);
FIELD_BSS_ALIAS(D_800B2180, g_FieldBss_800B2174, 0x00C);
FIELD_BSS_ALIAS(D_800B2184, g_FieldBss_800B2174, 0x010);
FIELD_BSS_ALIAS(D_800B2186, g_FieldBss_800B2174, 0x012);
FIELD_BSS_ALIAS(D_800B2188, g_FieldBss_800B2174, 0x014);
FIELD_BSS_ALIAS(D_800B218C, g_FieldBss_800B2174, 0x018);
FIELD_BSS_ALIAS(D_800B218E, g_FieldBss_800B2174, 0x01A);
FIELD_BSS_ALIAS(D_800B2190, g_FieldBss_800B2174, 0x01C);
FIELD_BSS_ALIAS(D_800B2191, g_FieldBss_800B2174, 0x01D);
FIELD_BSS_ALIAS(D_800B2192, g_FieldBss_800B2174, 0x01E);
FIELD_BSS_ALIAS(D_800B2194, g_FieldBss_800B2174, 0x020);
FIELD_BSS_ALIAS(D_800B2195, g_FieldBss_800B2174, 0x021);
FIELD_BSS_ALIAS(D_800B2196, g_FieldBss_800B2174, 0x022);
FIELD_BSS_ALIAS(D_800B2198, g_FieldBss_800B2174, 0x024);
FIELD_BSS_ALIAS(D_800B219A, g_FieldBss_800B2174, 0x026);
FIELD_BSS_ALIAS(D_800B219C, g_FieldBss_800B2174, 0x028);
FIELD_BSS_ALIAS(D_800B219D, g_FieldBss_800B2174, 0x029);
FIELD_BSS_ALIAS(D_800B219E, g_FieldBss_800B2174, 0x02A);
FIELD_BSS_ALIAS(D_800B219F, g_FieldBss_800B2174, 0x02B);
FIELD_BSS_ALIAS(D_800B21A0, g_FieldBss_800B2174, 0x02C);
FIELD_BSS_ALIAS(D_800B21A2, g_FieldBss_800B2174, 0x02E);
FIELD_BSS_ALIAS(D_800B21A4, g_FieldBss_800B2174, 0x030);
FIELD_BSS_ALIAS(D_800B21A6, g_FieldBss_800B2174, 0x032);
FIELD_BSS_ALIAS(D_800B21A8, g_FieldBss_800B2174, 0x034);
FIELD_BSS_ALIAS(D_800B21AA, g_FieldBss_800B2174, 0x036);
FIELD_BSS_ALIAS(D_800B21AC, g_FieldBss_800B2174, 0x038);
FIELD_BSS_ALIAS(D_800B21AE, g_FieldBss_800B2174, 0x03A);
FIELD_BSS_ALIAS(D_800B21B0, g_FieldBss_800B2174, 0x03C);
FIELD_BSS_ALIAS(D_800B21B2, g_FieldBss_800B2174, 0x03E);
FIELD_BSS_ALIAS(D_800B21B4, g_FieldBss_800B2174, 0x040);
FIELD_BSS_ALIAS(D_800B21B8, g_FieldBss_800B2174, 0x044);
FIELD_BSS_ALIAS(D_800B21BC, g_FieldBss_800B2174, 0x048);
FIELD_BSS_ALIAS(D_800B21C0, g_FieldBss_800B2174, 0x04C);
FIELD_BSS_ALIAS(D_800B21C4, g_FieldBss_800B2174, 0x050);
FIELD_BSS_ALIAS(D_800B21CC, g_FieldBss_800B2174, 0x058);
FIELD_BSS_ALIAS(D_800B21CD, g_FieldBss_800B2174, 0x059);
FIELD_BSS_ALIAS(D_800B21CE, g_FieldBss_800B2174, 0x05A);
FIELD_BSS_ALIAS(D_800B21CF, g_FieldBss_800B2174, 0x05B);
FIELD_BSS_ALIAS(D_800B21D0, g_FieldBss_800B2174, 0x05C);
FIELD_BSS_ALIAS(D_800B21D1, g_FieldBss_800B2174, 0x05D);
FIELD_BSS_ALIAS(D_800B21D2, g_FieldBss_800B2174, 0x05E);
FIELD_BSS_ALIAS(D_800B21D4, g_FieldBss_800B2174, 0x060);
FIELD_BSS_ALIAS(D_800B21D6, g_FieldBss_800B2174, 0x062);
FIELD_BSS_ALIAS(D_800B21D8, g_FieldBss_800B2174, 0x064);
FIELD_BSS_ALIAS(D_800B21DC, g_FieldBss_800B2174, 0x068);
FIELD_BSS_ALIAS(D_800B21E4, g_FieldBss_800B2174, 0x070);
FIELD_BSS_ALIAS(D_800B21EC, g_FieldBss_800B2174, 0x078);
FIELD_BSS_ALIAS(D_800B220C, g_FieldBss_800B2174, 0x098);
FIELD_BSS_ALIAS(D_800B221C, g_FieldBss_800B2174, 0x0A8);
FIELD_BSS_ALIAS(D_800B221E, g_FieldBss_800B2174, 0x0AA);
FIELD_BSS_ALIAS(D_800B2220, g_FieldBss_800B2174, 0x0AC);
FIELD_BSS_ALIAS(D_800B223C, g_FieldBss_800B2174, 0x0C8);
FIELD_BSS_ALIAS(D_800B225C, g_FieldBss_800B2174, 0x0E8);
FIELD_BSS_ALIAS(D_800B225D, g_FieldBss_800B2174, 0x0E9);
FIELD_BSS_ALIAS(D_800B225E, g_FieldBss_800B2174, 0x0EA);
FIELD_BSS_ALIAS(D_800B225F, g_FieldBss_800B2174, 0x0EB);
FIELD_BSS_ALIAS(D_800B2264, g_FieldBss_800B2174, 0x0F0);
FIELD_BSS_ALIAS(D_800B2268, g_FieldBss_800B2174, 0x0F4);
FIELD_BSS_ALIAS(g_PlayerActorIndex, g_FieldBss_800B2174, 0x0F8);
FIELD_BSS_ALIAS(D_800B2270, g_FieldBss_800B2174, 0x0FC);
FIELD_BSS_ALIAS(D_800B2290, g_FieldBss_800B2174, 0x11C);
FIELD_BSS_ALIAS(D_800B2294, g_FieldBss_800B2174, 0x120); /* encounter step counter (was asm-only) */
FIELD_BSS_ALIAS(D_800B2298, g_FieldBss_800B2174, 0x124);
FIELD_BSS_ALIAS(D_800B229C, g_FieldBss_800B2174, 0x128);
FIELD_BSS_ALIAS(D_800B22A0, g_FieldBss_800B2174, 0x12C);
FIELD_BSS_ALIAS(D_800B22DE, g_FieldBss_800B2174, 0x16A);
FIELD_BSS_ALIAS(D_800B22E0, g_FieldBss_800B2174, 0x16C);
FIELD_BSS_ALIAS(D_800B22E2, g_FieldBss_800B2174, 0x16E);
FIELD_BSS_ALIAS(D_800B233C, g_FieldBss_800B2174, 0x1C8);
FIELD_BSS_ALIAS(D_800B233E, g_FieldBss_800B2174, 0x1CA);
FIELD_BSS_ALIAS(D_800B2340, g_FieldBss_800B2174, 0x1CC);
FIELD_BSS_ALIAS(D_800B2342, g_FieldBss_800B2174, 0x1CE);
FIELD_BSS_ALIAS(D_800B2344, g_FieldBss_800B2174, 0x1D0);
FIELD_BSS_ALIAS(D_800B2346, g_FieldBss_800B2174, 0x1D2);
FIELD_BSS_ALIAS(D_800B2348, g_FieldBss_800B2174, 0x1D4);
FIELD_BSS_ALIAS(D_800B234A, g_FieldBss_800B2174, 0x1D6);
FIELD_BSS_ALIAS(D_800B234C, g_FieldBss_800B2174, 0x1D8);
FIELD_BSS_ALIAS(D_800B234E, g_FieldBss_800B2174, 0x1DA);
FIELD_BSS_ALIAS(D_800B2350, g_FieldBss_800B2174, 0x1DC);
FIELD_BSS_ALIAS(D_800B2354, g_FieldBss_800B2174, 0x1E0);
FIELD_BSS_ALIAS(D_800B2355, g_FieldBss_800B2174, 0x1E1);
FIELD_BSS_ALIAS(D_800B2356, g_FieldBss_800B2174, 0x1E2);
FIELD_BSS_ALIAS(D_800B2357, g_FieldBss_800B2174, 0x1E3);
FIELD_BSS_ALIAS(D_800B2358, g_FieldBss_800B2174, 0x1E4);
FIELD_BSS_ALIAS(D_800B235C, g_FieldBss_800B2174, 0x1E8);
FIELD_BSS_ALIAS(D_800B2360, g_FieldBss_800B2174, 0x1EC);
FIELD_BSS_ALIAS(D_800B2364, g_FieldBss_800B2174, 0x1F0);
FIELD_BSS_ALIAS(D_800B2368, g_FieldBss_800B2174, 0x1F4);
FIELD_BSS_ALIAS(D_800B236C, g_FieldBss_800B2174, 0x1F8);
FIELD_BSS_ALIAS(D_800B2374, g_FieldBss_800B2174, 0x200);
FIELD_BSS_ALIAS(D_800B2378, g_FieldBss_800B2174, 0x204);
FIELD_BSS_ALIAS(D_800B237C, g_FieldBss_800B2174, 0x208);
FIELD_BSS_ALIAS(D_800B2380, g_FieldBss_800B2174, 0x20C);
FIELD_BSS_ALIAS(D_800B2384, g_FieldBss_800B2174, 0x210);
FIELD_BSS_ALIAS(D_800B2394, g_FieldBss_800B2174, 0x220);
FIELD_BSS_ALIAS(D_800B2398, g_FieldBss_800B2174, 0x224);
FIELD_BSS_ALIAS(D_800B239C, g_FieldBss_800B2174, 0x228);
FIELD_BSS_ALIAS(D_800B23A0, g_FieldBss_800B2174, 0x22C);
FIELD_BSS_ALIAS(D_800B23A4, g_FieldBss_800B2174, 0x230);
FIELD_BSS_ALIAS(D_800B23A8, g_FieldBss_800B2174, 0x234);

FIELD_BSS_ALIAS(g_FieldSystemMode, g_FieldBss_800C2688, 0x004);
FIELD_BSS_ALIAS(D_800C2690, g_FieldBss_800C2688, 0x008);
FIELD_BSS_ALIAS(D_800C2692, g_FieldBss_800C2688, 0x00A);
FIELD_BSS_ALIAS(D_800C2694, g_FieldBss_800C2688, 0x00C);
FIELD_BSS_ALIAS(g_FieldTextBoxes, g_FieldBss_800C2688, 0x010);
FIELD_BSS_ALIAS(D_800C2B30, g_FieldBss_800C2688, 0x4A8);

FIELD_BSS_ALIAS(D_800C38F8, g_FieldBss_800C38F8, 0x000);
FIELD_BSS_ALIAS(D_800C38FC, g_FieldBss_800C38F8, 0x004);
FIELD_BSS_ALIAS(D_800C38FE, g_FieldBss_800C38F8, 0x006);
FIELD_BSS_ALIAS(D_800C3900, g_FieldBss_800C38F8, 0x008);
FIELD_BSS_ALIAS(g_Field24BitImageData, g_FieldBss_800C38F8, 0x00C);
FIELD_BSS_ALIAS(D_800C3908, g_FieldBss_800C38F8, 0x010);
FIELD_BSS_ALIAS(g_Field15BitImageData, g_FieldBss_800C38F8, 0x014);
FIELD_BSS_ALIAS(D_800C3910, g_FieldBss_800C38F8, 0x018);
FIELD_BSS_ALIAS(D_800C3914, g_FieldBss_800C38F8, 0x01C);
FIELD_BSS_ALIAS(D_800C3A18, g_FieldBss_800C38F8, 0x120);
FIELD_BSS_ALIAS(D_800C3A38, g_FieldBss_800C38F8, 0x140);
FIELD_BSS_ALIAS(D_800C3A3C, g_FieldBss_800C38F8, 0x144);
FIELD_BSS_ALIAS(D_800C3A40, g_FieldBss_800C38F8, 0x148);
FIELD_BSS_ALIAS(D_800C3A44, g_FieldBss_800C38F8, 0x14C);
FIELD_BSS_ALIAS(D_800C3A4C, g_FieldBss_800C38F8, 0x154);
FIELD_BSS_ALIAS(D_800C3A50, g_FieldBss_800C38F8, 0x158);
FIELD_BSS_ALIAS(D_800C3A54, g_FieldBss_800C38F8, 0x15C);
FIELD_BSS_ALIAS(D_800C3A5C, g_FieldBss_800C38F8, 0x164);
FIELD_BSS_ALIAS(D_800C3A60, g_FieldBss_800C38F8, 0x168);
FIELD_BSS_ALIAS(g_FieldScriptMemory, g_FieldBss_800C38F8, 0x170);
FIELD_BSS_ALIAS(D_800C4268, g_FieldBss_800C38F8, 0x970);

/*
 * Main-executable encounter section (retail 0x800658DC..0x80065B0C).
 *
 * Retail keeps this as ONE contiguous object: 16 formation records of 0x20
 * bytes (0x200 total; the 0x20 stride is visible in battle asm
 * asm/battle/nonmatchings/main/func_80070F40.s, which does
 * `lbu D_80059508` / `sll $a1,$a1,5` before memmove off %hi/%lo(D_800658DC)),
 * immediately followed by the 16 per-formation encounter WEIGHT bytes at
 * +0x200 -- an address that is exactly D_80065ADC. The already-ported
 * world-map selector reads the same shape out of the record blob
 * (pc_port/src/world_map_helper_75e7c.c:104 -- `record_base + 0x200 +
 * bucket * 0x10`, 16 weight bytes).
 *
 * FieldLoad LZSS-decompresses that whole section to the unshifted base in a
 * single call (src/field/main/misc3.c:831-833, retail 80071054..8007106C);
 * map 2's stream decodes 0x212 bytes -- all sixteen records plus a 2-byte
 * tail. See docs/evidence/opening-battle-encounter-20260905/README.md.
 *
 * The weighted encounter roll func_80079288 (src/field/main/misc4.c:314-333)
 * sums D_80065ADC[0..15]. While D_800658DC and D_80065ADC were two SEPARATE
 * native objects (an auto-generated stub plus a standalone 16-byte definition
 * in game_overrides.c), that entire 0x212-byte decode landed inside the first
 * object, the weight table was never written, sum was 0, `selected` stayed -1
 * and the roll returned early -- so no random encounter could fire anywhere in
 * the port (mountain path map15 / Blackmoon Forest map16 included).
 *
 * Backing both labels with one block restores the retail object. 0x230 is the
 * retail span D_800658DC..D_80065B0C (config/symbol_addrs.slus_006.64.txt:
 * 0x200 + 0x30), so the 0x212-byte decode fits with the tail intact.
 * Regression test: pc_port/tests/run_encounter_weight_aliasing_test.sh.
 */
unsigned char g_SlusBss_800658DC[0x230] __attribute__((aligned(8)));
FIELD_BSS_ALIAS(D_800658DC, g_SlusBss_800658DC, 0x000);
FIELD_BSS_ALIAS(D_80065ADC, g_SlusBss_800658DC, 0x200);

#undef FIELD_BSS_ALIAS

/* Overlay-region global (retail 0x80285988, PC-HDD debug overlay): set to 1
 * by func_8008399C when an interaction/talk starts in SYSTEM_MODE_PC_HDD.
 * The port runs SYSTEM_MODE_CD_ROM so the write is retail-dead here, but the
 * symbol must exist to link the faithful branch. */
s32 D_80285988;

/* Retail data, loaded from the user's disc before main() (retail_data.h):
 * symbol, retail file, file offset (from the guest address), size. */
#include "retail_data.h"

_Static_assert(sizeof(g_FieldOverlayLayoutData) == 0x710, "g_FieldOverlayLayoutData size");
_Static_assert(sizeof(D_8006FC48) == 0xE, "D_8006FC48 size");
_Static_assert(sizeof(D_800ADC24) == 0x20, "D_800ADC24 size");
_Static_assert(sizeof(D_800ADC44) == 0x6C, "D_800ADC44 size");
_Static_assert(sizeof(D_800ADCB0) == 0x18, "D_800ADCB0 size");
_Static_assert(sizeof(D_800ADCC8) == 0x18, "D_800ADCC8 size");
_Static_assert(sizeof(g_FieldAngleToDirectionLUT) == 0x10, "g_FieldAngleToDirectionLUT size");
_Static_assert(sizeof(D_800AEA44) == 0x10, "D_800AEA44 size");
_Static_assert(sizeof(g_FieldParticleShapes) == 0x1F8, "g_FieldParticleShapes size");
_Static_assert(sizeof(D_800AF474) == 0x8, "D_800AF474 size");
_Static_assert(sizeof(D_800AEA54) == 0x10, "D_800AEA54 size");
_Static_assert(sizeof(D_800AEA64) == 0x80, "D_800AEA64 size");
_Static_assert(sizeof(D_800AE1E0) == 0xB4, "D_800AE1E0 size");
_Static_assert(sizeof(D_800AEAE4) == 0x40, "D_800AEAE4 size");
_Static_assert(sizeof(D_800AEB24) == 0x3C, "D_800AEB24 size");
_Static_assert(sizeof(g_FieldData_800ADCE0) == 0x1B0, "g_FieldData_800ADCE0 size");
_Static_assert(sizeof(g_FieldData_800ADE9C) == 0xB8, "g_FieldData_800ADE9C size");
_Static_assert(sizeof(g_FieldData_800ADF54) == 0x10, "g_FieldData_800ADF54 size");
_Static_assert(sizeof(g_FieldData_800ADF64) == 0x68, "g_FieldData_800ADF64 size");
_Static_assert(sizeof(g_FieldData_800ADFCC) == 0x94, "g_FieldData_800ADFCC size");

XENO_RETAIL_DATA_BEGIN(field)
    XENO_RD(g_FieldOverlayLayoutData, XENO_RD_FIELD, XENO_RD_FIELD_OFF(0x800AEB68u), 0x710),
    XENO_RD(D_8006FC48, XENO_RD_FIELD, XENO_RD_FIELD_OFF(0x8006FC48u), 0xE),
    XENO_RD(D_800ADC24, XENO_RD_FIELD, XENO_RD_FIELD_OFF(0x800ADC24u), 0x20),
    XENO_RD(D_800ADC44, XENO_RD_FIELD, XENO_RD_FIELD_OFF(0x800ADC44u), 0x6C),
    XENO_RD(D_800ADCB0, XENO_RD_FIELD, XENO_RD_FIELD_OFF(0x800ADCB0u), 0x18),
    XENO_RD(D_800ADCC8, XENO_RD_FIELD, XENO_RD_FIELD_OFF(0x800ADCC8u), 0x18),
    XENO_RD(g_FieldAngleToDirectionLUT, XENO_RD_FIELD, XENO_RD_FIELD_OFF(0x800AEA34u), 0x10),
    XENO_RD(D_800AEA44, XENO_RD_FIELD, XENO_RD_FIELD_OFF(0x800AEA44u), 0x10),
    XENO_RD(g_FieldParticleShapes, XENO_RD_FIELD, XENO_RD_FIELD_OFF(0x800AF27Cu), 0x1F8),
    XENO_RD(D_800AF474, XENO_RD_FIELD, XENO_RD_FIELD_OFF(0x800AF474u), 0x8),
    XENO_RD(D_800AEA54, XENO_RD_FIELD, XENO_RD_FIELD_OFF(0x800AEA54u), 0x10),
    XENO_RD(D_800AEA64, XENO_RD_FIELD, XENO_RD_FIELD_OFF(0x800AEA64u), 0x80),
    XENO_RD(D_800AE1E0, XENO_RD_FIELD, XENO_RD_FIELD_OFF(0x800AE1E0u), 0xB4),
    XENO_RD(D_800AEAE4, XENO_RD_FIELD, XENO_RD_FIELD_OFF(0x800AEAE4u), 0x40),
    XENO_RD(D_800AEB24, XENO_RD_FIELD, XENO_RD_FIELD_OFF(0x800AEB24u), 0x3C),
    XENO_RD(g_FieldData_800ADCE0, XENO_RD_FIELD, XENO_RD_FIELD_OFF(0x800ADCE0u), 0x1B0),
    XENO_RD(g_FieldData_800ADE9C, XENO_RD_FIELD, XENO_RD_FIELD_OFF(0x800ADE9Cu), 0xB8),
    XENO_RD(g_FieldData_800ADF54, XENO_RD_FIELD, XENO_RD_FIELD_OFF(0x800ADF54u), 0x10),
    XENO_RD(g_FieldData_800ADF64, XENO_RD_FIELD, XENO_RD_FIELD_OFF(0x800ADF64u), 0x68),
    XENO_RD(g_FieldData_800ADFCC, XENO_RD_FIELD, XENO_RD_FIELD_OFF(0x800ADFCCu), 0x94),
XENO_RETAIL_DATA_END(field)
