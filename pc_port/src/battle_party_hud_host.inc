/* Fixed host C for the complete party HUD state routine. Generated offline
 * from the identity-pinned private body, then reviewed and tested against
 * its test-only CPU oracle. No original instruction payload is embedded.
 * Loads use direct temporaries and fixed commits; branches are C gotos.
 * Original call sites and guest memory effects remain observable. */
static void battle_party_hud_run(BattleEntryCall *entry)
{
    uint32_t *r = entry->cpu->gpr;
    /* 80073538 */
    r[2] = 0x800d0000u;
    /* 8007353C */
    uint32_t load_8007353c = battle_entry_read(entry, r[2] + 0x00002d28u, 4);
    /* 80073540 */
    r[29] = r[29] + 0xffffffb0u;
    r[2] = load_8007353c;
    /* 80073544 */
    battle_entry_write(entry, r[29] + 0x0000002cu, 4, r[17]);
    /* 80073548 */
    r[17] = 0u + 0u;
    /* 8007354C */
    battle_entry_write(entry, r[29] + 0x00000048u, 4, r[30]);
    /* 80073550 */
    r[30] = 0x800c0000u;
    /* 80073554 */
    r[30] = r[30] + 0x00003254u;
    /* 80073558 */
    battle_entry_write(entry, r[29] + 0x00000044u, 4, r[23]);
    /* 8007355C */
    r[23] = 0u | 0x1000u;
    /* 80073560 */
    battle_entry_write(entry, r[29] + 0x0000003cu, 4, r[21]);
    /* 80073564 */
    r[21] = 0x800d0000u;
    /* 80073568 */
    r[21] = r[21] + 0x000032a1u;
    /* 8007356C */
    battle_entry_write(entry, r[29] + 0x00000034u, 4, r[19]);
    /* 80073570 */
    r[19] = 0u | 0x2e08u;
    /* 80073574 */
    battle_entry_write(entry, r[29] + 0x00000038u, 4, r[20]);
    /* 80073578 */
    r[20] = 0u + 0u;
    /* 8007357C */
    battle_entry_write(entry, r[29] + 0x00000040u, 4, r[22]);
    /* 80073580 */
    r[22] = 0u + 0u;
    /* 80073584 */
    battle_entry_write(entry, r[29] + 0x0000004cu, 4, r[31]);
    /* 80073588 */
    battle_entry_write(entry, r[29] + 0x00000030u, 4, r[18]);
    /* 8007358C */
    battle_entry_write(entry, r[29] + 0x00000028u, 4, r[16]);
    /* 80073590 */
    battle_entry_write(entry, r[2] + 0x00000077u, 1, 0u);
L_80073594:;
    /* 80073594 */
    r[1] = 0x800c0000u;
    /* 80073598 */
    r[1] = r[1] + r[22];
    /* 8007359C */
    uint32_t load_8007359c = battle_entry_read(entry, r[1] + 0x00003eb6u, 1);
    /* 800735A0 */
    r[2] = 0u | 0x7fu;
    r[3] = load_8007359c;
    /* 800735A4: fixed control flow */
    int take_800735a4 = r[3] == r[2];
    /* 800735A8 */
    /* delay slot has no register or memory effect */
    if (take_800735a4) goto L_80073a08;
    /* 800735AC */
    r[2] = 0x800d0000u;
    /* 800735B0 */
    uint32_t load_800735b0 = battle_entry_read(entry, r[2] + 0x00002d28u, 4);
    /* 800735B4 */
    /* delay slot has no register or memory effect */
    r[2] = load_800735b0;
    /* 800735B8 */
    r[5] = r[2] + r[17];
    /* 800735BC */
    uint32_t load_800735bc = battle_entry_read(entry, r[5] + 0x00000090u, 1);
    /* 800735C0 */
    r[2] = 0u | 0x1u;
    r[3] = load_800735bc;
    /* 800735C4: fixed control flow */
    int take_800735c4 = r[3] == r[2];
    /* 800735C8 */
    r[2] = (int32_t)r[3] < 2;
    if (take_800735c4) goto L_80073838;
    /* 800735CC: fixed control flow */
    int take_800735cc = r[2] == 0u;
    /* 800735D0 */
    /* delay slot has no register or memory effect */
    if (take_800735cc) goto L_800735e4;
    /* 800735D4: fixed control flow */
    int take_800735d4 = r[3] == 0u;
    /* 800735D8 */
    r[4] = 0u | 0x8fu;
    if (take_800735d4) goto L_80073600;
    /* 800735DC: fixed control flow */
    /* 800735E0 */
    r[21] = r[21] + 0x00000008u;
    goto L_80073a0c;
L_800735e4:;
    /* 800735E4 */
    r[2] = 0u | 0x2u;
    /* 800735E8: fixed control flow */
    int take_800735e8 = r[3] == r[2];
    /* 800735EC */
    r[2] = 0u | 0x3u;
    if (take_800735e8) goto L_800739c4;
    /* 800735F0: fixed control flow */
    int take_800735f0 = r[3] == r[2];
    /* 800735F4 */
    /* delay slot has no register or memory effect */
    if (take_800735f0) goto L_80073988;
    /* 800735F8: fixed control flow */
    /* 800735FC */
    r[21] = r[21] + 0x00000008u;
    goto L_80073a0c;
L_80073600:;
    /* 80073600 */
    r[7] = 0u | 0x1fu;
    /* 80073604 */
    battle_entry_write(entry, r[5] + 0x00000074u, 1, 0u);
    /* 80073608 */
    r[2] = 0x800d0000u;
    /* 8007360C */
    uint32_t load_8007360c = battle_entry_read(entry, r[2] + 0x00002d28u, 4);
    /* 80073610 */
    r[8] = 0x800c0000u;
    r[2] = load_8007360c;
    /* 80073614 */
    uint32_t load_80073614 = battle_entry_read(entry, r[8] + 0x00003ea4u, 4);
    /* 80073618 */
    r[3] = 0x800d0000u;
    r[8] = load_80073614;
    /* 8007361C */
    uint32_t load_8007361c = battle_entry_read(entry, r[3] + 0x00003280u, 1);
    /* 80073620 */
    r[2] = r[2] + r[17];
    r[3] = load_8007361c;
    /* 80073624 */
    uint32_t load_80073624 = battle_entry_read(entry, r[2] + 0x00000074u, 1);
    /* 80073628 */
    r[8] = r[19] + r[8];
    r[2] = load_80073624;
    /* 8007362C */
    r[5] = r[2] << 2;
    /* 80073630 */
    r[5] = r[5] + r[2];
    /* 80073634 */
    r[5] = r[5] << 4;
    /* 80073638 */
    r[2] = r[3] << 1;
    /* 8007363C */
    r[2] = r[2] + r[3];
    /* 80073640 */
    r[2] = r[2] + r[17];
    /* 80073644 */
    r[2] = r[2] << 1;
    /* 80073648 */
    r[2] = r[2] + r[30];
    /* 8007364C */
    uint32_t load_8007364c = battle_entry_read(entry, r[2] + 0x00000000u, 2);
    /* 80073650 */
    r[5] = r[8] + r[5];
    r[6] = load_8007364c;
    /* 80073654 */
    r[6] = r[6] + 0x00000044u;
    /* 80073658 */
    r[6] = r[20] + r[6];
    /* 8007365C */
    r[6] = r[6] << 16;
    /* 80073660: fixed control flow */
    r[31] = 0x80073668u;
    /* 80073664 */
    r[6] = (r[6] >> 16) | ((r[6] & 0x80000000u) ? 0xffff0000u : 0u);
    battle_phase_call(entry, 0x80076a10u, 0x80073660u);
    /* 80073668 */
    r[5] = 0x800d0000u;
    /* 8007366C */
    uint32_t load_8007366c = battle_entry_read(entry, r[5] + 0x00002d28u, 4);
    /* 80073670 */
    r[4] = 0u | 0x52u;
    r[5] = load_8007366c;
    /* 80073674 */
    r[5] = r[5] + r[17];
    /* 80073678 */
    uint32_t load_80073678 = battle_entry_read(entry, r[5] + 0x00000074u, 1);
    /* 8007367C */
    r[7] = 0u | 0x1cu;
    r[3] = load_80073678;
    /* 80073680 */
    r[3] = r[3] + r[2];
    /* 80073684 */
    battle_entry_write(entry, r[5] + 0x00000074u, 1, r[3]);
    /* 80073688 */
    r[2] = 0x800d0000u;
    /* 8007368C */
    uint32_t load_8007368c = battle_entry_read(entry, r[2] + 0x00002d28u, 4);
    /* 80073690 */
    r[8] = 0x800c0000u;
    r[2] = load_8007368c;
    /* 80073694 */
    uint32_t load_80073694 = battle_entry_read(entry, r[8] + 0x00003ea4u, 4);
    /* 80073698 */
    r[3] = 0x800d0000u;
    r[8] = load_80073694;
    /* 8007369C */
    uint32_t load_8007369c = battle_entry_read(entry, r[3] + 0x00003280u, 1);
    /* 800736A0 */
    r[2] = r[2] + r[17];
    r[3] = load_8007369c;
    /* 800736A4 */
    uint32_t load_800736a4 = battle_entry_read(entry, r[2] + 0x00000074u, 1);
    /* 800736A8 */
    r[8] = r[19] + r[8];
    r[2] = load_800736a4;
    /* 800736AC */
    r[5] = r[2] << 2;
    /* 800736B0 */
    r[5] = r[5] + r[2];
    /* 800736B4 */
    r[5] = r[5] << 4;
    /* 800736B8 */
    r[2] = r[3] << 1;
    /* 800736BC */
    r[2] = r[2] + r[3];
    /* 800736C0 */
    r[2] = r[2] + r[17];
    /* 800736C4 */
    r[2] = r[2] << 1;
    /* 800736C8 */
    r[2] = r[2] + r[30];
    /* 800736CC */
    uint32_t load_800736cc = battle_entry_read(entry, r[2] + 0x00000000u, 2);
    /* 800736D0 */
    r[5] = r[8] + r[5];
    r[6] = load_800736cc;
    /* 800736D4 */
    r[6] = r[6] + 0x00000048u;
    /* 800736D8 */
    r[6] = r[20] + r[6];
    /* 800736DC */
    r[6] = r[6] << 16;
    /* 800736E0: fixed control flow */
    r[31] = 0x800736e8u;
    /* 800736E4 */
    r[6] = (r[6] >> 16) | ((r[6] & 0x80000000u) ? 0xffff0000u : 0u);
    battle_phase_call(entry, 0x80076a6cu, 0x800736e0u);
    /* 800736E8 */
    r[5] = 0x800d0000u;
    /* 800736EC */
    uint32_t load_800736ec = battle_entry_read(entry, r[5] + 0x00002d28u, 4);
    /* 800736F0 */
    r[4] = 0u | 0x53u;
    r[5] = load_800736ec;
    /* 800736F4 */
    r[5] = r[5] + r[17];
    /* 800736F8 */
    uint32_t load_800736f8 = battle_entry_read(entry, r[5] + 0x00000074u, 1);
    /* 800736FC */
    r[7] = 0u | 0x1cu;
    r[3] = load_800736f8;
    /* 80073700 */
    r[3] = r[3] + r[2];
    /* 80073704 */
    battle_entry_write(entry, r[5] + 0x00000074u, 1, r[3]);
    /* 80073708 */
    r[2] = 0x800d0000u;
    /* 8007370C */
    uint32_t load_8007370c = battle_entry_read(entry, r[2] + 0x00002d28u, 4);
    /* 80073710 */
    r[8] = 0x800c0000u;
    r[2] = load_8007370c;
    /* 80073714 */
    uint32_t load_80073714 = battle_entry_read(entry, r[8] + 0x00003ea4u, 4);
    /* 80073718 */
    r[3] = 0x800d0000u;
    r[8] = load_80073714;
    /* 8007371C */
    uint32_t load_8007371c = battle_entry_read(entry, r[3] + 0x00003280u, 1);
    /* 80073720 */
    r[2] = r[2] + r[17];
    r[3] = load_8007371c;
    /* 80073724 */
    uint32_t load_80073724 = battle_entry_read(entry, r[2] + 0x00000074u, 1);
    /* 80073728 */
    r[8] = r[19] + r[8];
    r[2] = load_80073724;
    /* 8007372C */
    r[16] = r[2] << 1;
    /* 80073730 */
    r[5] = r[2] << 2;
    /* 80073734 */
    r[5] = r[5] + r[2];
    /* 80073738 */
    r[5] = r[5] << 4;
    /* 8007373C */
    r[2] = r[3] << 1;
    /* 80073740 */
    r[2] = r[2] + r[3];
    /* 80073744 */
    r[2] = r[2] + r[17];
    /* 80073748 */
    r[2] = r[2] << 1;
    /* 8007374C */
    r[2] = r[2] + r[30];
    /* 80073750 */
    uint32_t load_80073750 = battle_entry_read(entry, r[2] + 0x00000000u, 2);
    /* 80073754 */
    r[5] = r[8] + r[5];
    r[6] = load_80073750;
    /* 80073758 */
    r[6] = r[6] + 0x00000048u;
    /* 8007375C */
    r[6] = r[20] + r[6];
    /* 80073760 */
    r[6] = r[6] << 16;
    /* 80073764: fixed control flow */
    r[31] = 0x8007376cu;
    /* 80073768 */
    r[6] = (r[6] >> 16) | ((r[6] & 0x80000000u) ? 0xffff0000u : 0u);
    battle_phase_call(entry, 0x80076a6cu, 0x80073764u);
    /* 8007376C */
    r[4] = 0x800d0000u;
    /* 80073770 */
    uint32_t load_80073770 = battle_entry_read(entry, r[4] + 0x00002d28u, 4);
    /* 80073774 */
    /* delay slot has no register or memory effect */
    r[4] = load_80073770;
    /* 80073778 */
    r[4] = r[4] + r[17];
    /* 8007377C */
    uint32_t load_8007377c = battle_entry_read(entry, r[4] + 0x00000074u, 1);
    /* 80073780 */
    /* delay slot has no register or memory effect */
    r[3] = load_8007377c;
    /* 80073784 */
    r[3] = r[3] + r[2];
    /* 80073788 */
    battle_entry_write(entry, r[4] + 0x00000074u, 1, r[3]);
    /* 8007378C */
    r[2] = 0x800d0000u;
    /* 80073790 */
    uint32_t load_80073790 = battle_entry_read(entry, r[2] + 0x00002d28u, 4);
    /* 80073794 */
    /* delay slot has no register or memory effect */
    r[2] = load_80073790;
    /* 80073798 */
    r[2] = r[2] + r[17];
    /* 8007379C */
    uint32_t load_8007379c = battle_entry_read(entry, r[2] + 0x00000074u, 1);
    /* 800737A0 */
    /* delay slot has no register or memory effect */
    r[2] = load_8007379c;
    /* 800737A4 */
    r[2] = r[2] << 1;
    /* 800737A8 */
    r[2] = (int32_t)r[16] < (int32_t)r[2];
    /* 800737AC: fixed control flow */
    int take_800737ac = r[2] == 0u;
    /* 800737B0 */
    r[18] = r[19] + 0u;
    if (take_800737ac) goto L_80073808;
L_800737b4:;
    /* 800737B4 */
    r[3] = 0x800c0000u;
    /* 800737B8 */
    uint32_t load_800737b8 = battle_entry_read(entry, r[3] + 0x00003ea4u, 4);
    /* 800737BC */
    r[2] = 0x800d0000u;
    r[3] = load_800737b8;
    /* 800737C0 */
    uint32_t load_800737c0 = battle_entry_read(entry, r[2] + 0xffffcb34u, 4);
    /* 800737C4 */
    r[3] = r[18] + r[3];
    r[2] = load_800737c0;
    /* 800737C8 */
    r[2] = r[16] + r[2];
    /* 800737CC */
    r[4] = r[2] << 2;
    /* 800737D0 */
    r[4] = r[4] + r[2];
    /* 800737D4 */
    r[4] = r[4] << 3;
    /* 800737D8: fixed control flow */
    r[31] = 0x800737e0u;
    /* 800737DC */
    r[4] = r[3] + r[4];
    battle_phase_call(entry, 0x80076c34u, 0x800737d8u);
    /* 800737E0 */
    r[2] = 0x800d0000u;
    /* 800737E4 */
    uint32_t load_800737e4 = battle_entry_read(entry, r[2] + 0x00002d28u, 4);
    /* 800737E8 */
    /* delay slot has no register or memory effect */
    r[2] = load_800737e4;
    /* 800737EC */
    r[2] = r[2] + r[17];
    /* 800737F0 */
    uint32_t load_800737f0 = battle_entry_read(entry, r[2] + 0x00000074u, 1);
    /* 800737F4 */
    r[16] = r[16] + 0x00000002u;
    r[2] = load_800737f0;
    /* 800737F8 */
    r[2] = r[2] << 1;
    /* 800737FC */
    r[2] = (int32_t)r[16] < (int32_t)r[2];
    /* 80073800: fixed control flow */
    int take_80073800 = r[2] != 0u;
    /* 80073804 */
    /* delay slot has no register or memory effect */
    if (take_80073800) goto L_800737b4;
L_80073808:;
    /* 80073808 */
    r[2] = 0x800d0000u;
    /* 8007380C */
    uint32_t load_8007380c = battle_entry_read(entry, r[2] + 0x00002d28u, 4);
    /* 80073810 */
    r[3] = 0x800d0000u;
    r[2] = load_8007380c;
    /* 80073814 */
    uint32_t load_80073814 = battle_entry_read(entry, r[3] + 0xffffcb34u, 1);
    /* 80073818 */
    r[2] = r[2] + r[17];
    r[3] = load_80073814;
    /* 8007381C */
    battle_entry_write(entry, r[2] + 0x00000084u, 1, r[3]);
    /* 80073820 */
    r[2] = 0x800d0000u;
    /* 80073824 */
    uint32_t load_80073824 = battle_entry_read(entry, r[2] + 0x00002d28u, 4);
    /* 80073828 */
    r[3] = 0u | 0x1u;
    r[2] = load_80073824;
    /* 8007382C */
    r[2] = r[2] + r[17];
    /* 80073830: fixed control flow */
    /* 80073834 */
    battle_entry_write(entry, r[2] + 0x000000ccu, 1, r[3]);
    goto L_80073a08;
L_80073838:;
    /* 80073838 */
    r[5] = 0u | 0x52u;
    /* 8007383C */
    r[9] = 0u | 0x10u;
    /* 80073840 */
    battle_entry_write(entry, r[29] + 0x00000010u, 4, r[9]);
    /* 80073844 */
    r[9] = 0u | 0x98u;
    /* 80073848 */
    r[7] = 0x800d0000u;
    /* 8007384C */
    uint32_t load_8007384c = battle_entry_read(entry, r[7] + 0xffffcb34u, 4);
    /* 80073850 */
    r[4] = 0x800d0000u;
    r[7] = load_8007384c;
    /* 80073854 */
    uint32_t load_80073854 = battle_entry_read(entry, r[4] + 0x00002f5cu, 4);
    /* 80073858 */
    r[6] = 0x800c0000u;
    r[4] = load_80073854;
    /* 8007385C */
    uint32_t load_8007385c = battle_entry_read(entry, r[6] + 0x00003ea4u, 4);
    /* 80073860 */
    r[16] = 0u | 0xc00u;
    r[6] = load_8007385c;
    /* 80073864 */
    battle_entry_write(entry, r[29] + 0x00000014u, 4, r[9]);
    /* 80073868 */
    battle_entry_write(entry, r[29] + 0x00000018u, 4, r[23]);
    /* 8007386C */
    battle_entry_write(entry, r[29] + 0x0000001cu, 4, r[23]);
    /* 80073870 */
    battle_entry_write(entry, r[29] + 0x00000020u, 4, r[16]);
    /* 80073874: fixed control flow */
    r[31] = 0x8007387cu;
    /* 80073878 */
    r[6] = r[6] + 0x00003768u;
    battle_phase_call(entry, 0x80025fa8u, 0x80073874u);
    /* 8007387C */
    r[3] = 0x800d0000u;
    /* 80073880 */
    uint32_t load_80073880 = battle_entry_read(entry, r[3] + 0x00002d28u, 4);
    /* 80073884 */
    r[5] = 0u | 0x53u;
    r[3] = load_80073880;
    /* 80073888 */
    battle_entry_write(entry, r[3] + 0x00000077u, 1, r[2]);
    /* 8007388C */
    r[7] = 0x800d0000u;
    /* 80073890 */
    uint32_t load_80073890 = battle_entry_read(entry, r[7] + 0xffffcb34u, 4);
    /* 80073894 */
    r[2] = 0x800d0000u;
    r[7] = load_80073890;
    /* 80073898 */
    uint32_t load_80073898 = battle_entry_read(entry, r[2] + 0x00002d28u, 4);
    /* 8007389C */
    r[4] = 0x800d0000u;
    r[2] = load_80073898;
    /* 800738A0 */
    uint32_t load_800738a0 = battle_entry_read(entry, r[4] + 0x00002f5cu, 4);
    /* 800738A4 */
    uint32_t load_800738a4 = battle_entry_read(entry, r[2] + 0x00000077u, 1);
    r[4] = load_800738a0;
    /* 800738A8 */
    r[9] = 0u | 0x10u;
    r[2] = load_800738a4;
    /* 800738AC */
    battle_entry_write(entry, r[29] + 0x00000010u, 4, r[9]);
    /* 800738B0 */
    r[9] = 0u | 0x98u;
    /* 800738B4 */
    battle_entry_write(entry, r[29] + 0x00000014u, 4, r[9]);
    /* 800738B8 */
    battle_entry_write(entry, r[29] + 0x00000018u, 4, r[23]);
    /* 800738BC */
    battle_entry_write(entry, r[29] + 0x0000001cu, 4, r[23]);
    /* 800738C0 */
    battle_entry_write(entry, r[29] + 0x00000020u, 4, r[16]);
    /* 800738C4 */
    r[16] = r[2] << 1;
    /* 800738C8 */
    r[6] = r[2] << 2;
    /* 800738CC */
    r[6] = r[6] + r[2];
    /* 800738D0 */
    r[6] = r[6] << 4;
    /* 800738D4 */
    r[2] = 0x800c0000u;
    /* 800738D8 */
    uint32_t load_800738d8 = battle_entry_read(entry, r[2] + 0x00003ea4u, 4);
    /* 800738DC */
    r[6] = r[6] + 0x00003768u;
    r[2] = load_800738d8;
    /* 800738E0: fixed control flow */
    r[31] = 0x800738e8u;
    /* 800738E4 */
    r[6] = r[6] + r[2];
    battle_phase_call(entry, 0x80025fa8u, 0x800738e0u);
    /* 800738E8 */
    r[4] = 0x800d0000u;
    /* 800738EC */
    uint32_t load_800738ec = battle_entry_read(entry, r[4] + 0x00002d28u, 4);
    /* 800738F0 */
    /* delay slot has no register or memory effect */
    r[4] = load_800738ec;
    /* 800738F4 */
    uint32_t load_800738f4 = battle_entry_read(entry, r[4] + 0x00000077u, 1);
    /* 800738F8 */
    /* delay slot has no register or memory effect */
    r[3] = load_800738f4;
    /* 800738FC */
    r[3] = r[3] + r[2];
    /* 80073900 */
    battle_entry_write(entry, r[4] + 0x00000077u, 1, r[3]);
    /* 80073904 */
    r[2] = 0x800d0000u;
    /* 80073908 */
    uint32_t load_80073908 = battle_entry_read(entry, r[2] + 0x00002d28u, 4);
    /* 8007390C */
    /* delay slot has no register or memory effect */
    r[2] = load_80073908;
    /* 80073910 */
    uint32_t load_80073910 = battle_entry_read(entry, r[2] + 0x00000077u, 1);
    /* 80073914: fixed control flow */
    r[2] = load_80073910;
    /* 80073918 */
    r[2] = r[2] << 1;
    goto L_80073964;
L_8007391c:;
    /* 8007391C */
    r[2] = 0x800d0000u;
    /* 80073920 */
    uint32_t load_80073920 = battle_entry_read(entry, r[2] + 0xffffcb34u, 4);
    /* 80073924 */
    /* delay slot has no register or memory effect */
    r[2] = load_80073920;
    /* 80073928 */
    r[2] = r[16] + r[2];
    /* 8007392C */
    r[4] = r[2] << 2;
    /* 80073930 */
    r[4] = r[4] + r[2];
    /* 80073934 */
    r[4] = r[4] << 3;
    /* 80073938 */
    r[2] = 0x800c0000u;
    /* 8007393C */
    uint32_t load_8007393c = battle_entry_read(entry, r[2] + 0x00003ea4u, 4);
    /* 80073940 */
    r[4] = r[4] + 0x00003768u;
    r[2] = load_8007393c;
    /* 80073944: fixed control flow */
    r[31] = 0x8007394cu;
    /* 80073948 */
    r[4] = r[4] + r[2];
    battle_phase_call(entry, 0x80076c34u, 0x80073944u);
    /* 8007394C */
    r[2] = 0x800d0000u;
    /* 80073950 */
    uint32_t load_80073950 = battle_entry_read(entry, r[2] + 0x00002d28u, 4);
    /* 80073954 */
    /* delay slot has no register or memory effect */
    r[2] = load_80073950;
    /* 80073958 */
    uint32_t load_80073958 = battle_entry_read(entry, r[2] + 0x00000077u, 1);
    /* 8007395C */
    r[16] = r[16] + 0x00000002u;
    r[2] = load_80073958;
    /* 80073960 */
    r[2] = r[2] << 1;
L_80073964:;
    /* 80073964 */
    r[2] = (int32_t)r[16] < (int32_t)r[2];
    /* 80073968: fixed control flow */
    int take_80073968 = r[2] != 0u;
    /* 8007396C */
    /* delay slot has no register or memory effect */
    if (take_80073968) goto L_8007391c;
    /* 80073970 */
    r[3] = 0x800d0000u;
    /* 80073974 */
    uint32_t load_80073974 = battle_entry_read(entry, r[3] + 0x00002d28u, 4);
    /* 80073978 */
    r[2] = 0x800d0000u;
    r[3] = load_80073974;
    /* 8007397C */
    uint32_t load_8007397c = battle_entry_read(entry, r[2] + 0xffffcb34u, 1);
    /* 80073980: fixed control flow */
    r[2] = load_8007397c;
    /* 80073984 */
    battle_entry_write(entry, r[3] + 0x00000087u, 1, r[2]);
    goto L_80073a08;
L_80073988:;
    /* 80073988 */
    uint32_t load_80073988 = battle_entry_read(entry, r[21] + 0x00000000u, 1);
    /* 8007398C */
    /* delay slot has no register or memory effect */
    r[2] = load_80073988;
    /* 80073990: fixed control flow */
    int take_80073990 = r[2] == 0u;
    /* 80073994 */
    r[2] = 0u | 0x7u;
    if (take_80073990) goto L_800739b8;
    /* 80073998 */
    r[1] = 0x800d0000u;
    /* 8007399C */
    r[1] = r[1] + r[17];
    /* 800739A0 */
    uint32_t load_800739a0 = battle_entry_read(entry, r[1] + 0x00002d24u, 1);
    /* 800739A4 */
    /* delay slot has no register or memory effect */
    r[3] = load_800739a0;
    /* 800739A8: fixed control flow */
    int take_800739a8 = r[3] == r[2];
    /* 800739AC */
    /* delay slot has no register or memory effect */
    if (take_800739a8) goto L_800739b8;
    /* 800739B0: fixed control flow */
    /* 800739B4 */
    battle_entry_write(entry, r[5] + 0x000000ccu, 1, 0u);
    goto L_800739bc;
L_800739b8:;
    /* 800739B8 */
    battle_entry_write(entry, r[5] + 0x00000074u, 1, 0u);
L_800739bc:;
    /* 800739BC: fixed control flow */
    r[31] = 0x800739c4u;
    /* 800739C0 */
    r[4] = r[17] + 0u;
    battle_phase_call(entry, 0x800765c4u, 0x800739bcu);
L_800739c4:;
    /* 800739C4: fixed control flow */
    r[31] = 0x800739ccu;
    /* 800739C8 */
    r[4] = r[17] + 0u;
    battle_phase_call(entry, 0x80076710u, 0x800739c4u);
    /* 800739CC */
    uint32_t load_800739cc = battle_entry_read(entry, r[21] + 0x00000000u, 1);
    /* 800739D0 */
    /* delay slot has no register or memory effect */
    r[2] = load_800739cc;
    /* 800739D4: fixed control flow */
    int take_800739d4 = r[2] == 0u;
    /* 800739D8 */
    r[2] = 0u | 0x7u;
    if (take_800739d4) goto L_800739f4;
    /* 800739DC */
    r[1] = 0x800d0000u;
    /* 800739E0 */
    r[1] = r[1] + r[17];
    /* 800739E4 */
    uint32_t load_800739e4 = battle_entry_read(entry, r[1] + 0x00002d24u, 1);
    /* 800739E8 */
    /* delay slot has no register or memory effect */
    r[3] = load_800739e4;
    /* 800739EC: fixed control flow */
    int take_800739ec = r[3] != r[2];
    /* 800739F0 */
    /* delay slot has no register or memory effect */
    if (take_800739ec) goto L_80073a08;
L_800739f4:;
    /* 800739F4 */
    r[2] = 0x800d0000u;
    /* 800739F8 */
    uint32_t load_800739f8 = battle_entry_read(entry, r[2] + 0x00002d28u, 4);
    /* 800739FC */
    /* delay slot has no register or memory effect */
    r[2] = load_800739f8;
    /* 80073A00 */
    r[2] = r[2] + r[17];
    /* 80073A04 */
    battle_entry_write(entry, r[2] + 0x0000007fu, 1, 0u);
L_80073a08:;
    /* 80073A08 */
    r[21] = r[21] + 0x00000008u;
L_80073a0c:;
    /* 80073A0C */
    r[19] = r[19] + 0x00000320u;
    /* 80073A10 */
    r[20] = r[20] + 0x00000060u;
    /* 80073A14 */
    r[17] = r[17] + 0x00000001u;
    /* 80073A18 */
    r[2] = (int32_t)r[17] < 3;
    /* 80073A1C: fixed control flow */
    int take_80073a1c = r[2] != 0u;
    /* 80073A20 */
    r[22] = r[22] + 0x0000001cu;
    if (take_80073a1c) goto L_80073594;
    /* 80073A24 */
    uint32_t load_80073a24 = battle_entry_read(entry, r[29] + 0x0000004cu, 4);
    /* 80073A28 */
    uint32_t load_80073a28 = battle_entry_read(entry, r[29] + 0x00000048u, 4);
    r[31] = load_80073a24;
    /* 80073A2C */
    uint32_t load_80073a2c = battle_entry_read(entry, r[29] + 0x00000044u, 4);
    r[30] = load_80073a28;
    /* 80073A30 */
    uint32_t load_80073a30 = battle_entry_read(entry, r[29] + 0x00000040u, 4);
    r[23] = load_80073a2c;
    /* 80073A34 */
    uint32_t load_80073a34 = battle_entry_read(entry, r[29] + 0x0000003cu, 4);
    r[22] = load_80073a30;
    /* 80073A38 */
    uint32_t load_80073a38 = battle_entry_read(entry, r[29] + 0x00000038u, 4);
    r[21] = load_80073a34;
    /* 80073A3C */
    uint32_t load_80073a3c = battle_entry_read(entry, r[29] + 0x00000034u, 4);
    r[20] = load_80073a38;
    /* 80073A40 */
    uint32_t load_80073a40 = battle_entry_read(entry, r[29] + 0x00000030u, 4);
    r[19] = load_80073a3c;
    /* 80073A44 */
    uint32_t load_80073a44 = battle_entry_read(entry, r[29] + 0x0000002cu, 4);
    r[18] = load_80073a40;
    /* 80073A48 */
    uint32_t load_80073a48 = battle_entry_read(entry, r[29] + 0x00000028u, 4);
    r[17] = load_80073a44;
    /* 80073A4C */
    r[29] = r[29] + 0x00000050u;
    r[16] = load_80073a48;
    /* 80073A50: fixed control flow */
    /* 80073A54 */
    /* delay slot has no register or memory effect */
    return;
}

static const char battle_party_hud_sha[] = "981386c5dbbfcadf79bcac7721207ed8f77f10b4fc41dbc83ec85fca59bd0ad9";
static int battle_party_hud_try_host(BattleMipsRuntime *runtime, PcPortMipsCpu *cpu, uint32_t target)
{
    if (target != 0x80073538u || runtime->force_interpret || !runtime->initialized ||
        g_ActiveBattleRuntime != runtime || runtime->bridge_cpu != cpu ||
        (cpu->load_valid && cpu->load_reg != 0)) return 0;
    const char *control = getenv("XENO_BATTLE_NATIVE_PARTY_HUD");
    if (!control || strcmp(control, "1") != 0) return 0;
    uint8_t digest[32];
    file1_digest(PSX_ADDR(target), 1312, digest);
    for (unsigned i = 0; i < sizeof(digest); ++i)
        if (file1_hex_digit(digest[i] >> 4) != battle_party_hud_sha[2*i] ||
            file1_hex_digit(digest[i] & 15u) != battle_party_hud_sha[2*i+1]) return 0;
    return battle_phase_execute(runtime, cpu, battle_party_hud_run);
}

/* Existing canonical native gates precede this exact-edge legacy boundary. */
static int battle_party_hud_unconverted(uint32_t target, uint32_t return_pc)
{
    static const struct { uint32_t target, return_pc; } edges[] = {
        {0x80076a10u, 0x80073668u},
        {0x80076a6cu, 0x800736e8u},
        {0x80076a6cu, 0x8007376cu},
        {0x80076c34u, 0x800737e0u},
        {0x80025fa8u, 0x8007387cu},
        {0x80025fa8u, 0x800738e8u},
        {0x80076c34u, 0x8007394cu},
        {0x800765c4u, 0x800739c4u},
        {0x80076710u, 0x800739ccu},
    };
    for (unsigned i = 0; i < sizeof(edges) / sizeof(edges[0]); ++i)
        if (target == edges[i].target && return_pc == edges[i].return_pc) return 1;
    return 0;
}
