/* The host serves card requests synchronously from its file-backed slots.
 * These hooks control the console's shared controller/card interrupt service.
 * On the host there is no interrupt worker to start or drain. Lifecycle calls
 * leave inserted images, directory searches and open file cursors alone.
 */
void InitCARD(int pad_enable)
{
    (void)pad_enable;
}

int StartCARD(void)
{
    return 1;
}

int StopCARD(void)
{
    return 1;
}
