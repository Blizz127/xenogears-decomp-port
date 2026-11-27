#ifndef XENO_MEMBER_AND_SHOP_NUMERIC_DIGITS_H
#define XENO_MEMBER_AND_SHOP_NUMERIC_DIGITS_H

static void menuNumericDigits(unsigned int value)
{
    volatile u_char *digits = g_Menu->digits;
    unsigned int divisor = 100000000u;
    for (unsigned int i = 0; i < 9; ++i) {
        digits[i] = (u_char)(value / divisor);
        value %= divisor;
        divisor /= 10u;
    }
    for (unsigned int i = 1; i < 9; ++i) {
        if (digits[i] != 0) {
            if (digits[i - 1] == 0)
                digits[i - 1] = 255;
            break;
        }
        digits[i - 1] = 255;
    }
}
#endif
