/* Called on the game thread. Establish the clock on first use, even if
 * its first client is unregistering. Subsequent changes preserve its epoch. */
void func_8004B7D0(void (*handler)(void))
{
    initialize();
    callback = handler;
}
