#include "common.h"

#include "field/main.h"
#include "psyq/libgpu.h"
#include "psyq/libgte.h"
#include "psyq/libetc.h"
#include "system/controller.h"

void FieldInitializeControllersAndMouse(void) {
    /* Real asm (asm/field/main/init.s) computes the second arg as
     * g_C1Buffer + 0x22 (a single delay-slot addiu), not an independent
     * symbol load -- g_C2Buffer (0x8006261E) is exactly g_C1Buffer's address
     * (0x800625FC) + 0x22. Match that idiom: g_C2Buffer is real retail data
     * (asm/slus_006.64/data/49AC0.bss.s, symbol_addrs), but is never itself
     * referenced by any assembled field.elf object, so gears' auto-generated
     * cross-overlay symbol table (linker/undefined_syms_auto.field.txt) never
     * picks it up and the link fails. Using the offset form sidesteps that
     * gap and is the byte-matching-faithful expression besides. */
    FieldSetControllerBuffers(&g_C1Buffer, (u_char*)&g_C1Buffer + 0x22);
    FieldSetMouseSpeed(3, 4);
    func_8007ADA4(0, 0x140, 0, 0xE0);
    FieldSetMousePosition(0, 0x50, 0x64);
    FieldSetMousePosition(1, 0xFA, 0x64);
    func_8007ADA4(0, 0x12C, 0xA, 0xDC);
}

void FieldSetClipDimensions(short x, short y, short w, short h) {
    g_FieldRenderContexts[0].drawEnvs[0].clip.y = y;
    g_FieldRenderContexts[0].drawEnvs[0].clip.x = x;
    g_FieldRenderContexts[0].drawEnvs[0].clip.w = w;
    g_FieldRenderContexts[0].drawEnvs[0].clip.h = h;
    g_FieldRenderContexts[1].drawEnvs[0].clip.x = x;
    g_FieldRenderContexts[1].drawEnvs[0].clip.y = y + 0x100;
    g_FieldRenderContexts[1].drawEnvs[0].clip.w = w;
    g_FieldRenderContexts[1].drawEnvs[0].clip.h = h;
}

void FieldInitializeRenderContexts(void) {
    D_80059198 = 1;
    DrawSync(0);
    Vsync(0);
    InitGeom();
    SetGeomOffset(0xA0, 0x70);
    SetDefDrawEnv(&g_FieldRenderContexts[0].drawEnvs[0], 0, 0, 0x140, 0xE0);
    SetDefDrawEnv(&g_FieldRenderContexts[1].drawEnvs[0], 0, 0x100, 0x140, 0xE0);
    SetDefDrawEnv(&g_FieldRenderContexts[0].drawEnvs[1], 0, 0, 0x140, 0xE0);
    SetDefDrawEnv(&g_FieldRenderContexts[1].drawEnvs[1], 0, 0x100, 0x140, 0xE0);
    SetDefDispEnv(&g_FieldRenderContexts[0].dispEnv, 0, 0x100, 0x140, 0xE0);
    SetDefDispEnv(&g_FieldRenderContexts[1].dispEnv, 0, 0, 0x140, 0xE0);
    FieldSetClipDimensions(0x0, 0x0, 0x140, 0xE0);
    FieldSetScreenDimensions();

    g_FieldRenderContexts[0].drawEnvs[0].r0 = 0;
    g_FieldRenderContexts[0].drawEnvs[0].g0 = 0;
    g_FieldRenderContexts[0].drawEnvs[0].b0 = 0;
    g_FieldRenderContexts[1].drawEnvs[0].r0 = 0;
    g_FieldRenderContexts[1].drawEnvs[0].g0 = 0;
    g_FieldRenderContexts[1].drawEnvs[0].b0 = 0;
    g_FieldRenderContexts[0].drawEnvs[0].dtd = 1;
    g_FieldRenderContexts[1].drawEnvs[0].dtd = 1;
    
    Vsync(0);
    PutDispEnv(&g_FieldRenderContexts[1].dispEnv);
    PutDrawEnv(&g_FieldRenderContexts[1].drawEnvs[0]);
    func_8002DFF0(0x140, 0xF0);
}
