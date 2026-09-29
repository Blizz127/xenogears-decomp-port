#include "common.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

extern u32 D_800AFC60, D_800AFC64, D_800AFC68;
extern s32 D_800AF278;
extern void func_800A83B4(void);
static uintptr_t freed[2];
static unsigned free_count, sync_count;
int DrawSync(int mode) { if(mode==0) ++sync_count; return 0; }
void HeapFree(void* pointer) {
    if(free_count<2) freed[free_count]=(uintptr_t)pointer;
    ++free_count;
}
int main(void) {
    if((uintptr_t)&D_800AFC64-(uintptr_t)&D_800AFC60!=4 ||
       (uintptr_t)&D_800AFC68-(uintptr_t)&D_800AFC64!=4) return 2;
    for(unsigned active=0;active<2;++active) {
        D_800AFC60=0x12345000;D_800AFC64=0x23456000;D_800AFC68=0xabcdef01;
        D_800AF278=active;free_count=sync_count=0;memset(freed,0,sizeof(freed));
        func_800A83B4();
        if(D_800AF278 || free_count!=2*active || sync_count!=active ||
           (active && (freed[0]!=0x12345000 || freed[1]!=0x23456000)) ||
           D_800AFC60!=0x12345000 || D_800AFC64!=0x23456000 || D_800AFC68!=0xabcdef01) {
            fprintf(stderr,"OVERLAY POINTER FAIL active=%u free=%lx/%lx count=%u\n",
                    active,(unsigned long)freed[0],(unsigned long)freed[1],free_count);
            return 1;
        }
    }
    puts("OVERLAY POINTER PASS active/inactive cleanup with real packed data aliases");
    return 0;
}
