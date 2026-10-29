/* Isolation test for the generic host-call ABI mismatch in
 * battle_mips_runtime.c.
 *
 * runtime_bridge_call's generic path calls every resolved host function
 * through GenericHostFunction (twelve uintptr_t in, uintptr_t out), whatever
 * its real prototype.  That is undefined behaviour in C.  On SysV x86-64 it
 * happens to work for integer/pointer arguments, but clang's
 * -fsanitize=function rejects it at runtime.  GCC has no function sanitizer,
 * so the project's gcc UBSan regime never sees it.
 *
 * The typedef is copied out of the production source by the runner (which
 * fails if its shape changes), so this tracks the real boundary without
 * linking the runtime.  Modes:
 *   mismatch  call a HeapChangeCurrentUser-shaped void(int, void *) and an
 *             int(int) through the generic type, as production does;
 *   typed     the same two calls through their own types (control).
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "generic_host_function.h"

static int user_calls;

__attribute__((noinline)) void HostChangeUser(int user, void *owner)
{
    user_calls += user + (owner == NULL);
}

__attribute__((noinline)) int HostNegate(int value)
{
    return -value;
}

int main(int argc, char **argv)
{
    uintptr_t args[12] = {2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    int result;

    if (argc != 2)
        return 2;
    if (strcmp(argv[1], "typed") == 0) {
        HostChangeUser((int)args[0], (void *)args[1]);
        result = HostNegate(5);
    } else if (strcmp(argv[1], "mismatch") == 0) {
        GenericHostFunction function = (GenericHostFunction)(void (*)(void))HostChangeUser;
        function(args[0], args[1], args[2], args[3], args[4], args[5],
                 args[6], args[7], args[8], args[9], args[10], args[11]);
        function = (GenericHostFunction)(void (*)(void))HostNegate;
        args[0] = 5;
        result = (int)(uint32_t)function(args[0], args[1], args[2], args[3],
                                         args[4], args[5], args[6], args[7],
                                         args[8], args[9], args[10], args[11]);
    } else {
        return 2;
    }
    printf("GENERIC HOST CALL %s user_calls=%d result=%d\n", argv[1], user_calls, result);
    return user_calls == 3 && result == -5 ? 0 : 1;
}
