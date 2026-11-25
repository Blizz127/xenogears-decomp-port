#define _DEFAULT_SOURCE
#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>

typedef uint16_t u16;
typedef uint32_t u32;
typedef void (*SoundCommandCallback_t)(void);
typedef void Submit(u32,void *,u_long,SoundCommandCallback_t);
typedef void Empty(void);
void SoundQueueTransferCommand(u32,void *,u_long,SoundCommandCallback_t,u16);
Submit SoundQueueSpuReadCommand, func_8003BC58, func_8003BC7C;
Empty SoundUnkDebugNoReturn_8003F4BC, func_80039D24, func_8003A2D4, func_8003A2DC, func_8003BDF4;
int func_8003977C(void);
extern volatile u32 fixture_effect;

#ifdef BUILD_COMMANDS
#ifndef TRANSFER_FRAGMENT
#define TRANSFER_FRAGMENT "../../src/cleanroom/sound_transfer_reserved_ops.inc"
#endif
#include TRANSFER_FRAGMENT
#else
volatile u32 fixture_effect=0x45235a11;
static Submit *const senders[]={SoundQueueSpuReadCommand,func_8003BC58,func_8003BC7C};
static Empty *const empty[]={SoundUnkDebugNoReturn_8003F4BC,func_80039D24,func_8003A2D4,func_8003A2DC,func_8003BDF4};
static struct { u32 address; void *data; u_long size; SoundCommandCallback_t cb; u16 tag; } wanted;
static unsigned submissions, callback_invocations, active, host_calls;
static unsigned char writable[64], prior[64];
static int allow_service_write;
static void callback_a(void) { callback_invocations+=1; }
static void callback_b(void) { callback_invocations+=2; }

#ifdef OBSERVE_ACCESSES
static unsigned entries, exits;
static const void *expected_function;
void __cyg_profile_func_enter(void *fn,void *site)
{
    (void)site;
    assert(active && fn==expected_function && entries==0);++entries;
}
void __cyg_profile_func_exit(void *fn,void *site)
{
    (void)site;
    assert(active && fn==expected_function && entries==1 && exits==0);++exits;
}
void __tsan_init(void) {}
void __tsan_func_entry(void *p) { (void)p; }
void __tsan_func_exit(void) {}
#define NO_ACCESS(name,n) void __tsan_##name##n(void *p) { (void)p; abort(); }
#define NO_WIDTH(n) NO_ACCESS(read,n) NO_ACCESS(write,n) NO_ACCESS(volatile_read,n) NO_ACCESS(volatile_write,n)
NO_WIDTH(1) NO_WIDTH(2) NO_WIDTH(4) NO_WIDTH(8) NO_WIDTH(16)
#endif

void SoundQueueTransferCommand(u32 address,void *data,u_long size,SoundCommandCallback_t cb,u16 tag)
{
    assert(active && wanted.tag && submissions==0 && tag==wanted.tag);
    assert(address==wanted.address && data==wanted.data && size==wanted.size && cb==wanted.cb);
    assert(!memcmp(writable,prior,sizeof writable));
    ++submissions;
    if(allow_service_write && data==writable+3) writable[3]^=0xb6;
}

static void execute(unsigned id,u32 address,void *data,u_long size,SoundCommandCallback_t cb,u16 tag)
{
    assert(id<9 && fixture_effect==0x45235a11u);
    memcpy(prior,writable,sizeof writable);
    wanted.address=address; wanted.data=data; wanted.size=size; wanted.cb=cb; wanted.tag=tag;
    active=1;submissions=0;
#ifdef OBSERVE_ACCESSES
    entries=exits=0;
    if(id<3) expected_function=(const void *)senders[id];
    else if(id<8) expected_function=(const void *)empty[id-3];
    else expected_function=(const void *)func_8003977C;
#endif
    if(id<3) senders[id](address,data,size,cb);
    else if(id<8) empty[id-3]();
    else assert(func_8003977C()==0);
    active=0;
    assert(submissions==(unsigned)(id<3) && callback_invocations==0 && fixture_effect==0x45235a11u);
    if(id<3 && allow_service_write && data==writable+3) prior[3]^=0xb6;
    assert(!memcmp(writable,prior,sizeof writable));
#ifdef OBSERVE_ACCESSES
    assert(entries==1 && exits==1);
#endif
    ++host_calls;
}

static void physical(FILE *in)
{
    u32 input[6];
    unsigned count=0;
    while(fread(input,sizeof input,1,in)==1) {
        SoundCommandCallback_t callback;
        uintptr_t token=(uintptr_t)input[4];
        _Static_assert(sizeof token==sizeof callback,"Linux callback representation");
        memcpy(&callback,&token,sizeof callback);
        execute(input[0],input[1],(void *)(uintptr_t)input[2],input[3],callback,(u16)input[5]);
        ++count;
    }
    assert(feof(in) && count==1440);
}

int main(int argc,char **argv)
{
    for(unsigned i=0;i<sizeof writable;++i) writable[i]=(unsigned char)(i*19u+7u);
    if(argc==2) {
        FILE *in=fopen(argv[1],"rb"); assert(in);
        physical(in); assert(fclose(in)==0);
        printf("%u original input vectors passed\n",host_calls);
        return 0;
    }
    assert(argc==1);
    size_t page=(size_t)sysconf(_SC_PAGESIZE);
    char *guard=mmap(NULL,page,PROT_NONE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
    assert(guard!=MAP_FAILED && sizeof(u_long)==sizeof(unsigned long));
    if(sizeof(uintptr_t)>4)
        assert((uintptr_t)guard>UINT32_MAX && (uintptr_t)callback_a>UINT32_MAX && (uintptr_t)callback_b>UINT32_MAX);
    const u32 addresses[]={0,1,0xffff,0x10000,0x7fffffff,0x80000000,0xfffffff0,0xffffffff};
    const u_long sizes[]={0,1,0xffff,0x10000,0x7fffffff,0x80000000u,0xffffffffu,ULONG_MAX
#if ULONG_MAX > UINT32_MAX
        ,0x100000000ul,0x112345678ul,0xffffffff00000009ul
#endif
    };
    void *data[]={NULL,writable,writable+3,writable+63,guard,guard+page-1,(void *)(uintptr_t)1};
    SoundCommandCallback_t callbacks[]={NULL,callback_a,callback_b};
    for(unsigned id=0;id<3;++id)
    for(unsigned a=0;a<sizeof addresses/sizeof *addresses;++a)
    for(unsigned s=0;s<sizeof sizes/sizeof *sizes;++s)
    for(unsigned d=0;d<sizeof data/sizeof *data;++d)
    for(unsigned c=0;c<sizeof callbacks/sizeof *callbacks;++c) {
        allow_service_write=(s+a+c)%2==0;
        execute(id,addresses[a],data[d],sizes[s],callbacks[c],(u16)(id+2));
    }
    for(unsigned cycle=0;cycle<256;++cycle)
        for(unsigned id=3;id<9;++id) execute(id,~cycle,guard,ULONG_MAX,callback_b,0);
    assert(munmap(guard,page)==0);
    printf("%u native calls passed (%zu-bit data pointer, %zu-bit length)\n",
           host_calls,sizeof(void *)*8,sizeof(u_long)*8);
    return 0;
}
#endif
