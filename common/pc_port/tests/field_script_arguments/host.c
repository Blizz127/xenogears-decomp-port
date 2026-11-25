#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

int FieldScriptVMGetInstructionArgument(int);
int FieldScriptVMGetVariableValue(int);
short FieldScriptVMGetInstructionArgumentS16(int);

#ifndef FIELD_ARGUMENTS_EXTERNAL
#include "../../src/cleanroom/field_script_arguments.inc"
#else
int FieldScriptArgument1(int, int);
int FieldScriptArgument2(int, int);
int FieldScriptArgument3(int, int);
int FieldScriptArgument4(int, int);
int FieldScriptArgument5(int, int);
int FieldScriptArgument6(int, int);
int FieldScriptArgument7(int, int);
int FieldScriptArgument8(int, int);
#endif

enum Service { READ_WORD, READ_LITERAL, READ_VARIABLE };
struct Call { enum Service service; int argument; };
static struct Call trace[3];
static int count;
static int word;
static short literal;
static int variable;

static void note(enum Service service, int argument)
{
    if (count >= 3) abort();
    trace[count++] = (struct Call){ service, argument };
}

int FieldScriptVMGetInstructionArgument(int offset)
{
    note(READ_WORD, offset);
    return word;
}

short FieldScriptVMGetInstructionArgumentS16(int offset)
{
    note(READ_LITERAL, offset);
    return literal;
}

int FieldScriptVMGetVariableValue(int index)
{
    note(READ_VARIABLE, index);
    return variable;
}

int main(void)
{
    static int (*const readers[])(int, int) = {
        FieldScriptArgument1, FieldScriptArgument2, FieldScriptArgument3, FieldScriptArgument4,
        FieldScriptArgument5, FieldScriptArgument6, FieldScriptArgument7, FieldScriptArgument8
    };
    static const int offsets[] = {INT_MIN, -65537, -1, 0, 1, 65537, 0x1234abcd, INT_MAX};
    static const int words[] = {0, -1, 0xabcd, -32767, 0x1234fffe, INT_MIN, 0x7fff8000, -65529};
    static const short literals[] = {SHRT_MIN, -32767, -256, -1, 0, 1, 256, SHRT_MAX};
    static const int results[] = {INT_MAX, INT_MIN, -65537, 0x7654fedc, 0, -1, 0x120000, 65536};
    static const int upperFlags[] = {0, 0x2100, 0x7fffff00, -256};
    int cases = 0;

    for (int reader = 0; reader < 8; ++reader) {
        for (int low = 0; low < 256; ++low) {
            for (int high = 0; high < 4; ++high) {
                int flags = upperFlags[high] + low;
                for (int vector = 0; vector < 8; ++vector) {
                    int offset = offsets[vector];
                    word = words[vector];
                    literal = literals[vector];
                    variable = results[vector];
                    count = 0;
                    int actual = readers[reader](offset, flags);
                    int immediate = (low & (128 >> reader)) != 0;
                    if (count != (immediate ? 1 : 2) ||
                        trace[0].service != (immediate ? READ_LITERAL : READ_WORD) ||
                        trace[0].argument != offset ||
                        (!immediate && (trace[1].service != READ_VARIABLE ||
                                        trace[1].argument != (int)((unsigned)word & 65535u))) ||
                        actual != (immediate ? (int)literal : variable)) {
                        fprintf(stderr, "argument %d, flags %#x, vector %d failed\n",
                                reader + 1, (unsigned)flags, vector);
                        return 1;
                    }
                    ++cases;
                }
            }
        }
    }
    printf("Field script argument selectors passed %d return and ordered-call cases\n", cases);
    return 0;
}
