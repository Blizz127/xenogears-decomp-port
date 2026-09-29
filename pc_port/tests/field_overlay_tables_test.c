#include <stdint.h>
#include <stdio.h>
#include <string.h>
extern unsigned short g_FieldOverlayLayoutData[226][4] __attribute__((weak));
extern unsigned short D_800AEB68[] __attribute__((weak));
extern unsigned short D_800AEB6A[] __attribute__((weak));
extern unsigned short D_800AEB6C[] __attribute__((weak));
extern unsigned short D_800AEB6E[] __attribute__((weak));
extern unsigned short D_800AEF10[] __attribute__((weak));
extern unsigned short D_800AEF14[] __attribute__((weak));
extern unsigned short D_800AEF16[] __attribute__((weak));
int main(void) {
    unsigned char retail[0x710];
    if(!g_FieldOverlayLayoutData || !D_800AEF10 || !D_800AEF14 || !D_800AEF16) {
        fputs("OVERLAY TABLE FAIL missing owner\n",stderr);return 1;
    }
    FILE* f=fopen("disc/field.bin","rb");
    if(!f || fseek(f,0x800aeb68u-0x8006faf0u,SEEK_SET) ||
       fread(retail,1,sizeof(retail),f)!=sizeof(retail)) return 2;
    if(fclose(f)) return 2;
    uintptr_t base=(uintptr_t)g_FieldOverlayLayoutData;
    if((uintptr_t)D_800AEB68!=base || (uintptr_t)D_800AEB6A!=base+2 ||
       (uintptr_t)D_800AEB6C!=base+4 || (uintptr_t)D_800AEB6E!=base+6 ||
       (uintptr_t)D_800AEF10!=base+0x3a8 || (uintptr_t)D_800AEF14!=base+0x3ac ||
       (uintptr_t)D_800AEF16!=base+0x3ae ||
       memcmp(retail,g_FieldOverlayLayoutData,sizeof(retail))) {
        fputs("OVERLAY TABLE FAIL bytes/interior aliases\n",stderr);return 1;
    }
    for(unsigned i=0;i<109;++i) if(D_800AEF14[i*4]>=117) return 3;
    puts("OVERLAY TABLE PASS all 1808 retail bytes, seven aliases, 109 texture indices");
    return 0;
}
