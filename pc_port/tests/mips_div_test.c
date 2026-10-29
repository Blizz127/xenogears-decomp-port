/* XenoMipsDiv (include/common.h): R3000 DIV results where x86 would trap.
 * run_mips_div_test.sh extracts the helper from common.h, so this checks the
 * shipped definition. */
#include <stdio.h>
#include "mips_div_helper.inc"
static int fails;
static void check(int a, int b, int want) {
    int got = XenoMipsDiv(a, b);
    if (got != want) { printf("FAIL %d / %d = %d, want %d\n", a, b, got, want); fails++; }
}
int main(void) {
    check(-0x10000, 0, 1);          /* FieldFadeToBlack with duration 0 */
    check(0xFF00, 0, -1);           /* FieldFadeToWhite with duration 0 */
    check(0, 0, -1);
    check((int)0x80000000, -1, (int)0x80000000);
    check(-0x10000, 16, -0x1000);
    check(0xFF00, 30, 0xFF00 / 30);
    check(-7, 2, -3);               /* C and R3000 both truncate toward zero */
    printf("mips_div_test: %s (%d failures)\n", fails ? "FAIL" : "PASS", fails);
    return fails != 0;
}
