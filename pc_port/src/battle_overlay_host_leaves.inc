/* Battle overlay functions the port may run as native C instead of
 * interpreting disc/battle.bin.  Hand-maintained; add a name here, then run
 * pc_port/build_port.sh and pc_port/tests/run_battle_overlay_host_differential_test.sh.
 *
 * Names in this comment are left unquoted deliberately:
 * battle_overlay_host_tus.py scrapes "func_..." string literals out of this
 * file, so a quoted name in a comment would be read as an adopted leaf.
 *
 * (func_800BEDE8, mainc124.c, was demoted because its host body handed the raw
 * KSEG0 word at 0x800C3610 to the native HeapFree.  It now translates the word
 * the way translate_argument() does, and it is adopted again, fo/bat4.)
 *
 * (func_8008887C, formerly listed here with an "unexplained" 1-byte
 * divergence at D_800C3A9C, is adopted: func_8001BD40 was an unlinked
 * placeholder in the prover, so both sides stored an undefined return
 * register.  With the real temp3.c body linked it passes every case.)
 *
 * UNPROVEN rather than wrong -- the sweep cannot construct a conclusive case,
 * so adoption would be unverified: func_8008AB94, hitting
 * "unresolved native call target=0x80028470" (func_8008AA40, once listed
 * here with it, is adopted with the func_8008A9C0 closure at the end of
 * this list and proven by the differential's h-battle block, its sfx call
 * through a func_80039DB8 spy); func_80076A10 and
 * func_80076A6C call func_8002675C, which is not yet linked as a real body;
 * func_80076B00, func_80076B68, func_80076BAC and func_80076BF0 were listed
 * here as reaching retail SetSemiTrans at 0x80043BFC, which the differential
 * could not resolve; it now links PsyCross LIBGPU, and their siblings
 * func_80076AC8 and func_80076C34 are adopted (below).
 */
    "func_80070E2C",
    "func_80070EB0",
    "func_8009892C",
    "func_800723E0",
    "func_80071B94",
    "func_800742A0",
    "func_80085350",
    "func_8008ABB8",
    "func_80079934",
    "func_80089B50",
    "func_800A3484",
    "func_800AEEEC",
    "func_800B16A4",
    "func_800B6930",
    "func_800B6990",
    "func_800B69E4",
    "func_800AA898",
    "func_80079ED8",
    "func_8007A280",
    "func_80079E18",
    "func_80079E4C",
    "func_80079054",
    "func_80079098",
    "func_80079114",
    "func_80079E7C",
    "func_8007A628",
    "func_8007A6C8",
    "func_8007A744",
    "func_8007BA88",
    "func_8007BAB8",
    "func_8007E674",
    "func_8007E934",
    "func_8007EED0",
    "func_800BF720",
    "func_8007EEE8",
    "func_8007FCE8",
    "func_80089C08",
    "func_8008AAA0",
    "func_8008B108",
    "func_8008BC40",
    "func_80089BEC",
    "func_800A577C",
    "func_8007EF44",
    "func_8008CCCC",
    "func_8009C0E0",
    "func_8009E268",
    "func_800A578C",
    "func_800A5E9C",
    "func_80077980",
    "func_800AF400",
    "func_800B14B8",
    "func_800B8048",
    "func_800B8054",
    "func_800BC3F8",
    "func_800BC454",
    "func_800BCD8C",
    "func_800BE108",
    "func_800BED30",
    "func_800BF5E8",
    "func_800BF730",
    "func_800BF954",
    "func_800C0F70",
    "func_800B7C28",
    "func_800B89F4",
    "func_80071A08",
    "func_800BF6F8",
    "func_80080AE4",
    "func_80080B64",
    "func_80080C6C",
    "func_80085E78",
    "func_800879A8",
    "func_80088990",
    "func_80089C24",
    "func_80089C48",
    "func_80089C6C",
    "func_80089C9C",
    "func_80099FB0",
    "func_8009A074",
    "func_8009B098",
    "func_8009E364",
    "func_8009E410",
    "func_8009E48C",
    "func_8009413C",
    "func_80094C78",
    "func_800957D8",
    "func_800B168C",
    "func_8009E3C8",
    "func_8009E508",
    "func_8007887C",
    "func_80078D48",
    "func_800B3B6C",
    "func_8008AA74",
    "func_8009CB68",
    "func_8009CA90",
    "func_800B9B30",
    "func_8009795C",
    "func_8009F5B0",
    "func_800B3348",
    "func_800B3350",
    "func_800BAF40",
    "func_800BDD34",
    "func_80097D08",
    "func_8009C050",
    "func_800B00D0",
    "func_800BCB54",
    "func_800BF0B4",
    "func_800A3490",
    "func_800A3514",
    "func_800A3578",
    "func_800A35C8",
    "func_80085388",
    "func_800800E8",
    "func_80085454",
    "func_80085AC4",
    "func_8009AA44",
    "func_8009AB00",
    "func_800AA788",
    "func_800AA7DC",
    "func_800AA79C",
    "func_800885D0",
    "func_800AA760",
    "func_800AA600",
    /* fo/bat2 */
    "func_8008887C",
/* fo/bat1 */
    "func_80078508",
    "func_800785D4",
    "func_800787E0",
    "func_8007893C",
    "func_80078C9C",
    "func_80078CEC",
    "func_800765C4",
    "func_8007FB70",
    "func_8007FBE0",
    "func_8007FDEC",
    /* fo/bat2 (main39/main40) */
    "func_800883AC",
    "func_80087EDC",
    "func_800881B8",
    "func_80088490",
    /* fo/bat3 */
    "func_8009A0DC",
    "func_8009A258",
    "func_8009A2D4",
    "func_8009A7E4",
    "func_8009A854",
    "func_8009A9D0",
    "func_8009AB38",
    "func_8009AC48",
    "func_8009ADA0",
    "func_8009AEFC",
    "func_8009AFD8",
    "func_8009B104",
    "func_8009B1E4",
    "func_8009B46C",
    "func_8009B684",
    "func_8009BAC4",
    "func_8009BD94",
    "func_8009BE0C",
    "func_8009E278",
    "func_8009E2EC",
    "func_8009E53C",
    "func_8009E5C8",
    "func_8009E788",
    "func_8009E868",
    "func_8009C134",
    "func_8009C9C4",
    "func_8009C4B4",
    "func_8009DB54",
    "func_8009D3A0",
    "func_8009DBFC",
    /* fo/bat4 */
    "func_800BE6A0",
    "func_800BE6E8",
    "func_800BF85C",
    "func_800BF8CC",
    "func_800B12D0",
    "func_800B1EA0",
    "func_800BEDE8",
    "func_800BEEB4",
    "func_800BA59C",
    "func_800B9258",
/* Battle guest ABI prototype (M-a; include/battle_guest_abi.h): matched C
 * re-typed at its guest dereferences, built with strict int/pointer errors,
 * proven by the mesh/slot/heap fixtures in the differential harness. */
    "func_800A5A48",
    "func_800A579C",
    "func_800A5D54",
    "func_800A5914",
    "func_800A5BE8",
    "func_800A5870",
    "func_8007AE38",
    "func_8007E8E0",
    "func_8007E154",
    "func_8007FEC4",
/* Codemod sweep batch 1 (guest ABI): ppBoard family, main19/main30. */
    "func_8007AE98",
    "func_8007AFFC",
    "func_8007B040",
    "func_8007B084",
    "func_8007B0C8",
    "func_8007B134",
    "func_8007E954",
    "func_8007E98C",
    "func_8007E9D0",
    "func_8007EAC8",
/* main19: wrap-faithful u16 multiply and the four bare-DIVU siblings. */
    "func_8007AF5C",
    "func_8007AFAC",
    "func_8007B198",
    "func_8007B208",
    "func_8007B264",
/* Codemod sweep batch 2: main30 compare leaves (guest ABI). */
    "func_8007EA08",
    "func_8007EA4C",
    "func_8007EA84",
    "func_8007EB08",
    "func_8007EB50",
    "func_8007EB90",
    "func_8007EBD8",
    "func_8007EC10",
    "func_8007EC54",
    "func_8007EC94",
/* Codemod sweep batch 3: main30 compare leaves (guest ABI). */
    "func_8007ECDC",
    "func_8007ED14",
    "func_8007ED58",
    "func_8007ED98",
    "func_8007EDE0",
    "func_8007EE28",
/* Codemod sweep batch 3 (guest ABI): main22 row copies, main14 board store. */
    "func_8007B98C",
    "func_8007B9C8",
    "func_8007A828",
    "func_8007A7BC",
/* Opening-battle hot set (guest ABI): mainl110 sprite floor query. */
    "func_800BA8F4",
/* Opening-battle hot set: slot-actor and sprite timer callbacks (mainc112,
 * main111 port bodies), primitive semi-transparency setters (main4, main5). */
    "func_800BB13C",
    "func_800BAC50",
    "func_80076AC8",
    "func_80076C34",
/* Opening-battle hot set: main72 motion stepper (port body; its callees stay
 * interpreted through PcPort_BattleMipsCallGuest). */
    "func_800AAA20",
/* Opening-battle hot set: main68 effect-image stepper (port body). */
    "func_800A3E98",
/* Opening-battle hot set: main65 motion-track decoder (lifted port body;
 * func_800A23E8 stays interpreted through the guest-call service). */
    "func_800A0838",
/* 800AAA20's motion callees (lifted port bodies): main73 floor query,
 * main64 hierarchy matrix rebuilds. */
    "func_800AFF9C",
    "func_8009EF3C",
    "func_8009F1C4",
/* Opening-battle hot set: main72 motion-state update (lifted port body;
 * every call through the interpreter's jal path). */
    "func_800AAD54",
/* 800AAA20's per-step event callee (lifted port body; every call through
 * the interpreter's jal path). */
    "func_800AE2A4",
/* Battle frame input dispatch: main43 func_8008A9C0 (called by the native
 * battle frame tick at 0x800BE914 / 0x800BE93C) and its static callee
 * closure -- the D_800C3E4C mode handlers 8008A684 / 8008A3EC (main43 port
 * bodies) and 8008A274 (main42 matched C), main42's texture-strip upload
 * 8008A144 and debug-mode handler 80089CCC (port bodies), the sfx helper
 * 8008AA40 (main43) and the actor wait countdown 8007171C (main.c matched C
 * on typed guest-RAM views).  8008A684's callees at 0x801DF270 / 0x801DF4C0
 * (module at 0x801DE000, no C body) run through BATTLE_GUEST_CALL0.
 * Proven by the differential's h-battle block (scripted pad services,
 * spied sound / pause-letter / sfx / VRAM-copy services, guest stubs at
 * 0x801DF270 / 0x801DF4C0, a shared debug-flag word, required outcome
 * classes per leaf): idle, pad lost and back, pad reset, button commands,
 * Start pause and resume with the D_80059488 save / restore, the paused
 * 4 + 8 resume, the debug commands, the countdown tail with its guest
 * calls, the strip uploads, all 8007171C actor paths.  Against scripted
 * services, not the real controller / SPU code.  Not covered: a call that
 * starts paused and resumes without pausing or losing the pad itself
 * (retail restores an uninitialised saved word, the port bodies the
 * current D_80059488: a documented difference), and 80089CCC's debug-font
 * load (func_8003747C is a placeholder there; the port body passes the raw
 * guest word 0x80200000 to it, a latent issue on a debug-only path). */
    "func_8008A9C0",
    "func_8008A684",
    "func_8008A3EC",
    "func_8008A274",
    "func_8008A144",
    "func_80089CCC",
    "func_8008AA40",
    "func_8007171C",
/* h-lift experiment: lifted port bodies (tools/scripts/battle_lift_asm.py
 * --jal-service, typed W32 thunks; every call through the interpreter's jal
 * path).  80077364 and 800780A8 are interpreted by the opening battle;
 * 80076C78/80076CE8 (frameless) and 800B3B94 (JALR) exercise the new
 * lifter paths. */
    "func_80077364",
    "func_80076C78",
    "func_80076CE8",
    "func_800780A8",
    "func_800B3B94",
/* h-frontier step1: native-only frontier closure (lifted, --jal-service). */
    "func_800BEB04",
    "func_800B8354",
/* h-frontier step2: native-only frontier closure (lifted, --jal-service).
 * 800780A8 and 80077364 belong to this closure but are listed once, in the
 * h-lift block above (321613d2); their bodies are identical.  Step-2
 * evidence for them: 12 proven / 0 inconclusive in O0, O2 and UBSan; the
 * sb slips at 800780F0 and 800773AC are rejected (12 FAIL lines each). */
    "func_8007819C",
    "func_8008F8F4",
    "func_80077454",
    "func_8008F6E4",
    "func_8008DC34",
    "func_8008DE04",
    "func_8008E430",
    "func_8008EA70",
    "func_8008F0A8",
    "func_80076B00",
/* h-frontier step3: native-only frontier closure (lifted, --jal-service). */
    "func_8009F844",
    "func_800A2E88",
    "func_800A7948",
/* h-frontier step4/5: work callback of a slot actor (lifted, --jal-service,
 * typed W32 thunk; mainc112's hand-written BG_* body is guarded out). */
    "func_800BB248",
/* h-hotpath batch 1: hottest interpreted targets of the hybrid battle-1
 * fixture (lifted, --jal-service, typed W32 thunks).  func_800AA514 (main70):
 * the actor-update host's saturating colour scale.  func_800C11CC (main135):
 * the battle sprite animation-script interpreter; its native main-executable
 * caller keeps the dispatching trampoline in game_overrides.c, so the lifted
 * battle copy links as battle_func_800C11CC. */
    "func_800AA514",
    "func_800C11CC",
/* h-hotpath batch 2 (lifted, --jal-service, typed W32 thunks; both lifts
 * live in mainc114.c): func_800BB844 the battle view matrix (in the
 * native-only build the identity-checked target-camera host claims it
 * first), func_800BD810 the glyph quad emitter. */
    "func_800BB844",
    "func_800BD810",
/* h-frontier step7: native-only frontier closure (lifted, --jal-service). */
    "func_800BA984",
/* h-frontier step8: 800BA984's work and free callbacks (lifted, --jal-service). */
    "func_800BAB0C",
    "func_800BABDC",
/* h-hotpath batch 3 (lifted in mainc114.c, --jal-service, typed W32 thunk):
 * func_800BC460 the battle target-chain camera (memset through the bridge's
 * typed C-library path; in the native-only build the identity-checked
 * target-camera host claims it first). */
    "func_800BC460",
/* h-hotpath batch 4 (lifted in mainc114.c, --jal-service, void thunks):
 * main2 party-HUD layout func_80075168 and bars func_80075938 (no host
 * claim; the phase-chain host reaches 80075938 through the bridge). */
    "func_80075168",
    "func_80075938",
/* h-hotpath batch 4: func_800745EC the menu panel prims (no host claim; the
 * phase-chain host reaches it through the bridge). */
    "func_800745EC",
/* h-hotpath batch 4b: 800745EC's AddPrim leaves, so it runs native in native-only. */
    "func_800743A4",
    "func_800744BC",
    "func_80074554",
/* h-frontier step10a: the slot task 801E6A4C queues (lifted, --jal-service). */
    "func_800BB760",
/* h-frontier step11: native-only frontier closure (lifted, --jal-service). */
    "func_800BB6E0",
    "func_800BB690",
    "func_800A9540",
/* Command-menu target highlight (800BCD98 at 80080674): the popup mask
 * latch, its shared timer task (BCC60/BCBB4), the per-slot highlight task
 * (BD098/BCFAC) and the TMD prim conversion BD098 runs (B1720), plus the
 * highlight task's draw (BCEAC, through the native func_800B1F6C of
 * anim_render_index15.inc) and on-free (BD024) callbacks. */
    "func_800BCD98",
    "func_800BCC60",
    "func_800BCBB4",
    "func_800BD098",
    "func_800BCFAC",
    "func_800B1720",
    "func_800BCEAC",
    "func_800BD024",
/* Command-menu state 1 (Attack ring) handler, the shared main35.c body.
 * Its unconverted callees 80084A7C/80077698 (and 800B89FC under 80087A38)
 * are battle_guest_trampolines.c entries, so they abort visibly in
 * native-only instead of hitting silent stubs. */
    "func_8008115C",
/* Attack accept -> command window open (func_80087A38 -> 800B89FC,
 * mainc104_port.c). */
    "func_800B89FC",
/* 800B8D7C (main105.c): stop the archive stream, then 800B8D04.  It was a
 * silent generated stub for main.c/mainc112/mainl110 callers before. */
    "func_800B8D7C",
/* 800BED4C (mainc124_port.c): the command task constructor 800B89FC calls. */
    "func_800BED4C",
/* 800BF3E8 (mainc127_port.c): bind the command's target and face it. */
    "func_800BF3E8",
/* 800BF2B8 (mainc126.c): load the command's effect file, and the SED
 * registration walk 800C1140 (main135_port.c) its sibling 800BF3A4 runs. */
    "func_800BF2B8",
    "func_800C1140",
/* 800B8D04 (mainc104.c): finish the effect load (wait loop on 800BE790,
 * SED registration, buffer free). */
    "func_800B8D04",
/* 80077698 (main7_port.c): the Attack-accept G3 arrow primitives. */
    "func_80077698",
/* Attack-accept target list (main35_port.c): 80084A7C picks the target
 * from the list 800841E0 builds with the 80083FF4 eligibility test. */
    "func_80084A7C",
    "func_800841E0",
    "func_80083FF4",
/* Command-menu text glyph sprites and their fade (main49_port.c). */
    "func_8008D598",
    "func_8008D328",
/* Battle frame-pump phase step 80076544 (main3.c; its phase calls go
 * through the runtime) and phase bodies 800764B4/80076418 (main2.c).
 * Phase 2 (800764EC) still reaches seven asm-only functions. */
    "func_80076544",
    "func_800764B4",
    "func_80076418",
/* Frame draw-list steps (main2_port.c): HUD pulse 80074D4C, panel strips
 * 80073FB8/80074AB8 and their strip linker 800728B8. */
    "func_80074D4C",
    "func_80073FB8",
    "func_80074AB8",
    "func_800728B8",
/* 800B9F78 (mainl110.c port body): the command task callback 800BED4C
 * installs, run by the frame pump 800BE790. */
    "func_800B9F78",
/* Command task callback 800B9F78 callees (mainc107/mainc132/main106/
 * mainc100/mainc127/mainl109/mainc108 _port.c). */
    "func_800B905C",
    "func_800BFA9C",
    "func_800B8EBC",
    "func_800B7E94",
    "func_800BF4F0",
    "func_800B9B54",
    "func_800B9284",
/* Action/animation driver 800B9508 (mainc108_port.c) and the sprite
 * command step 800BF0C4 (mainc126.c port arm). */
    "func_800B9508",
    "func_800BF0C4",
/* Battle command-menu driver 80080160 (main34_q1_port.c); its 16 state
 * handlers and 800716D8/8007FF14 are battle_guest_trampolines.c entries. */
    "func_80080160",
/* Command-menu state-5 handler 80081B58 (mainasm_12068_port.c). */
    "func_80081B58",
