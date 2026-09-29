/* Execute the retail SLUS routine and its GetClut/GetTPage/AddPrim callees.
 * Only the two graphics globals, fixture RAM and guest stack are mapped.
 * Compare all fixture bytes and the allocation cursor against production C.
 * The old sixth-argument implementation is given a guarded trap buffer by
 * the runner; that argument is absent from the retail five-argument API.
 */
#include "battle_mips_adapter.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
void *g_GfxCurWorkBuffer, *g_GfxCurWorkBufferEnd;
uint8_t *g_GfxCurOT;
extern void atlas_test_call(uint8_t *, int32_t, int32_t, int32_t, void *,
                            void *);
static uint8_t exe[0x70000], stack[0x2000];
static struct Fixture {
  uint8_t table[512], arena[512], trap[512];
  uint32_t ot;
} f, initial, expected;
static uint32_t cursor, end;
static uint8_t *resolve(uint32_t a, unsigned n) {
  uintptr_t p = (uintptr_t)&f;
  if (a >= p && (uint64_t)a + n <= p + sizeof(f))
    return (uint8_t *)(uintptr_t)a;
  if (a == 0x80059580 && n == 4)
    return (uint8_t *)&cursor;
  if (a == 0x80059534 && n == 4)
    return (uint8_t *)&end;
  if (a >= 0x801fe000 && (uint64_t)a + n <= 0x80200000)
    return stack + a - 0x801fe000;
  if (a >= 0x8000f800 && (uint64_t)a + n <= 0x8000f800 + sizeof(exe))
    return exe + a - 0x8000f800;
  return NULL;
}
static int rd(void *u, uint32_t a, unsigned n, uint32_t *v) {
  (void)u;
  uint8_t *p = resolve(a, n);
  if (!p)
    return -1;
  *v = 0;
  for (unsigned i = 0; i < n; i++)
    *v |= (uint32_t)p[i] << (8 * i);
  return 0;
}
static int wr(void *u, uint32_t a, unsigned n, uint32_t v) {
  (void)u;
  uint8_t *p = resolve(a, n);
  if (!p)
    return -1;
  for (unsigned i = 0; i < n; i++)
    p[i] = v >> (8 * i);
  return 0;
}
int GetClut(int x, int y) { return ((uint32_t)y << 6) | ((x >> 4) & 63); }
int GetTPage(int tp, int abr, int x, int y) {
  return ((tp & 3) << 7) | ((abr & 3) << 5) | ((y & 0x100) >> 4) |
         ((x & 0x3ff) >> 6) | ((y & 0x200) << 2);
}
void AddPrim(void *o, void *p) {
  uint32_t ot, tag;
  memcpy(&ot, o, 4);
  memcpy(&tag, p, 4);
  tag = (tag & 0xff000000) | (ot & 0xffffff);
  ot = (ot & 0xff000000) | ((uintptr_t)p & 0xffffff);
  memcpy(p, &tag, 4);
  memcpy(o, &ot, 4);
}
static void put16(uint8_t *p, uint16_t v) { memcpy(p, &v, 2); }
static uint32_t rng = 0xabc98765;
static uint32_t next(void) {
  rng = rng * 1664525u + 1013904223u;
  return rng;
}
int main(void) {
  FILE *fp = fopen("disc/SLUS_006.64", "rb");
  if (!fp)
    return 2;
  size_t loaded = fread(exe, 1, sizeof(exe), fp);
  fclose(fp);
  if (loaded < 0x40000)
    return 2;
  unsigned cases = 0;
  for (unsigned trial = 0; trial < 200; trial++)
    for (unsigned count = 0; count <= 4; count++)
      for (int slack = -1; slack <= 1; slack++) {
        for (unsigned i = 0; i < sizeof(f); i++)
          ((uint8_t *)&f)[i] = next() >> 24;
        put16(f.table + 6, 32);
        put16(f.table + 32, count);
        for (unsigned i = 0; i < count; i++) {
          uint8_t *p = f.table + 36 + i * 28;
          put16(p + 16, trial % 3);
        }
        initial = f;
        int32_t x = (int32_t)next(), y = (int32_t)next();
        static const int32_t edges[] = {INT32_MIN, INT32_MAX, -1, 0};
        if (trial < 4) {
          x = edges[trial];
          y = edges[3 - trial];
        }
        cursor = (uintptr_t)f.arena + 16;
        end = cursor + count * 40 + slack;
        PcPortMipsBus bus = {.read = rd, .write = wr};
        PcPortMipsCpu cpu;
        PcPortMipsCpuInit(&cpu, &bus);
        cpu.gpr[4] = (uintptr_t)f.table;
        cpu.gpr[5] = 1;
        cpu.gpr[6] = x;
        cpu.gpr[7] = y;
        cpu.gpr[29] = 0x801fff00;
        cpu.gpr[31] = 0xfffffffc;
        wr(NULL, 0x801fff10, 4, (uintptr_t)&f.ot);
        wr(NULL, 0x801fff14, 4, (uintptr_t)f.trap);
        if (PcPortMipsRun(&cpu, 0x80026ba4, 0xfffffffc, 100000) !=
            PC_PORT_MIPS_HALTED) {
          fprintf(stderr, "oracle %s\n", cpu.error);
          return 3;
        }
        expected = f;
        uint32_t expected_cursor = cursor;
        f = initial;
        g_GfxCurWorkBuffer = f.arena + 16;
        g_GfxCurWorkBufferEnd = (void *)(uintptr_t)end;
        g_GfxCurOT = (uint8_t *)&f.ot;
        atlas_test_call(f.table, 1, x, y, &f.ot, f.trap);
        if (memcmp(&f, &expected, sizeof(f)) ||
            (uintptr_t)g_GfxCurWorkBuffer != expected_cursor) {
          fprintf(
              stderr,
              "MISMATCH trial=%u count=%u slack=%d cursor=%lx expected=%x\n",
              trial, count, slack, (unsigned long)g_GfxCurWorkBuffer,
              expected_cursor);
          return 1;
        }
        cases++;
      }
  printf("ATLAS WORK BUFFER RETAIL PASS cases=%u\n", cases);
  return 0;
}
