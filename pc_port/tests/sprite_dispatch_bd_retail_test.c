/* Real dispatcher instructions; child creation is a recorded call boundary. */
#include "battle_mips_adapter.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
extern void func_8001FBE4(void *, uint32_t, void *);
extern uint8_t D_8006BE10[32];
static uint8_t ram[0x200000];
static struct {
  uint8_t sprite[256], ops[8], table[0x204], guard[16];
} f, initial, expected;
static unsigned calls, cases;
static uint32_t args[3], want[3];
static void put32(void *p, uint32_t v) { memcpy(p, &v, 4); }
static uint8_t *addr(uint32_t a, unsigned w) {
  uintptr_t base = (uintptr_t)&f;
  if (a >= base && (uint64_t)a + w <= base + sizeof(f))
    return (uint8_t *)(uintptr_t)a;
  if (a >= 0x80000000 && (uint64_t)a + w <= 0x80200000)
    return ram + (a & 0x1fffff);
  return NULL;
}
static int rd(void *u, uint32_t a, unsigned w, uint32_t *v) {
  (void)u;
  uint8_t *p = addr(a, w);
  if (!p)
    return -1;
  *v = 0;
  for (unsigned i = 0; i < w; i++)
    *v |= (uint32_t)p[i] << (8 * i);
  return 0;
}
static int wr(void *u, uint32_t a, unsigned w, uint32_t v) {
  (void)u;
  uint8_t *p = addr(a, w);
  if (!p)
    return -1;
  for (unsigned i = 0; i < w; i++)
    p[i] = v >> (8 * i);
  return 0;
}
static void record(uint32_t a, uint32_t b, uint32_t c) {
  calls++;
  args[0] = a;
  args[1] = b;
  args[2] = c;
  f.sprite[0x3c] ^= 0x55;
}
void *func_80023B84(void *a, void *b, void *c) {
  record((uintptr_t)a, (uintptr_t)b, (uintptr_t)c);
  return (void *)0x123456;
}
static int bridge(void *u, PcPortMipsCpu *c, uint32_t a) {
  (void)u;
  if (a != 0x80023b84)
    return 0;
  record(c->gpr[4], c->gpr[5], c->gpr[6]);
  c->gpr[2] = 0x123456;
  return 1;
}
static void compare(unsigned index, unsigned off, unsigned variant) {
  for (unsigned i = 0; i < sizeof(f); i++)
    ((uint8_t *)&f)[i] = (i * 17 + index) & 255;
  uint8_t *table = variant & 1 ? f.sprite : f.table;
  uint8_t *ops = variant & 2 ? f.sprite + 0x24 : f.ops;
  put32(f.sprite + 0x24, 0xabc00100u + off);
  ops[0] = index;
  /* Alias-table cases use small indices so the full offset halfword is valid.
   */
  uint16_t half = off;
  memcpy(table + 2 + index * 2, &half, 2);
  put32(ram + 0x6be20, (uintptr_t)table);
  initial = f;
  calls = 0;
  PcPortMipsBus bus = {.read = rd, .write = wr, .bridge = bridge};
  PcPortMipsCpu c;
  PcPortMipsCpuInit(&c, &bus);
  c.gpr[4] = (uintptr_t)f.sprite;
  c.gpr[5] = 0xBD | (variant & 4 ? 0xa5120000u : 0);
  c.gpr[6] = (uintptr_t)ops;
  c.gpr[29] = 0x801fff00;
  c.gpr[31] = 0xfffffffc;
  if (PcPortMipsRun(&c, 0x8001fbe4, 0xfffffffc, 400) != PC_PORT_MIPS_HALTED) {
    fprintf(stderr, "BD oracle %s\n", c.error);
    exit(2);
  }
  if (calls != 1)
    exit(2);
  expected = f;
  memcpy(want, args, sizeof(args));
  f = initial;
  calls = 0;
  put32(D_8006BE10 + 16, (uintptr_t)table);
  func_8001FBE4(f.sprite, 0xbd | (variant & 4 ? 0xa5120000u : 0), ops);
  if (calls != 1 || memcmp(args, want, sizeof(args)) ||
      memcmp(&f, &expected, sizeof(f))) {
    fprintf(stderr, "SPRITE BD FAIL case=%u index=%u offset=%u variant=%u\n",
            cases, index, off, variant);
    exit(1);
  }
  cases++;
}
int main(void) {
  FILE *p = fopen("disc/SLUS_006.64", "rb");
  if (!p)
    return 2;
  fseek(p, 0x800, SEEK_SET);
  size_t n = fread(ram + 0x10000, 1, sizeof(ram) - 0x10000, p);
  fclose(p);
  if (n < 0x11ac0)
    return 2;
  for (unsigned o = 0; o < 65536; o++) {
    compare(0, o, 0);
    compare(255, o, 0);
    compare(1, o, 3);
    compare(17, o, 4);
  }
  for (unsigned i = 0; i < 256; i++) {
    compare(i, 0x8000, 0);
    compare(i, 0xffff, 2);
  }
  printf("SPRITE BD PASS cases=%u child-helper boundary only\n", cases);
}
