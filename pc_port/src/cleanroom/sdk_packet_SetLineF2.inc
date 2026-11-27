/* PsyCross e56e4cde, include/psx/libgpu.h (MIT). */
void SetLineF2(LINE_F2 *packet)
{
    /* The port's patched lowercase macro differs from the MIT three-word packet. */
    setlen(packet, 3);
    setcode(packet, 0x40);
}
