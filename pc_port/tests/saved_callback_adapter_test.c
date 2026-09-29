#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "work_list_callback.h"
extern void PcPort_WorkListInvokeSavedCallback(uint32_t,void*) __attribute__((weak));
static unsigned nativeCalls,guestCalls;
static uint32_t value;
static void nativeCallback(void*p){assert(p==&value);++nativeCalls;value^=0x55aa;}
int PcPort_BattleMipsDispatchCallback(uint32_t callback,void*p){++guestCalls;if(callback!=0x800bc018u)return 0;assert(p==&value);value+=0x1234;return 1;}
int main(int argc,char**argv){(void)argv;if(!PcPort_WorkListInvokeSavedCallback){fputs("SAVED CALLBACK FAIL missing adapter\n",stderr);return 1;}if(argc>1){PcPort_WorkListInvokeSavedCallback(0x80123450u,&value);fputs("SAVED CALLBACK FAIL unresolved guest returned\n",stderr);return 1;}
 PcPort_WorkListInvokeSavedCallback((uint32_t)(uintptr_t)nativeCallback,&value);assert(nativeCalls==1&&guestCalls==0&&value==0x55aa);
 PcPort_WorkListInvokeSavedCallback(0x800bc018u,&value);assert(nativeCalls==1&&guestCalls==1&&value==0x67de);
 puts("SAVED CALLBACK PASS native and guest routing");
}
