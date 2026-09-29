/* Retail93 and its real allocator/initializer/format leaf. Heap and frame
 * selection are recorded boundaries; no frame-render parity claim. */
#include "battle_mips_adapter.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
extern void func_8001FBE4(void *, uint32_t, void *);
static uint8_t ram[0x200000];
static struct {
  uint8_t sprite[256], parent[256], model[80], othermodel[80], parentmodel[80],
      package[16], format[16], dirs[256];
} f, initial, expected;
static unsigned cases, heapcalls, framecalls, frame, rebind;
static uint32_t flags_at_frame;
static void put32(void *p, uint32_t v) { memcpy(p, &v, 4); }
static uint32_t get32(void *p) {
  uint32_t v;
  memcpy(&v, p, 4);
  return v;
}
static void fail(const char *s) {
  fprintf(stderr, "SPRITE93 FAIL case=%u %s\n", cases, s);
  exit(1);
}
static void *allocate(uint32_t size, uint32_t flags) {
  if (size != 64 || flags)
    fail("heap arguments");
  heapcalls++;
  if (rebind)
    put32(f.sprite + 32, (uintptr_t)f.othermodel);
  return f.dirs + 128;
}
void *HeapAlloc(unsigned n, unsigned flags) { return allocate(n, flags); }
void func_8001D2B0(void *p, int16_t n) {
  if (p != f.sprite)
    fail("frame sprite");
  framecalls++;
  frame = (uint16_t)n;
  flags_at_frame = get32(f.sprite + 0x40);
}
static uint8_t *addr(uint32_t a, unsigned w) {
  uintptr_t b = (uintptr_t)&f;
  if (a >= b && (uint64_t)a + w <= b + sizeof(f))
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
static int bridge(void *u, PcPortMipsCpu *c, uint32_t t) {
  (void)u;
  if (t == 0x80031bdc) {
    c->gpr[2] = (uintptr_t)allocate(c->gpr[4], c->gpr[5]);
    return 1;
  }
  if (t == 0x8001d2b0) {
    func_8001D2B0((void *)(uintptr_t)c->gpr[4], c->gpr[5]);
    return 1;
  }
  return 0;
}
static void run(unsigned gate, unsigned mode, unsigned fmt, unsigned variant,
                unsigned value) {
  for (unsigned i = 0; i < sizeof(f); i++)
    ((uint8_t *)&f)[i] = (i * 31 + value) & 255;
  put32(f.sprite + 0x70, gate == 0 ? 0 : (uintptr_t)f.parent);
  put32(f.sprite + 0x3c, 0xa5110040 | mode);
  put32(f.sprite + 0x24, (uintptr_t)f.package);
  put32(f.package, (uintptr_t)f.format);
  f.format[1] = fmt;
  put32(f.sprite + 0x20, gate == 1 ? 0 : (uintptr_t)f.model);
  put32(f.parent + 0x20, (uintptr_t)f.parentmodel);
  put32(f.parentmodel + 0x34, gate == 2 ? 0 : (uintptr_t)(f.dirs + 32));
  int offsets[] = {128, 32, 33, 36, 31, 28, 40};
  put32(f.model + 0x34,
        variant == 0 ? 0 : (uintptr_t)(f.dirs + offsets[variant]));
  put32(f.othermodel + 0x34, (uintptr_t)(f.dirs + 192));
  uint16_t fr = value;
  memcpy(f.sprite + 0x34, &fr, 2);
  rebind = variant == 0 && (value & 1);
  initial = f;
  heapcalls = framecalls = frame = flags_at_frame = 0;
  PcPortMipsBus b = {.read = rd, .write = wr, .bridge = bridge};
  PcPortMipsCpu c;
  PcPortMipsCpuInit(&c, &b);
  c.gpr[4] = (uintptr_t)f.sprite;
  c.gpr[5] = 0xa5340093;
  c.gpr[6] = 0;
  c.gpr[29] = 0x801fff00;
  c.gpr[31] = 0xfffffffc;
  if (PcPortMipsRun(&c, 0x8001fbe4, 0xfffffffc, 3000) != PC_PORT_MIPS_HALTED) {
    fprintf(stderr, "oracle %s pc%x\n", c.error, c.pc);
    exit(2);
  }
  expected = f;
  unsigned h = heapcalls, fc = framecalls, frv = frame, fl = flags_at_frame;
  f = initial;
  heapcalls = framecalls = frame = flags_at_frame = 0;
  func_8001FBE4(f.sprite, 0xa5340093, NULL);
  if (memcmp(&f, &expected, sizeof(f)) || h != heapcalls || fc != framecalls ||
      frv != frame || fl != flags_at_frame)
    fail("complete fixture or call trace");
  cases++;
}
int main(void) {
  FILE *p = fopen("disc/SLUS_006.64", "rb");
  if (!p)
    return 2;
  fseek(p, 0x800, SEEK_SET);
  size_t n = fread(ram + 0x10000, 1, sizeof(ram) - 0x10000, p);
  fclose(p);
  if (n < 0x134e0)
    return 2;
  for (unsigned g = 0; g < 4; g++)
    for (unsigned m = 0; m < 4; m++)
      for (unsigned fmt = 0; fmt < 256; fmt++)
        for (unsigned v = 0; v < 7; v++)
          run(g, m, fmt, v, fmt * 257 + v);
  for (unsigned n = 0; n < 65536; n++)
    run(3, 1, n & 255, 0, n);
  printf("SPRITE93 PASS cases=%u actual allocator and direction initializer; "
         "frame boundary\n",
         cases);
}
