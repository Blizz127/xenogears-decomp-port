#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Keep the production-private ownership table visible to this focused test.
 * Unused overlay functions are discarded by --gc-sections in the runner. */
#include "../src/field_object_overlay.c"

enum CallKind {
    CALL_HEAP_FREE,
    CALL_MODEL_CLEANUP,
};

typedef struct RecordedCall {
    enum CallKind kind;
    void* pointer;
    u8 trackHeader[8];
    u8 auxHeader[8];
} RecordedCall;

static RecordedCall s_calls[32];
static int s_callCount;

u_int HeapFree(void* pointer)
{
    if (s_callCount >= (int)(sizeof(s_calls) / sizeof(s_calls[0]))) {
        abort();
    }
    s_calls[s_callCount].kind = CALL_HEAP_FREE;
    s_calls[s_callCount].pointer = pointer;
    memcpy(s_calls[s_callCount].trackHeader, D_801E86A8, 8);
    memcpy(s_calls[s_callCount].auxHeader, D_801E86A0, 8);
    s_callCount++;
    return 0;
}

void func_8002CBBC(u8* modelData)
{
    if (s_callCount >= (int)(sizeof(s_calls) / sizeof(s_calls[0]))) {
        abort();
    }
    s_calls[s_callCount].kind = CALL_MODEL_CLEANUP;
    s_calls[s_callCount].pointer = modelData;
    s_callCount++;
}

static void Check(int condition, const char* message)
{
    if (!condition) {
        fprintf(stderr, "FAIL: %s\n", message);
        exit(1);
    }
}

static void ExpectCall(int index, enum CallKind kind, void* pointer)
{
    Check(index < s_callCount, "missing cleanup call");
    Check(s_calls[index].kind == kind, "cleanup call kind/order mismatch");
    Check(s_calls[index].pointer == pointer, "cleanup call pointer mismatch");
}

int main(void)
{
    static u8 object[OVLY_OBJ_SIZE];
    static u8 nodes[3 * OVLY_NODE_STRIDE];
    static u8 copiedModel[0x80];
    static u8 model0[OVLY_GROUP_STRIDE];
    static u8 model1[OVLY_GROUP_STRIDE];
    static u8 packet0[0x40];
    static u8 packet1[0x40];
    static u8 object1[OVLY_OBJ_SIZE];
    static u8 object8[OVLY_OBJ_SIZE];
    static u8 workspaceAllocationA0[0x20];
    static u8 workspaceAllocationA8[0x20];
    static u32 modelPointers[2];
    const int slot = 3;

    Check((uintptr_t)object <= UINT32_MAX, "test storage must fit a PSX pointer");
    Check((uintptr_t)nodes <= UINT32_MAX, "node storage must fit a PSX pointer");

    memset(object, 0, sizeof(object));
    memset(nodes, 0, sizeof(nodes));
    memset(s_ptrTab, 0, sizeof(s_ptrTab));
    memset(D_801E8670, 0, sizeof(D_801E8670));
    memset(s_calls, 0, sizeof(s_calls));
    s_callCount = 0;

    modelPointers[0] = (u32)(uintptr_t)model0;
    modelPointers[1] = (u32)(uintptr_t)model1;
    s_ptrTab[slot].ptrs = modelPointers;
    s_ptrTab[slot].count = 2;

    *(u32*)(object + 0x00) = (u32)(uintptr_t)&s_ptrTab[slot];
    *(u32*)(object + 0x04) = (u32)(uintptr_t)nodes;
    *(u32*)(object + 0xA8) = (u32)(uintptr_t)copiedModel;
    *(u16*)(nodes + 0x0A) = 3;
    *(u32*)(nodes + 0x68) = (u32)(uintptr_t)packet0;
    *(u32*)(nodes + 0x6C) = (u32)(uintptr_t)(packet0 + 0x20);
    *(u32*)(nodes + OVLY_NODE_STRIDE + 0x68) =
        (u32)(uintptr_t)packet1;
    *(u32*)(nodes + OVLY_NODE_STRIDE + 0x6C) =
        (u32)(uintptr_t)(packet1 + 0x20);
    D_801E8670[slot] = (u32)(uintptr_t)object;

    *(u32*)(object + 0xAC) = (u32)(uintptr_t)packet0;
    func_801E8030(slot);
    Check(s_callCount == 0, "extended owner was partially freed");
    Check(D_801E8670[slot] == (u32)(uintptr_t)object,
          "extended owner slot was cleared");
    *(u32*)(object + 0xAC) = 0;

    func_801E8030(slot);

    Check(s_callCount == 8, "unexpected cleanup call count");
    ExpectCall(0, CALL_HEAP_FREE, copiedModel);
    ExpectCall(1, CALL_MODEL_CLEANUP, model0);
    ExpectCall(2, CALL_MODEL_CLEANUP, model1);
    ExpectCall(3, CALL_HEAP_FREE, modelPointers);
    ExpectCall(4, CALL_HEAP_FREE, packet0);
    ExpectCall(5, CALL_HEAP_FREE, packet1);
    ExpectCall(6, CALL_HEAP_FREE, nodes);
    ExpectCall(7, CALL_HEAP_FREE, object);

    Check(D_801E8670[slot] == 0, "slot was not cleared");
    Check(s_ptrTab[slot].ptrs == NULL, "pointer table allocation was not cleared");
    Check(s_ptrTab[slot].count == 0, "pointer table count was not cleared");
    Check(*(u32*)(nodes + 0x68) == 0 && *(u32*)(nodes + 0x6C) == 0,
          "root node packet fields were not cleared");
    Check(*(u32*)(nodes + OVLY_NODE_STRIDE + 0x68) == 0 &&
              *(u32*)(nodes + OVLY_NODE_STRIDE + 0x6C) == 0,
          "child node packet fields were not cleared");

    func_801E8030(slot);
    func_801E8030(-1);
    func_801E8030(OVLY_SLOT_MAX);
    Check(s_callCount == 8, "empty or invalid slot performed cleanup");

    memset(object1, 0, sizeof(object1));
    memset(object8, 0, sizeof(object8));
    *(u32*)object1 = (u32)(uintptr_t)&s_ptrTab[1];
    *(u32*)object8 = (u32)(uintptr_t)&s_ptrTab[8];
    D_801E8670[1] = (u32)(uintptr_t)object1;
    D_801E8670[8] = (u32)(uintptr_t)object8;
    *(u32*)D_801E86A8 = (u32)(uintptr_t)workspaceAllocationA8;
    *(u16*)(D_801E86A8 + 4) = 7;
    *(u32*)D_801E86A0 = (u32)(uintptr_t)workspaceAllocationA0;
    *(u16*)(D_801E86A0 + 4) = 8;
    *(u16*)(D_801E86A0 + 6) = 9;
    s_callCount = 0;

    func_801E7FD4();

    Check(s_callCount == 4, "destroy-all cleanup call count mismatch");
    ExpectCall(0, CALL_HEAP_FREE, object1);
    ExpectCall(1, CALL_HEAP_FREE, object8);
    ExpectCall(2, CALL_HEAP_FREE, workspaceAllocationA8);
    ExpectCall(3, CALL_HEAP_FREE, workspaceAllocationA0);
    Check(s_calls[2].trackHeader[4] == 0 && s_calls[2].trackHeader[5] == 0,
          "track next index must clear before HeapFree");
    Check(s_calls[3].auxHeader[4] == 0 && s_calls[3].auxHeader[5] == 0 &&
              s_calls[3].auxHeader[6] == 0 && s_calls[3].auxHeader[7] == 0,
          "aux indices must clear before HeapFree");
    Check(D_801E8670[1] == 0 && D_801E8670[8] == 0,
          "destroy-all left an object slot populated");
    Check(*(u32*)D_801E86A8 == 0 && *(u16*)(D_801E86A8 + 4) == 0,
          "destroy-all left workspace A8 populated");
    Check(*(u32*)D_801E86A0 == 0 && *(u16*)(D_801E86A0 + 4) == 0 &&
              *(u16*)(D_801E86A0 + 6) == 0,
          "destroy-all left workspace A0 populated");

    puts("FIELD_OBJECT_CLEANUP_ADAPTER PASS checks=39");
    return 0;
}
