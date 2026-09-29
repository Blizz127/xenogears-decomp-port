#include <assert.h>
#include <stdint.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include "iso9660.h"
#define eprintinfo(...) ((void)0)
#include "vfile.inc"
typedef void (*CdlCB)(u_char,u_char*);
typedef void (*CdlDataCB)(void);
static VFILE g_imageFile;
static Sector g_cdSectorData;
static int g_cdSpoolerSeekCmd,g_cdReadDoneFlag,g_isCdSectorDataRead,g_cdSectorSize=2352;
static unsigned lock_depth,locks,unlocks,pace,ready_calls,data_calls,xa_calls,accepted_calls;
static int g_xeno_cdMode;
static struct {u_char file,chan;} g_xeno_cdFilter;
static unsigned consumes,accepts;
static unsigned char raw[4704];
static void lock(void){assert(lock_depth++==0);++locks;}
static void unlock(void){assert(lock_depth--==1);++unlocks;}
#define CD_LOCK_MUTEX() lock()
#define CD_UNLOCK_MUTEX() unlock()
static void _eCdSpoolerPace(void){assert(!lock_depth);++pace;}
static int PsyX_CdXaSectorAccepted(int a,int b,int c,int d,int e,int f){
    (void)a;(void)b;(void)c;(void)d;(void)e;(void)f;assert(!lock_depth);++accepted_calls;return accepts;
}
static void PsyX_SPUAL_CdPushXaSector(const void*p,int n,int coding){
    (void)coding;assert(!lock_depth && p==g_cdSectorData.data && n==2304);++xa_calls;
}
static void ready(u_char status,u_char*response){
    assert(!lock_depth && status==1);assert(response);++ready_calls;g_isCdSectorDataRead=consumes;
}
static void data(void){assert(!lock_depth);++data_calls;}
static CdlCB g_readyCallback;
static CdlDataCB g_dataCallback;
#include "stream.inc"
int main(void){
    unsigned fixtures=0;
    for(unsigned i=0;i<sizeof raw;++i)raw[i]=(i*19+3)&255;
    for(unsigned stop=0;stop<2;++stop)for(unsigned r=0;r<2;++r)
    for(unsigned d=0;d<2;++d)for(consumes=0;consumes<2;++consumes)
    for(accepts=0;accepts<2;++accepts)for(unsigned prior=0;prior<2;++prior){
        Sector before;memset(&before,0xa5,sizeof before);g_cdSectorData=before;
        g_imageFile=(VFILE){NULL,raw,raw,sizeof raw};
        g_cdSpoolerSeekCmd=stop?-1:0;g_cdReadDoneFlag=0;g_isCdSectorDataRead=prior;
        g_readyCallback=r?ready:NULL;g_dataCallback=d?data:NULL;
        lock_depth=locks=unlocks=pace=ready_calls=data_calls=xa_calls=accepted_calls=0;
        int result=_eCdSpoolerFunc();
        assert(!lock_depth && locks==1 && unlocks==1 && g_cdSpoolerSeekCmd==0);
        if(stop){
            assert(result==0 && g_cdReadDoneFlag==1);
            assert(g_imageFile.curPtr==raw && !memcmp(&before,&g_cdSectorData,sizeof before));
            assert(!pace && !ready_calls && !data_calls && !xa_calls && !accepted_calls);
            assert(g_isCdSectorDataRead==(int)prior);
        }else{
            assert(result==(int)r && g_cdReadDoneFlag==0);
            assert(g_imageFile.curPtr==raw+2352 && !memcmp(raw,&g_cdSectorData,2352));
            assert(pace==1 && accepted_calls==1 && xa_calls==accepts);
            assert(ready_calls==r && data_calls==(r&&d&&consumes));
        }
        ++fixtures;
    }
    printf("PASS %u stream pause/delivery fixtures; worker body, callbacks intercepted\n",fixtures);
}
