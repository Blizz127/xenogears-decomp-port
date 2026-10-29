/* Fail-visible report for the retail fatal error handler GameHandleError
 * (src/slus_006.64/main/main_loop.c).
 *
 * Retail answers a fatal system error (heap misuse, missing overlay, ...) by
 * clearing the screen red and spinning forever, or by looping on an error text
 * screen.  The port keeps that behaviour exactly: this only writes one stderr
 * line with the error code and the host call chain before the spin, because
 * the retail "From %08x" address is a MIPS $ra that the host cannot capture
 * (it is 0 here), and a silent spin is indistinguishable from a hang. */
#include <stdio.h>

#if defined(__GLIBC__)
#include <execinfo.h>
#include <unistd.h>
#endif

extern unsigned int g_HeapLastAllocSize;
extern unsigned int g_HeapLastAllocSrcAddr;

void PcPort_ReportGameError(unsigned int errorCode, unsigned int sourceAddress)
{
    static int reported;

    if (reported)
        return;
    reported = 1;
    fprintf(stderr,
            "[xeno-port] GameHandleError code=0x%X (%u) source=%08x "
            "heapLastSize=%u heapLastSrc=%08x -- retail halts here (spin); "
            "host call chain follows\n",
            errorCode, errorCode, sourceAddress, g_HeapLastAllocSize,
            g_HeapLastAllocSrcAddr);
#if defined(__GLIBC__)
    {
        void* frames[32];
        int n = backtrace(frames, 32);

        backtrace_symbols_fd(frames, n, 2);
    }
#endif
    fflush(stderr);
}
