/* Read-only 8001D0A4 for the first popup owner lookup. The list and callback
 * identities remain packed 32-bit words; no native callback translation is
 * performed. A bounded snapshot refuses cycles and changed inputs. */
typedef struct {
    uint32_t address,owner,callback,flags,next;
} PopupLookupNode;

typedef struct {
    uint32_t head,owner_id;
    unsigned count;
    PopupLookupNode nodes[32];
} PopupLookupSnapshot;

static int popup_lookup_snapshot(BattleMipsRuntime *runtime, uint32_t owner,
                                  PopupLookupSnapshot *snapshot)
{
    void **native_head=dlsym(RTLD_DEFAULT,"g_TimerWorkList");
    ResolvedData *binding=find_data(runtime,0x8005958cu,4u);
    uint32_t head;
    if (!native_head || !binding || binding->address!=0x8005958cu ||
        binding->host!=(uint8_t*)native_head || binding->size<4u ||
        (uintptr_t)*native_head>UINT32_MAX || (owner&3u) ||
        owner<BATTLE_BUS_KUSEG_RAM_END || owner>UINT32_MAX-0x10u ||
        runtime_read(runtime,owner+0x10u,4u,&snapshot->owner_id)!=0 ||
        runtime_read(runtime,0x8005958cu,4u,&head)!=0 ||
        head!=(uint32_t)(uintptr_t)*native_head)return 0;
    snapshot->head=head;snapshot->count=0;
    while (head) {
        if ((head&3u) || head<BATTLE_BUS_KUSEG_RAM_END ||
            head>UINT32_MAX-0x18u || snapshot->count>=32u ||
            !resolve_memory(runtime,head,0x1cu,0))return 0;
        for(unsigned i=0;i<snapshot->count;i++)
            if(snapshot->nodes[i].address==head)return 0;
        PopupLookupNode *n=&snapshot->nodes[snapshot->count++];n->address=head;
        if(runtime_read(runtime,head,4u,&n->owner)!=0 ||
           runtime_read(runtime,head+8u,4u,&n->callback)!=0 ||
           runtime_read(runtime,head+0x14u,4u,&n->flags)!=0 ||
           runtime_read(runtime,head+0x18u,4u,&n->next)!=0)return 0;
        head=n->next;
    }
    return 1;
}

static int popup_lookup_unchanged(BattleMipsRuntime *runtime,uint32_t owner,
                                   const PopupLookupSnapshot *before)
{
    PopupLookupSnapshot after;
    if(!popup_lookup_snapshot(runtime,owner,&after) || before->head!=after.head ||
       before->owner_id!=after.owner_id || before->count!=after.count)return 0;
    for(unsigned i=0;i<after.count;i++) {
        const PopupLookupNode *a=&before->nodes[i],*b=&after.nodes[i];
        if(a->address!=b->address || a->owner!=b->owner ||
           a->callback!=b->callback || a->flags!=b->flags || a->next!=b->next)return 0;
    }
    return 1;
}

static void battle_popup_lookup_run(BattleEntryCall *entry,
                                     const PopupLookupSnapshot *snapshot)
{
    uint32_t *r=entry->cpu->gpr;
    if(!popup_lookup_unchanged(entry->runtime,r[4],snapshot))
        battle_entry_fail(entry,"popup lookup inputs changed",r[4]);
    r[6]=battle_entry_read(entry,r[28]+0x41cu,4);r[2]=0;
    if(!r[6])return;
    r[7]=0x1fffffffu;
    for(unsigned i=0;i<snapshot->count;i++) {
        if(r[6]!=snapshot->nodes[i].address)
            battle_entry_fail(entry,"popup lookup link changed",r[6]);
        r[2]=battle_entry_read(entry,r[6],4);
        if(r[2]==r[4]) {
            r[3]=battle_entry_read(entry,r[6]+0x14u,4);
            r[2]=battle_entry_read(entry,r[4]+0x10u,4);
            r[3]&=r[7];r[2]&=r[7];
            if(r[3]==r[2]) {
                r[2]=battle_entry_read(entry,r[6]+8u,4);
                int match=(r[2]==r[5]);r[2]=r[6];
                if(match)return;
            }
        }
        r[6]=battle_entry_read(entry,r[6]+0x18u,4);r[2]=0;
        if(!r[6])return;
    }
    battle_entry_fail(entry,"popup lookup list changed",r[6]);
}

static int battle_popup_lookup_try_host(BattleMipsRuntime *runtime,
                                         PcPortMipsCpu *cpu,uint32_t target)
{
    if(target!=0x8001d0a4u ||
       (cpu->gpr[31]!=0x800bce4cu && cpu->gpr[31]!=0x800bce1cu) ||
       runtime->force_interpret || !runtime->initialized ||
       g_ActiveBattleRuntime!=runtime || runtime->bridge_cpu!=cpu ||
       (cpu->load_valid && cpu->load_reg!=0) || cpu->gpr[28]!=0x80059170u ||
       cpu->gpr[5]!=0x800bcfacu)return 0;
    const char *enabled=getenv("XENO_BATTLE_NATIVE_POPUP_LOOKUP_EXPERIMENT");
    if(!enabled || strcmp(enabled,"1")!=0)return 0;
    const XenoRetailFileSlot *image=XenoRetailData_File(XENO_RD_SLUS);
    size_t offset=XENO_RD_SLUS_OFF(target);
    if(!image || !image->bytes || image->size<offset+0x68u)return 0;
    static const char expected[]="4d2e3d0559a80d25efb325eadf0c64edacc7fabd24e3147a3ade1cfd7c0f1090";
    uint8_t digest[32];file1_digest(image->bytes+offset,0x68u,digest);
    for(unsigned i=0;i<sizeof(digest);i++)
        if(file1_hex_digit(digest[i]>>4)!=expected[2*i] ||
           file1_hex_digit(digest[i]&15u)!=expected[2*i+1])return 0;
    PopupLookupSnapshot snapshot;
    if(!popup_lookup_snapshot(runtime,cpu->gpr[4],&snapshot))return 0;
    BattleEntryCall entry={.runtime=runtime,.cpu=cpu};
    if(setjmp(entry.failure)!=0)return -1;
    battle_popup_lookup_run(&entry,&snapshot);
    cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}
