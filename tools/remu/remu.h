/* remu: tiny MIPS-I retail-byte emulator. See remu.c for semantics. */
#ifndef REMU_H
#define REMU_H
#include <stdint.h>

typedef struct remu remu_t;

remu_t *remu_create(void);
void remu_destroy(remu_t *m);
void remu_set_reg(remu_t *m, int r, uint32_t v);
uint32_t remu_get_reg(remu_t *m, int r);
/* Seed/extract emulated RAM. Returns 0 ok, -1 out of range. */
int remu_poke(remu_t *m, uint32_t addr, const void *data, uint32_t len);
int remu_peek(remu_t *m, uint32_t addr, void *data, uint32_t len);
/* Load a splat .s function file's retail bytes; returns entry vram (0 fail).
 * May be called repeatedly to pre-load callee .s files (ranges accumulate). */
uint32_t remu_load_s(remu_t *m, const char *path);
/* Run from entry with a0..a3. Return codes: 0 returned, 1 syscall-stop,
 * 2 break-stop, 3 uninit-read (halted at first touch), -1 step budget,
 * -2 mem fault, -3 unknown opcode, -4 cop1/2/3. */
int remu_call(remu_t *m, uint32_t entry, uint32_t a0, uint32_t a1,
              uint32_t a2, uint32_t a3);
/* External (unloaded) jal targets hit during the last call. */
int remu_stub_calls(remu_t *m);
const char *remu_stub_log(remu_t *m);

#endif
