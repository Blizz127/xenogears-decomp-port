#include <stdint.h>
#include <string.h>

void FieldSetMousePosition(int selector, int x, int y)
{
    const volatile u_short *speed_x = &g_FieldMouseSpeedX;
    const volatile u_short *speed_y = &g_FieldMouseSpeedY;
    uint32_t scaled_x = (uint32_t)x * *speed_x;
    uint32_t scaled_y = (uint32_t)y * *speed_y;

    /* The original multiplies the selector by four in one 32-bit word. */
    uint32_t port = (uint32_t)selector & UINT32_C(0x3fffffff);
    if (port >= 2)
        return;

    int x_word, y_word;
    memcpy(&x_word, &scaled_x, sizeof(x_word));
    memcpy(&y_word, &scaled_y, sizeof(y_word));
    volatile int *positions_x = g_FieldMousePositionsX;
    volatile int *positions_y = g_FieldMousePositionsY;
    positions_x[port] = x_word;
    positions_y[port] = y_word;
}
