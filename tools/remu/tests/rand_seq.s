/* Programmed `rand` for remu multi-load tests: returns a per-boot
 * counter (low byte), incrementing retail scratch at 0x801F0200.
 * Install this test routine at the guest libc rand entry (0x8003FA38).
 * The host test resets that word per seed and mirrors the counter, so
 * both sides observe the identical deterministic sequence. This stands
 * in for the real libc rand, which has no .s to load (remu would stub
 * it to a constant 0, which cannot reach every path of callers like
 * func_8007D478).
 *
 * Hand-encoded MIPS. NOTE the remu comment convention: the hex field is
 * the ROM byte sequence in display (memory) order, i.e. the little-
 * endian bytes of the word, NOT a plain %08X print. E.g. lui is word
 * 0x3C01801F, bytes 1F 80 01 3C on the wire.
 *   lui  $at, 0x801F        word 3C01801F
 *   lw   $v0, 0x0200($at)   word 8C220200
 *   addiu $v1, $v0, 1       word 24430001
 *   sw   $v1, 0x0200($at)   word AC230200
 *   andi $v0, $v0, 0xFF     word 304200FF
 *   jr   $ra                word 03E00008
 *   nop                     word 00000000
 */
glabel rand
    /* 0 8003FA38 1F80013C */  lui        $at, 0x801F
    /* 4 8003FA3C 0002228C */  lw         $v0, 0x0200($at)
    /* 8 8003FA40 01004324 */  addiu      $v1, $v0, 1
    /* C 8003FA44 000223AC */  sw         $v1, 0x0200($at)
    /* 10 8003FA48 FF004230 */  andi      $v0, $v0, 0xFF
    /* 14 8003FA4C 0800E003 */  jr        $ra
    /* 18 8003FA50 00000000 */   nop
.size rand, . - rand
