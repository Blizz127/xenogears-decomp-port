#include <assert.h>
#include <stdint.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include "vfile.inc"
int main(void){
    unsigned char bytes[257];unsigned cases=0;
    memset(bytes,0,sizeof bytes);
    const int offsets[]={INT_MIN,-258,-257,-256,-1,0,1,128,256,257,258,INT_MAX};
    const int origins[]={SEEK_SET,SEEK_CUR,SEEK_END,-1,99};
    for(unsigned length=0;length<=256;++length)for(unsigned pos=0;pos<=length;++pos)
    for(unsigned o=0;o<sizeof offsets/sizeof offsets[0];++o)
    for(unsigned w=0;w<sizeof origins/sizeof origins[0];++w){
        VFILE v={NULL,bytes,bytes+pos,length};
        int valid=origins[w]==SEEK_SET||origins[w]==SEEK_CUR||origins[w]==SEEK_END;
        int64_t base=origins[w]==SEEK_CUR?pos:origins[w]==SEEK_END?length:0;
        int64_t target=base+(int64_t)offsets[o];
        valid=valid && target>=0 && target<=length;
        int r=vfseek(&v,offsets[o],origins[w]);
        assert((r==0)==valid);
        assert(v.curPtr==bytes+(valid?(unsigned)target:pos));
        assert(vftell(&v)==(valid?(int)target:(int)pos));
        assert(v.basePtr==bytes && v.size==length && !v.fp);++cases;
    }
    FILE*f=tmpfile();assert(f);
    assert(fwrite(bytes,1,sizeof bytes,f)==sizeof bytes);rewind(f);
    VFILE file={f,NULL,NULL,0};
    assert(vfseek(&file,17,SEEK_SET)==0 && vftell(&file)==17);
    assert(vfseek(&file,-3,SEEK_CUR)==0 && vftell(&file)==14);
    assert(vfseek(&file,-1,SEEK_SET)!=0 && vftell(&file)==14);
    assert(vfseek(&file,0,SEEK_END)==0 && vftell(&file)==257);
    assert(fseek(f,(long)INT_MAX+1L,SEEK_SET)==0 && vftell(&file)==-1);
    assert(fclose(f)==0);
    VFILE missing={NULL,NULL,NULL,0};
    assert(vfseek(&missing,0,SEEK_SET)!=0 && vftell(&missing)==-1);
    VFILE oversized={NULL,bytes,bytes,SIZE_MAX};
    assert(vfseek(&oversized,0,SEEK_END)!=0 && oversized.curPtr==bytes);
    assert(vftell(&oversized)==-1);
    assert(vfseek(NULL,0,SEEK_SET)==-1 && vftell(NULL)==-1);
    VFILE before={NULL,bytes+1,bytes,4};
    assert(vfseek(&before,0,SEEK_CUR)==-1 && vftell(&before)==-1 && before.curPtr==bytes);
    VFILE after={NULL,bytes,bytes+5,4};
    assert(vfseek(&after,0,SEEK_CUR)==-1 && vftell(&after)==-1 && after.curPtr==bytes+5);
    puts("PASS file-backed seeks and invalid memory-image guards");
    printf("PASS %u bounded memory seek fixtures\n",cases);
}
