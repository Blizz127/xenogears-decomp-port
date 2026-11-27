#ifndef XENO_HOST_THREAD_H
#define XENO_HOST_THREAD_H

#ifdef _WIN32
#include <process.h>
#include <windows.h>

typedef HANDLE HostThread;
typedef SRWLOCK HostMutex;
#define HOST_MUTEX_INITIALIZER SRWLOCK_INIT
#define HOST_THREAD_RESULT unsigned __stdcall
static inline void host_mutex_lock(HostMutex* mutex) { AcquireSRWLockExclusive(mutex); }
static inline void host_mutex_unlock(HostMutex* mutex) { ReleaseSRWLockExclusive(mutex); }

static inline int host_thread_start(HostThread* thread, unsigned (__stdcall *run)(void*), void* arg)
{
    *thread = (HANDLE)_beginthreadex(NULL, 0, run, arg, 0, NULL);
    return *thread ? 0 : -1;
}
static inline void host_thread_detach(HostThread thread) { CloseHandle(thread); }
static inline void host_thread_join(HostThread thread)
{
    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread);
}
#else
#include <pthread.h>
typedef pthread_t HostThread;
typedef pthread_mutex_t HostMutex;
#define HOST_MUTEX_INITIALIZER PTHREAD_MUTEX_INITIALIZER
#define HOST_THREAD_RESULT void*
static inline void host_mutex_lock(HostMutex* mutex) { pthread_mutex_lock(mutex); }
static inline void host_mutex_unlock(HostMutex* mutex) { pthread_mutex_unlock(mutex); }
static inline int host_thread_start(HostThread* thread, void* (*run)(void*), void* arg)
{
    return pthread_create(thread, NULL, run, arg);
}
static inline void host_thread_detach(HostThread thread) { pthread_detach(thread); }
static inline void host_thread_join(HostThread thread) { pthread_join(thread, NULL); }
#endif

#endif
