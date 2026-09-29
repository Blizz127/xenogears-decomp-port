#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "krom_rom.h"
static uint8_t bios[0x80001];
static int fail_alloc;
static int read_fault, close_fault;
static void *owned[4];
static unsigned live;
void *__real_malloc(size_t);
void *__wrap_malloc(size_t n) {
    if(fail_alloc){fail_alloc=0;return NULL;}
    void *p=__real_malloc(n);assert(p && live<4);owned[live++]=p;return p;
}
void __real_free(void*);
void __wrap_free(void *p) {
    if(p){unsigned i=0;while(i<live && owned[i]!=p)++i;assert(i<live);owned[i]=owned[--live];}
    __real_free(p);
}
ssize_t __real_read(int,void*,size_t);
ssize_t __wrap_read(int fd,void *p,size_t n) {
    int fault=read_fault;
    if(fault==5 && n!=1)return __real_read(fd,p,n);
    read_fault=0;
    if(fault==1){errno=EIO;return -1;}
    if(fault==2)return 0;
    if(fault==3){errno=EINTR;return -1;}
    if(fault==4)return __real_read(fd,p,7);
    if(fault==5){*(uint8_t*)p=0;return 1;}
    return __real_read(fd,p,n);
}
/* Ubuntu's fortified O2 build replaces the bulk read with __read_chk. */
ssize_t __wrap___read_chk(int fd,void *p,size_t n,size_t capacity) {
    assert(n<=capacity);return __wrap_read(fd,p,n);
}
int __real_close(int);
int __wrap_close(int fd) {
    int r=__real_close(fd);
    if(close_fault){close_fault=0;errno=EIO;return -1;}return r;
}
static void save(const char *p,size_t n) {
    FILE *f=fopen(p,"wb");assert(f);assert(fwrite(bios,1,n,f)==n);assert(fclose(f)==0);
}
static unsigned checks;
static void reject(const char *p,PcPortKromRomResult expected) {
    PcPortKromRom *rom=(void*)(uintptr_t)1;
    PcPortKromRomResult got=PcPortKromRomLoad(p,&rom);
    if(got!=expected)fprintf(stderr,"LOAD DIFF check=%u got=%d expected=%d read_fault=%d close_fault=%d\n",checks,got,expected,read_fault,close_fault);
    assert(got==expected);assert(rom==NULL && live==0);++checks;
}
int main(int argc,char **argv) {
    assert(argc==2);const char *path=argv[1];
    FILE *f=fopen("disc/scph5500.bin","rb");assert(f);
    assert(fread(bios,1,0x80000,f)==0x80000 && fgetc(f)==EOF);fclose(f);
    reject(NULL,PC_PORT_KROM_ROM_IO);
    reject(path,PC_PORT_KROM_ROM_IO);
    reject("disc",PC_PORT_KROM_ROM_IO);
    assert(mkfifo(path,0600)==0);reject(path,PC_PORT_KROM_ROM_IO);assert(unlink(path)==0);
    const size_t sizes[]={0,1,0x7ffff,0x80001};
    for(unsigned i=0;i<4;++i){save(path,sizes[i]);reject(path,PC_PORT_KROM_ROM_SIZE);}
    for(unsigned i=0;i<0x80000;i+=4096) {
        bios[i]^=1;save(path,0x80000);reject(path,PC_PORT_KROM_ROM_HASH);bios[i]^=1;
    }
    bios[0x7ffff]^=128;save(path,0x80000);reject(path,PC_PORT_KROM_ROM_HASH);bios[0x7ffff]^=128;
    save(path,0x80000);fail_alloc=1;reject(path,PC_PORT_KROM_ROM_MEMORY);assert(!fail_alloc);
    read_fault=1;reject(path,PC_PORT_KROM_ROM_IO);assert(!read_fault);
    read_fault=2;reject(path,PC_PORT_KROM_ROM_IO);assert(!read_fault);
    read_fault=5;reject(path,PC_PORT_KROM_ROM_SIZE);assert(!read_fault);
    close_fault=1;reject(path,PC_PORT_KROM_ROM_IO);assert(!close_fault);
    PcPortKromRom *rom,*other;
    for(int fault=3;fault<=4;++fault) {
        read_fault=fault;assert(PcPortKromRomLoad(path,&rom)==PC_PORT_KROM_ROM_OK);
        assert(!read_fault);PcPortKromRomFree(rom);++checks;
    }
    assert(PcPortKromRomLoad(path,&rom)==PC_PORT_KROM_ROM_OK);
    assert(PcPortKromRomLoad(path,&other)==PC_PORT_KROM_ROM_OK && rom!=other);
    /* File changes after verification cannot alter either owned copy. */
    bios[0x66000]^=255;save(path,0x80000);bios[0x66000]^=255;
    assert(unlink(path)==0);
    for(unsigned code=0;code<65536;++code) {
        const uint8_t *expected,*actual;
        PcPortKromResult r=PcPortKromResolveRomSpan(bios,code,NULL,32,&expected);
        assert(PcPortKromRomResolve(rom,code,NULL,32,&actual)==r);
        if(r==PC_PORT_KROM_OK){assert(actual!=expected && memcmp(actual,expected,32)==0);}
        else assert(actual==NULL);
        ++checks;
    }
    for(unsigned s=0;s<256;++s) {
        uint8_t selector=s;const uint8_t *expected,*actual;
        PcPortKromResult r=PcPortKromResolveRomSpan(bios,0x817f,&selector,32,&expected);
        assert(PcPortKromRomResolve(rom,0x817f,&selector,32,&actual)==r);
        if(r==PC_PORT_KROM_OK)assert(memcmp(actual,expected,32)==0);
        else assert(actual==NULL);
        ++checks;
    }
    PcPortKromRomFree(rom);const uint8_t *span;
    assert(PcPortKromRomResolve(other,0x8140,NULL,32,&span)==PC_PORT_KROM_OK);
    assert(memcmp(span,bios+0x66000,32)==0);
    PcPortKromRomFree(other);PcPortKromRomFree(NULL);assert(live==0);
    printf("PASS %u ROM load/resolve checks; owned copy and independent handles\n",checks);
}
