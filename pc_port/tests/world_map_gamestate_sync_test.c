/* The world map's g_GameState fields have one value (world_map_gamestate.h):
 * the saved world position (func_8008E034 / func_8008DFF4) and the field
 * transition tuple written by the mode teardowns must reach the host
 * g_GameState that FieldMain and save/load read. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_gamestate.h"
#include "world_map_helper_8dff4.h"
#include "world_map_helper_8e034.h"

uint8_t g_PsxRam[PSX_RAM_SIZE];
unsigned char g_GameState[0x4600];

static int fails;
static void check(int ok, const char* what)
{
    printf("%s %s\n", ok ? "ok  " : "FAIL", what);
    fails += !ok;
}
static uint16_t host16(uint32_t off) { uint16_t v; memcpy(&v, g_GameState + off, 2); return v; }
static uint16_t guest16(uint32_t a) { uint16_t v; memcpy(&v, PSX_ADDR(a), 2); return v; }

int main(void)
{
    const uint32_t vec = 0x800A0000u;
    int32_t in[3] = { 0x1234000, -0x2000, 0x7FF000 };
    int32_t out[3];

    memcpy(PSX_ADDR(vec), in, sizeof(in));
    wm_8008E034(vec);
    check(host16(0x182C) == 0x1234 && host16(0x182E) == 0xFFFE && host16(0x1830) == 0x07FF,
          "save: world position reaches host g_GameState+0x182C");
    check(guest16(0x8006EE60u) == 0x1234 && guest16(0x8006EE64u) == 0x07FF,
          "save: guest copy kept for world-map readers");

    /* A loaded save changes only the host copy; restore must use it. */
    memset(PSX_ADDR(0x8006EE60u), 0, 6);
    { uint16_t v[3] = { 0x0100, 0xFF80, 0x0040 }; memcpy(g_GameState + 0x182C, v, 6); }
    wm_8008DFF4(vec + 0x10u);
    memcpy(out, PSX_ADDR(vec + 0x10u), sizeof(out));
    check(out[0] == 0x100000 && out[1] == -0x80000 && out[2] == 0x40000,
          "restore: world position read from host g_GameState");

    /* Mode teardown stores (e.g. mode 8/11: map 17, entrance 7). */
    wm_gs_sh(0x8006F94Eu, 17u);
    wm_gs_sh(0x8006F954u, 7u);
    check(host16(0x231A) == 17 && host16(0x2320) == 7 && guest16(0x8006F94Eu) == 17,
          "teardown: field map / entrance reach host D_8006F94E / D_8006F954");

    /* Outside g_GameState only guest RAM is touched. */
    wm_gs_sh(0x8009BD3Au, 0x1234u);
    check(guest16(0x8009BD3Au) == 0x1234 && wm_gs_lhu(0x8009BD3Au) == 0x1234,
          "non-g_GameState address stays guest-only");
    printf("world map g_GameState sync test: %s\n", fails ? "FAIL" : "PASS");
    return fails != 0;
}
