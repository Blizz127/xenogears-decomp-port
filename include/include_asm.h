#ifndef INCLUDE_ASM_H
#define INCLUDE_ASM_H

/* SKIP_ASM makes INCLUDE_ASM/INCLUDE_RODATA expand to nothing, so a TU compiles
 * to *only* the C that has actually been decompiled.
 *
 * This is what makes the objdiff progress report honest. `gears report` passes
 * -DSKIP_ASM precisely so that unmatched functions are ABSENT from the compared
 * object; without it the macro pastes the retail .s back in, every unmatched
 * function comes out byte-identical to the target, and the report reads ~100%
 * matched no matter how little C exists. Report mode does not link (see
 * tools/gears/src/main.rs, `if args.mode != Mode::Report`), so the resulting
 * holes are harmless there.
 *
 * VERIFIED: adding SKIP_ASM to this guard leaves the matching build bit-for-bit
 * identical (field.bin e91869555a1b88d6 / battle.bin 1830b4ef1fe37129 with and
 * without it) -- matching mode never defines SKIP_ASM. The port already passes
 * -DSKIP_ASM, but every INCLUDE_ASM site it compiles sits inside a
 * `#ifndef XENO_PC_PORT` branch, so it is unaffected too. */
#if !defined(M2CTX) && !defined(PERMUTER) && !defined(SKIP_ASM)

#ifndef INCLUDE_ASM
/* The asm is wrapped in a dummy function that maspsx strips (only the
 * `# maspsx-keep` lines survive). This keeps each INCLUDE_ASM in source order
 * relative to the C functions: at -G8 cc1 defers every function body to the
 * end of the TU, while a bare top-level __asm__ is emitted immediately, so a
 * top-level INCLUDE_ASM would be hoisted ahead of all C in its TU. */
#define INCLUDE_ASM(FOLDER, NAME) \
    void __maspsx_include_asm_hack_##NAME() { \
        __asm__( \
            ".text # maspsx-keep\n" \
            "\t.align\t2 # maspsx-keep\n" \
            "\t.set noreorder # maspsx-keep\n" \
            "\t.set noat # maspsx-keep\n" \
            ".include \"" FOLDER "/" #NAME ".s\" # maspsx-keep\n" \
            "\t.set reorder # maspsx-keep\n" \
            "\t.set at # maspsx-keep\n" \
        ); \
    }
#endif
#ifndef INCLUDE_RODATA
#define INCLUDE_RODATA(FOLDER, NAME) \
    __asm__( \
        ".section .rodata\n" \
        "    .include \"" FOLDER "/" #NAME ".s\"\n" \
        ".section .text" \
    )
#endif

#if INCLUDE_ASM_USE_MACRO_INC
__asm__(".include \"include/macro.inc\"\n");
#else
__asm__(".include \"include/labels.inc\"\n");
#endif

#else

#ifndef INCLUDE_ASM
#define INCLUDE_ASM(FOLDER, NAME)
#endif
#ifndef INCLUDE_RODATA
#define INCLUDE_RODATA(FOLDER, NAME)
#endif

#endif /* !defined(M2CTX) && !defined(PERMUTER) && !defined(SKIP_ASM) */

#endif /* INCLUDE_ASM_H */
