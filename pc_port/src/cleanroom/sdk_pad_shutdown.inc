/* PsyCross owns the pad communication flag tested by the SDL poller. */
extern void PadStopCom(void);

void func_800408F4(void)
{
    PadStopCom();
}
