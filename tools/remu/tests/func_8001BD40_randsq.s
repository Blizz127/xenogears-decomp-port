/* Patched test-local copy of asm/slus_006.64/matchings/system/temp3/func_8001BD40.s:
 * the two jal-rand words (8EFE000C -> 0x8003FA38, unmapped libc) are
 * retargeted to the programmed counter in tools/remu/tests/rand_seq.s
 * (C0C0070C -> 0x801F0300). All other words are byte-identical.
 */
glabel func_8001BD40
    /* C540 8001BD40 E0FFBD27 */  addiu      $sp, $sp, -0x20
    /* C544 8001BD44 1400B1AF */  sw         $s1, 0x14($sp)
    /* C548 8001BD48 21888000 */  addu       $s1, $a0, $zero
    /* C54C 8001BD4C FF002332 */  andi       $v1, $s1, 0xFF
    /* C550 8001BD50 FF000234 */  ori        $v0, $zero, 0xFF
    /* C554 8001BD54 1800BFAF */  sw         $ra, 0x18($sp)
    /* C558 8001BD58 1A006210 */  beq        $v1, $v0, .L8001BDC4
    /* C55C 8001BD5C 1000B0AF */   sw        $s0, 0x10($sp)
    /* C560 8001BD60 FF00A230 */  andi       $v0, $a1, 0xFF
    /* C564 8001BD64 03004014 */  bnez       $v0, .L8001BD74
    /* C568 8001BD68 00000000 */   nop
    /* C56C 8001BD6C 716F0008 */  j          .L8001BDC4
    /* C570 8001BD70 21100000 */   addu      $v0, $zero, $zero
  .L8001BD74:
    /* C574 8001BD74 03006214 */  bne        $v1, $v0, .L8001BD84
    /* C578 8001BD78 23804300 */   subu      $s0, $v0, $v1
    /* C57C 8001BD7C 716F0008 */  j          .L8001BDC4
    /* C580 8001BD80 21106000 */   addu      $v0, $v1, $zero
  .L8001BD84:
    /* C584 8001BD84 FF00022A */  slti       $v0, $s0, 0xFF
    /* C588 8001BD88 0B004010 */  beqz       $v0, .L8001BDB8
    /* C58C 8001BD8C 00000000 */   nop
    /* C590 8001BD90 C0C0070C */  jal        rand
    /* C594 8001BD94 00000000 */   nop
    /* C598 8001BD98 FF004230 */  andi       $v0, $v0, 0xFF
    /* C59C 8001BD9C 01000326 */  addiu      $v1, $s0, 0x1
    /* C5A0 8001BDA0 1A004300 */  div        $zero, $v0, $v1
    /* C5A4 8001BDA4 10180000 */  mfhi       $v1
    /* C5A8 8001BDA8 00000000 */  nop
    /* C5AC 8001BDAC 21182302 */  addu       $v1, $s1, $v1
    /* C5B0 8001BDB0 716F0008 */  j          .L8001BDC4
    /* C5B4 8001BDB4 FF006230 */   andi      $v0, $v1, 0xFF
  .L8001BDB8:
    /* C5B8 8001BDB8 C0C0070C */  jal        rand
    /* C5BC 8001BDBC 00000000 */   nop
    /* C5C0 8001BDC0 FF004230 */  andi       $v0, $v0, 0xFF
  .L8001BDC4:
    /* C5C4 8001BDC4 1800BF8F */  lw         $ra, 0x18($sp)
    /* C5C8 8001BDC8 1400B18F */  lw         $s1, 0x14($sp)
    /* C5CC 8001BDCC 1000B08F */  lw         $s0, 0x10($sp)
    /* C5D0 8001BDD0 2000BD27 */  addiu      $sp, $sp, 0x20
    /* C5D4 8001BDD4 0800E003 */  jr         $ra
    /* C5D8 8001BDD8 00000000 */   nop
.size func_8001BD40, . - func_8001BD40
