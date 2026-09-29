#include "battle_target_list_ram.h"
#include "battle_target_eligibility_ram.h"
#include "battle_target_selection_ram.h"
#include <setjmp.h>
typedef uint8_t u8;
typedef uint32_t u32;
typedef int32_t s32;
struct ListRam {
    uint8_t *bytes;
    size_t size;
    uint32_t frame;
    jmp_buf failure;
};
static uint8_t *access_ram(struct ListRam *r, uint32_t address,
                           unsigned width, unsigned alignment) {
    uint32_t at;
    if (address>=0x80000000u && address<0x80200000u) at=address-0x80000000u;
    else if (address>=0xA0000000u && address<0xA0200000u) at=address-0xA0000000u;
    else longjmp(r->failure,1);
    if (address%alignment || width>0x200000u-at || at>r->size || width>r->size-at)
        longjmp(r->failure,1);
    return r->bytes+at;
}
static uint32_t read_word(struct ListRam *r, uint32_t address, unsigned width) {
    uint8_t *p=access_ram(r,address,width,width);
    uint32_t value=0;
    for (unsigned i=0; i<width; i++) value|=(uint32_t)p[i]<<(8*i);
    return value;
}
static void save_word(struct ListRam *r, unsigned offset, uint32_t value) {
    uint8_t *p=access_ram(r,r->frame+offset,4,4);
    for (unsigned i=0; i<4; i++) p[i]=value>>(8*i);
}
static uint32_t read_eligibility(struct ListRam *r, uint32_t actor, uint32_t target) {
    uint32_t result;
    if (PcPortBattleTargetEligibilityRam(r->bytes,r->size,actor,target,&result))
        longjmp(r->failure,1);
    return result;
}
#define XBT_LIST_CUSTOM_BINDINGS
#define XBT_LIST_SIGNATURE static u32 list_body(struct ListRam *r, u32 arg0)
#define XBT_LIST_LOCALS
/* Validate scratch at each actual access, after preceding retail writes. */
#define XBT_LIST_CANDIDATE(i) (*access_ram(r,r->frame+0x10+(i),1,1))
#define XBT_LIST_MATCHING(i) (*access_ram(r,r->frame+0x20+(i),1,1))
#define XBT_LIST_COUNT (*access_ram(r,0x800D3274u,1,1))
#define XBT_LIST_ENTRY(i) (*access_ram(r,0x800C3E90u+(i),1,1))
#define XBT_LIST_GROUP(i) (*access_ram(r,0x800C3EB4u+(i)*28,1,1))
#define XBT_LIST_RANK(i) read_word(r,0x800CCD34u+(i)*0x170,2)
#define XBT_LIST_ELIGIBLE(a,t) read_eligibility(r,a,t)
#include "../../src/battle/target_list_impl.inc"

int PcPortBattleTargetListRam(uint8_t *bytes, size_t size, PcPortMipsCpu *cpu) {
    if (!bytes || !cpu) return -1;
    struct ListRam r={.bytes=bytes,.size=size,.frame=cpu->gpr[29]-0x50u};
    if (setjmp(r.failure)) return -1;
    /* Retail save order, not a bulk frame clear. Untouched gaps remain RAM. */
    cpu->gpr[29]=r.frame;
    save_word(&r,0x48,cpu->gpr[20]);
    save_word(&r,0x38,cpu->gpr[16]);
    save_word(&r,0x4C,cpu->gpr[31]);
    save_word(&r,0x44,cpu->gpr[19]);
    save_word(&r,0x40,cpu->gpr[18]);
    save_word(&r,0x3C,cpu->gpr[17]);
    cpu->gpr[2]=list_body(&r,cpu->gpr[4]);
    /* Reload saved bytes: they may have been changed through guest aliases. */
    cpu->gpr[31]=read_word(&r,r.frame+0x4C,4);
    for (unsigned reg=20; reg>=16; reg--)
        cpu->gpr[reg]=read_word(&r,r.frame+0x38+(reg-16)*4,4);
    cpu->gpr[29]=r.frame+0x50;
    return 0;
}

struct SelectionRam {
    struct ListRam memory;
    PcPortMipsCpu *cpu;
    uint32_t owner;
    uint8_t target, count;
};
static uint32_t context_address(struct SelectionRam *s, uint32_t displacement) {
    /* Validate packed owner before derived address construction. */
    (void)access_ram(&s->memory,s->owner,1,1);
    if (s->owner>UINT32_MAX-displacement) longjmp(s->memory.failure,1);
    return s->owner+displacement;
}
static uint32_t context_target(struct SelectionRam *s) {
    if (s->cpu->gpr[17]>UINT32_MAX-0x3C) longjmp(s->memory.failure,1);
    return read_word(&s->memory,context_address(s,s->cpu->gpr[17]+0x3C),1);
}
static void store_selection(struct SelectionRam *s, uint32_t value) {
    *access_ram(&s->memory,context_address(s,0x2E8),1,1)=(uint8_t)value;
}
static void build_selection_list(struct SelectionRam *s, uint32_t actor) {
    s->cpu->gpr[4]=actor;
    s->cpu->gpr[31]=0x80084AB0; /* Return PC saved by the nested list frame. */
    if (PcPortBattleTargetListRam(s->memory.bytes,s->memory.size,s->cpu))
        longjmp(s->memory.failure,1);
}
static void after_selection_list(struct SelectionRam *s) {
    s->count=read_word(&s->memory,0x800D3274,1);
    if (s->count) {
        s->owner=read_word(&s->memory,0x800C3EAC,4);
        s->target=context_target(s);
    }
}
#define XBT_SELECTION_CUSTOM_BINDINGS
#define XBT_SELECTION_SIGNATURE static void selection_body(struct SelectionRam *s, u32 arg0)
#define XBT_SELECTION_TARGET(a) s->target
#define XBT_SELECTION_COUNT s->count
/* List aliases may change saved s0. Retail tests and increments the restored
 * register, not a host-local boolean. uint32_t also preserves wrapping ADDIU. */
#define XBT_SELECTION_LOCALS
#define XBT_SELECTION_FOUND s->cpu->gpr[16]
#define XBT_SELECTION_ENTRY(i) read_word(&s->memory,0x800C3E90u+(i),1)
#define XBT_SELECTION_BUILD(a) build_selection_list(s,a)
#define XBT_SELECTION_STORE(value) store_selection(s,value)
#define XBT_SELECTION_AFTER_BUILD after_selection_list(s);
#define XBT_SELECTION_BEFORE_FALLBACK s->owner=read_word(&s->memory,0x800C3EAC,4);
#include "../../src/battle/target_selection_impl.inc"

int PcPortBattleTargetSelectionRam(uint8_t *bytes, size_t size, PcPortMipsCpu *cpu) {
    if (!bytes || !cpu) return -1;
    struct SelectionRam s={.memory={.bytes=bytes,.size=size,.frame=cpu->gpr[29]-0x28u},
                           .cpu=cpu};
    if (setjmp(s.memory.failure)) return -1;
    uint32_t actor=cpu->gpr[4]&255;
    cpu->gpr[29]=s.memory.frame;
    s.owner=read_word(&s.memory,0x800C3EAC,4);
    save_word(&s.memory,0x1C,cpu->gpr[17]);
    cpu->gpr[17]=actor*64;
    save_word(&s.memory,0x20,cpu->gpr[31]);
    save_word(&s.memory,0x18,cpu->gpr[16]);
    s.target=context_target(&s);
    cpu->gpr[16]=0;
    selection_body(&s,actor);
    cpu->gpr[31]=read_word(&s.memory,s.memory.frame+0x20,4);
    cpu->gpr[17]=read_word(&s.memory,s.memory.frame+0x1C,4);
    cpu->gpr[16]=read_word(&s.memory,s.memory.frame+0x18,4);
    cpu->gpr[29]=s.memory.frame+0x28;
    return 0;
}
