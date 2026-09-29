/* Compare the production binder up to its first allocation with actual
 * retail instructions. Allocators are terminal observation boundaries;
 * later sprite setup is explicitly outside this test. */
#include "battle_mips_adapter.h"
#include "common.h"
#include "field/actor.h"
#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define ENTRY 0x80076AC0u
#define STOP 0xfffffffcu
#define BASE 0x80130000u
#define DATA 0x80140000u
#define STACK 0x801ff000u
static unsigned char ram[0x200000], code[0x540];
static unsigned char actors[0x5c * 2] __attribute__((aligned(16))),
    data[0x138] __attribute__((aligned(16)));
FieldActor *volatile g_FieldActors;
u8 D_800B1F78[256 * 8];
s32 g_GamePartySkinsInitialized, D_800AFC74;
s16 D_800B218E;
static jmp_buf stop;
static unsigned cases, native;
static struct Result {
  unsigned calls, freed, id, args[7];
  unsigned char bytes[0x138];
} result;
static void check(int ok) {
  if (!ok) {
    fprintf(stderr, "BIND ARGS FAIL case=%u native=%u\n", cases, native);
    exit(1);
  }
}
static unsigned rd(unsigned a) {
  unsigned v;
  memcpy(&v, ram + (a & 0x1fffff), 4);
  return v;
}
static void wr(unsigned a, unsigned v) { memcpy(ram + (a & 0x1fffff), &v, 4); }
void HeapChangeCurrentUser(unsigned user, void *p) {
  check(user == 8 && !p);
  result.calls++;
}
void func_800230A8(void *p) {
  check((uintptr_t)p == 0x12340);
  result.freed++;
}
static void alloc_event(unsigned id, void *p, s16 x, s16 y, s16 cx, s16 cy,
                        s16 flags, s32 page) {
  result.id = id;
  unsigned a[] = {(unsigned)(uintptr_t)p, (unsigned)(s32)x,
                  (unsigned)(s32)y,       (unsigned)(s32)cx,
                  (unsigned)(s32)cy,      (unsigned)(s32)flags,
                  (unsigned)page};
  memcpy(result.args, a, sizeof(a));
  longjmp(stop, 1);
}
void *func_80024524(void *p, s16 x, s16 y, s16 cx, s16 cy, s16 f) {
  alloc_event(1, p, x, y, cx, cy, f, 0);
  return NULL;
}
void *func_80024294(void *p, s16 x, s16 y, s16 cx, s16 cy, s16 f, s32 pg) {
  alloc_event(2, p, x, y, cx, cy, f, pg);
  return NULL;
}
/* Unreachable post-allocation dependencies: never substitutes behavior under
 * test. */
void func_80023340(void *p, s32 n) { abort(); }
void func_8001F5BC(void *p, s32 n, s32 *a, s32 *b, s32 *c) { abort(); }
void func_80021C00(void *p, u32 n) { abort(); }
void func_800245D8(void *p, s16 n) { abort(); }
void func_80021FE0(void *p, s16 n) { abort(); }
void func_80021BF8(void *p, s32 n) { abort(); }
void AnimScriptTick(void *p) { abort(); }
void TimerWorkListUpdate(void) { abort(); }
void func_80076A74(void *p) { abort(); }
extern void func_80076AC0(s32, s32, void *, s32, s32, s32, s32);
static int readbus(void *o, u32 a, unsigned w, u32 *v) {
  if (a < 0x80000000u || (uint64_t)a + w > 0x80200000u)
    return -1;
  *v = 0;
  for (unsigned i = 0; i < w; i++)
    *v |= (u32)ram[(a & 0x1fffff) + i] << (i * 8);
  return 0;
}
static int writebus(void *o, u32 a, unsigned w, u32 v) {
  if (a < 0x80000000u || (uint64_t)a + w > 0x80200000u)
    return -1;
  for (unsigned i = 0; i < w; i++)
    ram[(a & 0x1fffff) + i] = v >> (i * 8);
  return 0;
}
static int bridge(void *o, PcPortMipsCpu *c, u32 target) {
  if (target == 0x80032498) {
    check(c->gpr[4] == 8 && c->gpr[5] == 0);
    result.calls++;
    return 1;
  }
  if (target == 0x800230a8) {
    check(c->gpr[4] == 0x12340);
    result.freed++;
    return 1;
  }
  if (target == 0x80024524 || target == 0x80024294) {
    result.id = target == 0x80024524 ? 1 : 2;
    for (unsigned i = 0; i < 4; i++)
      result.args[i] = c->gpr[4 + i];
    for (unsigned i = 4; i < 7; i++)
      result.args[i] = rd(c->gpr[29] + 16 + (i - 4) * 4);
    if (result.id == 1)
      result.args[6] = 0;
    c->gpr[31] = STOP;
    return 1;
  }
  return 0;
}
static struct Result run(unsigned nat, int slot, int mode, int page,
                         int selector, int flag) {
  native = nat;
  memset(&result, 0, sizeof(result));
  memset(data, 0xa5, sizeof(data));
  memset(actors, 0, sizeof(actors));
  unsigned short status = flag ? 1 : 0;
  memcpy(actors + 0x5a, &status, 2);
  unsigned q = 0x12340;
  memcpy(actors + 4, &q, 4);
  for (unsigned i = 0; i < sizeof(D_800B1F78); i++)
    D_800B1F78[i] = (i * 31 + 123) & 255;
  if (nat) {
    g_FieldActors = (FieldActor *)actors;
    q = (unsigned)(uintptr_t)data;
    memcpy(actors + 0x4c, &q, 4);
    if (!setjmp(stop))
      func_80076AC0(0, slot, (void *)0x123456, mode, page, selector, flag);
    memcpy(result.bytes, data, sizeof(data));
  } else {
    memcpy(ram + (ENTRY & 0x1fffff), code, sizeof(code));
    memcpy(ram + (BASE & 0x1fffff), actors, sizeof(actors));
    memcpy(ram + (DATA & 0x1fffff), data, sizeof(data));
    wr(BASE + 0x4c, DATA);
    wr(0x800afb10, BASE);
    memcpy(ram + 0xb1f78, D_800B1F78, sizeof(D_800B1F78));
    PcPortMipsBus b = {0};
    PcPortMipsCpu c;
    b.read = readbus;
    b.write = writebus;
    b.bridge = bridge;
    PcPortMipsCpuInit(&c, &b);
    c.gpr[4] = 0;
    c.gpr[5] = slot;
    c.gpr[6] = 0x123456;
    c.gpr[7] = mode;
    c.gpr[29] = STACK;
    c.gpr[31] = STOP;
    wr(STACK + 16, page);
    wr(STACK + 20, selector);
    wr(STACK + 24, flag);
    int rc = PcPortMipsRun(&c, ENTRY, STOP, 2000);
    if (rc != PC_PORT_MIPS_HALTED)
      fprintf(stderr, "%s pc=%x\n", c.error, c.pc);
    check(rc == PC_PORT_MIPS_HALTED);
    memcpy(result.bytes, ram + (DATA & 0x1fffff), sizeof(data));
  }
  check(result.calls == 1 && result.id && result.freed == (unsigned)flag);
  return result;
}
int main(int argc, char **argv) {
  check(argc == 2);
  FILE *f = fopen(argv[1], "rb");
  check(f != NULL);
  check(!fseek(f, 0x6fd0, SEEK_SET));
  check(fread(code, 1, sizeof(code), f) == sizeof(code));
  fclose(f);
  int pages[] = {0, 1, 3, 15, 16, 127, 255};
  int modes[] = {0, 1, 2, 3};
  for (int selector = 0; selector < 256; selector++)
    for (unsigned m = 0; m < 4; m++)
      for (unsigned p = 0; p < 7; p++)
        for (int flag = 0; flag < 2; flag++) {
          int slot = selector;
          struct Result a = run(0, slot, modes[m], pages[p], selector, flag),
                        b = run(1, slot, modes[m], pages[p], selector, flag);
          check(!memcmp(&a, &b, sizeof(a)));
          cases++;
        }
  printf("BIND ARGS PASS cases=%u allocation boundary only\n", cases);
}
