/* Test-only logging stub for func_80076A10 (real body belongs to
 * main4.c, untranscribed). Records the call count + a0..a3 of up to 3
 * calls into scratch 0x80070000 (test pokes zeros first; count lives at
 * 0x20, call i args at 0x24+16*i). Returns a table-driven nonzero value
 * (table of 3 words at 0x80070060, poked by the test: 0x55, 0xAA, 0xFF)
 * so the caller's add-back path (incl. the 0xFF+0xFF wrap) is exercised
 * identically on both sides. Opcodes generated mechanically (bit-field
 * construction, cross-checked by the test's stub-image assert); comment
 * order is byte-sequence. */
glabel func_80076A10
    /* 70A10 80076A10 0780013C */  lui $at, 0x8007         
    /* 70A14 80076A14 2000288C */  lw $t0, 0x20($at)       
    /* 70A18 80076A18 01000325 */  addiu $v1, $t0, 1       
    /* 70A1C 80076A1C 200023AC */  sw $v1, 0x20($at)       
    /* 70A20 80076A20 80180800 */  sll $v1, $t0, 2         
    /* 70A24 80076A24 21186100 */  addu $v1, $v1, $at      
    /* 70A28 80076A28 6000628C */  lw $v0, 0x60($v1)       
    /* 70A2C 80076A2C 00410800 */  sll $t0, $t0, 4         
    /* 70A30 80076A30 21082800 */  addu $at, $at, $t0      
    /* 70A34 80076A34 240024AC */  sw $a0, 0x24($at)       
    /* 70A38 80076A38 280025AC */  sw $a1, 0x28($at)       
    /* 70A3C 80076A3C 2C0026AC */  sw $a2, 0x2C($at)       
    /* 70A40 80076A40 300027AC */  sw $a3, 0x30($at)       
    /* 70A44 80076A44 0800E003 */  jr $ra                  
    /* 70A48 80076A48 00000000 */  nop                     
.size func_80076A10, . - func_80076A10
