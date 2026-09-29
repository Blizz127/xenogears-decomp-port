#include <assert.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include "krom_rom.h"
static uint8_t expected[32];
static const uint8_t *results[8];
static void *worker(void *arg) {
    unsigned i=(uintptr_t)arg;
    results[i]=PcPortKromFont(0x8140,32);
    assert(memcmp(results[i],expected,32)==0);return NULL;
}
static void abort_case(const char *path,uint32_t code,size_t length) {
    pid_t pid=fork();assert(pid>=0);
    if(pid==0) {
        assert(setenv("XENO_BIOS",path,1)==0);
        PcPortKromFont(code,length);_exit(42);
    }
    int status;assert(waitpid(pid,&status,0)==pid);
    assert(WIFSIGNALED(status) && WTERMSIG(status)==SIGABRT);
}
int main(void) {
    FILE *f=fopen("disc/scph5500.bin","rb");assert(f);
    assert(fseek(f,0x66000,SEEK_SET)==0 && fread(expected,1,32,f)==32);fclose(f);
    abort_case("/nonexistent/xeno-krom.bin",0x8140,32);
    abort_case("disc/menu.bin",0x8140,32);
    abort_case("disc/scph5500.bin",0x817f,32);
    abort_case("disc/scph5500.bin",0x8140,SIZE_MAX);
    assert(unsetenv("XENO_BIOS")==0);
    pthread_t threads[8];
    for(unsigned i=0;i<8;++i)assert(pthread_create(&threads[i],NULL,worker,(void*)(uintptr_t)i)==0);
    for(unsigned i=0;i<8;++i)assert(pthread_join(threads[i],NULL)==0 && results[i]==results[0]);
    assert((uintptr_t)results[0]>UINT32_MAX);
    /* Once-owned ROM, not a reload from a changed path on each glyph. */
    assert(setenv("XENO_BIOS","/nonexistent/changed-after-init.bin",1)==0);
    assert(PcPortKromFont(0x12348140,32)==results[0]);
    assert(PcPortKromFont(0,32)==(const uint8_t*)(intptr_t)-1);
    assert(PcPortKromFont(0xffff,32)==(const uint8_t*)(intptr_t)-1);
    puts("PASS native KROM font: concurrent init, full-width ROM span, rejection and four unresolved boundaries");
}
