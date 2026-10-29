/* Emits movie_retail_layout.h: host layout of the production MovieRenderEnv
 * (src/movie/main.c) so the retail-differential test never hard-codes it.
 * main.c is included whole; the generator never calls into it (the link
 * ignores its unresolved externs).  Retail layout (asm/movie/9F8.s): draw 0x00
 * (0x5C), disp 0x5C (0x14), ot 0x70 (32 x u32), box 0xF0, frame 0x114,
 * sizeof 0x138. */
#include "../../src/movie/main.c"
#undef offsetof
#define offsetof(s, m) __builtin_offsetof(s, m)
#define P(m) printf("#define MVH_%s 0x%zxu\n", #m, (size_t)offsetof(MovieRenderEnv, m))
int main(void) {
    printf("/* generated */\n");
    printf("#define MVH_ENV_SIZE 0x%zxu\n#define MVH_ULONG %zu\n", sizeof(MovieRenderEnv), sizeof(u_long));
    printf("#define MVH_DRAW_SIZE 0x%zxu\n#define MVH_DISP_SIZE 0x%zxu\n#define MVH_POLY_SIZE 0x%zxu\n",
           sizeof(DRAWENV), sizeof(DISPENV), sizeof(POLY_G4));
    P(draw); P(disp); P(ot); P(box); P(frame);
    printf("#define MVH_DRAW_ISBG 0x%zxu\n#define MVH_DISP_ISRGB24 0x%zxu\n",
           offsetof(DRAWENV, isbg), offsetof(DISPENV, isrgb24));
    return 0;
}
