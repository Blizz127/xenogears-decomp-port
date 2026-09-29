/* Pointer transport contract. Real raster coverage lives in
 * string_render_pointer_test.c; this mock checks the call boundary. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include "common.h"
#include "pointers.inc"
static void *wanted_string,*wanted_work;
static unsigned string_equal,work_equal,row_equal,calls;
static void func_80033DF0(void *arg) {
    u8 *d=arg;u32 string,work,row;
    assert(d==s_StringRenderDescriptor && calls++==0);
    memcpy(&string,d+0x1c,4);memcpy(&work,d+0x2c,4);memcpy(&row,d+0x28,4);
    assert(string==(u32)(uintptr_t)wanted_string && work==(u32)(uintptr_t)wanted_work);
    assert(row==(u32)(uintptr_t)(d+0x90));
    string_equal=(uintptr_t)wanted_string==SYSTEM_TEXT_ADDRESS(d,0x1c);
    work_equal=(uintptr_t)wanted_work==SYSTEM_TEXT_ADDRESS(d,0x2c);
    row_equal=(uintptr_t)(d+0x90)==SYSTEM_TEXT_ADDRESS(d,0x28);
    u16 rendered=5;memcpy(d+0xe8,&rendered,2);
}
#include "render.inc"
static void run(void *string,void *work,unsigned expect_string,unsigned expect_work) {
    wanted_string=string;wanted_work=work;calls=0;
    memset(s_StringRenderDescriptor,0,sizeof s_StringRenderDescriptor);
    assert(SystemRenderStringEntry(string,work,0x24,0)==20 && calls==1);
    assert(string_equal==expect_string && work_equal==expect_work && row_equal);
}
int main(void) {
    u8 high_string[24],high_work[0x400];
    u8 *low=mmap(NULL,0x1000,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS|MAP_32BIT,-1,0);
    assert(low!=MAP_FAILED && (uintptr_t)low+0x1000<=UINT32_MAX);
    assert((uintptr_t)s_StringRenderDescriptor+sizeof s_StringRenderDescriptor<=UINT32_MAX);
    assert((uintptr_t)high_string>UINT32_MAX && (uintptr_t)high_work>UINT32_MAX);
    run(low,low+0x100,1,1);
    puts("CONTROL low string/work pointers preserved at descriptor boundary");
    run(high_string,low+0x100,1,1);
    puts("PASS high stack string pointer preserved");
    run(low,high_work,1,1);
    puts("PASS high work pointer preserved");
    assert(munmap(low,0x1000)==0);
    puts("BOUNDARY_ONLY: real renderer exercised by run_string_render_pointer_test.sh");
}
