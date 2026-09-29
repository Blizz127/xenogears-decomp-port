/* Test-only logging stub for func_800BE538 (real body belongs to
 * mainc122.c and has a deep subtree with a polling loop). Records the
 * call count + a0..a3 into scratch 0x80070000 (test pokes zeros first).
 * Opcodes assembled programmatically (see session notes) and verified
 * by field-decode + dump in the test; comment order is byte-sequence. */
glabel func_800BE538
    /* 4E948 800BE538 0780013C */  lui        $at, 0x8007
    /* 4E94C 800BE53C 0000228C */  lw         $v0, 0x0($at)
    /* 4E950 800BE540 01004224 */  addiu      $v0, $v0, 1
    /* 4E954 800BE544 000022AC */  sw         $v0, 0x0($at)
    /* 4E958 800BE548 040024AC */  sw         $a0, 0x4($at)
    /* 4E95C 800BE54C 080025AC */  sw         $a1, 0x8($at)
    /* 4E960 800BE550 0C0026AC */  sw         $a2, 0xC($at)
    /* 4E964 800BE554 100027AC */  sw         $a3, 0x10($at)
    /* 4E968 800BE558 0800E003 */  jr         $ra
    /* 4E96C 800BE55C 00000000 */  nop
.size func_800BE538, . - func_800BE538
