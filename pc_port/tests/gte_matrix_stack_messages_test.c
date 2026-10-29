/* Real SDK stack operations: valid calls stay silent, failures preserve state.
 * Link against the candidate LIBGTE.C and GTE support objects. */
#include <libgte.h>
#include <psx/gtereg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

extern int matrixLevel;
extern MATRIX stack[],*currentMatrix;

static void check(int ok,const char *what)
{
    if(!ok) {
        fprintf(stderr,"GTE MATRIX STACK FAIL: %s\n",what);
        exit(1);
    }
}

static void output_matches(FILE *capture,const char *expected)
{
    static off_t consumed;
    char buffer[256];
    check(fflush(stdout)==0,"stdout flush");
    /* Read appended bytes without changing stdout's shared file offset or
     * its stdio position cache. Truncating through a second FILE is unsafe. */
    off_t end=lseek(fileno(capture),0,SEEK_CUR);
    long size=(long)(end-consumed);
    check(size>=0&&(size_t)size<sizeof(buffer),"bounded diagnostic");
    check(pread(fileno(capture),buffer,(size_t)size,consumed)==size,"diagnostic read");
    buffer[size]=0;
    if(strcmp(buffer,expected))
        fprintf(stderr,"diagnostic expected <%s>, got <%s> (%ld bytes, depth %d)\n",
                expected,buffer,size,matrixLevel);
    check(strcmp(buffer,expected)==0,"diagnostic text and count");
    consumed=end;
}

int main(void)
{
    _Static_assert(sizeof(MATRIX)==32,"packed SDK matrix");
    GTERegisters saved[20];
    MATRIX matrices[20],before[20];
    FILE *capture=tmpfile();
    check(capture!=NULL,"capture open");
    check(fflush(stdout)==0,"initial stdout flush");
    int original=dup(STDOUT_FILENO);
    check(original>=0&&dup2(fileno(capture),STDOUT_FILENO)>=0,"capture stdout");

    memset(&gteRegs,0x42,sizeof(gteRegs));
    memset(stack,0x96,sizeof(before));
    matrixLevel=0;currentMatrix=stack;
    for(unsigned depth=0;depth<20;depth++) {
        MATRIX *m=&matrices[depth];
        memset(m,0,sizeof(*m));
        for(unsigned row=0;row<3;row++)
            for(unsigned col=0;col<3;col++)
                m->m[row][col]=(short)(-3000+(int)depth*41+(int)row*7+(int)col);
        for(unsigned axis=0;axis<3;axis++)
            m->t[axis]=(int32_t)(0x80000000u+depth*101u+axis*113u);
        SetRotMatrix(m);SetTransMatrix(m);
        saved[depth]=gteRegs;
        memcpy(before,stack,sizeof(before));
        PushMatrix();
        check(matrixLevel==(int)depth+1&&currentMatrix==stack+depth+1,"push depth");
        check(!memcmp(&gteRegs,&saved[depth],sizeof(gteRegs)),"push GTE preservation");
        check(!memcmp(stack[depth].m,m->m,sizeof(m->m))&&
              !memcmp(stack[depth].t,m->t,sizeof(m->t)),"saved matrix");
        /* Saving the fifth control word also writes the matrix padding. */
        memcpy(&before[depth],saved[depth].CP2C.r,sizeof(MATRIX));
        check(!memcmp(stack,before,sizeof(before)),"push writes only saved matrix");
        output_matches(capture,"");
    }
    memcpy(before,stack,sizeof(before));
    GTERegisters overflow=gteRegs;
    PushMatrix();
    check(matrixLevel==20&&currentMatrix==stack+20,"overflow depth");
    check(!memcmp(stack,before,sizeof(before))&&
          !memcmp(&gteRegs,&overflow,sizeof(gteRegs)),"overflow preserves state");
    output_matches(capture,"[psycross] PushMatrix: matrix stack full (20 entries)\n");

    for(int depth=19;depth>=0;depth--) {
        PopMatrix();
        check(matrixLevel==depth&&currentMatrix==stack+depth,"pop depth");
        check(!memcmp(&gteRegs,&saved[depth],sizeof(gteRegs)),"pop restores GTE");
        check(!memcmp(stack,before,sizeof(before)),"pop preserves saved matrices");
        output_matches(capture,"");
    }
    GTERegisters underflow=gteRegs;
    PopMatrix();
    check(matrixLevel==0&&currentMatrix==stack,"underflow depth");
    check(!memcmp(stack,before,sizeof(before))&&
          !memcmp(&gteRegs,&underflow,sizeof(gteRegs)),"underflow preserves state");
    output_matches(capture,"[psycross] PopMatrix: matrix stack empty\n");

    check(dup2(original,STDOUT_FILENO)>=0,"restore stdout");
    close(original);fclose(capture);
    puts("GTE MATRIX STACK PASS: 40 valid silent calls, overflow and underflow");
    return 0;
}
