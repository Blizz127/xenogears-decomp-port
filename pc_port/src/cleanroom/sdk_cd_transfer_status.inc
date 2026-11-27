/* PsyCross copies each requested sector slice to its destination inside
 * CdGetSector. There is no outstanding sector DMA to wait for or poll. */
int CdDataSync(int wait_mode)
{
    (void)wait_mode;
    return 0;
}
