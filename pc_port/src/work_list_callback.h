#ifndef XENO_WORK_LIST_CALLBACK_H
#define XENO_WORK_LIST_CALLBACK_H
#include <stdint.h>

/* Invoke a callback saved from a packed native work-list task. callback is
 * the task's unchanged 32-bit code address; argument is the native task.
 * Native code lives below 4 GiB. Guest addresses go through the existing
 * active-battle dispatcher and unresolved guest addresses abort, never
 * becoming native indirect calls. No ownership, retry, or null fallback. */
void PcPort_WorkListInvokeSavedCallback(uint32_t callback, void* argument);
#endif
