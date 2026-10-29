#!/usr/bin/env bash
# Differential prover for the adopted battle overlay host bodies.
#
# Runs every function in pc_port/src/battle_overlay_host_leaves.inc twice on the
# same guest RAM -- once as the retail MIPS bytes out of disc/battle.bin, once
# as the linked host C body -- and compares the result and the RAM that changed.
# See pc_port/tests/battle_overlay_host_differential_test.c for the contract and
# for what the comparison deliberately does not cover.
#
# Requires the port to have been built once (pc_port/build_native/stubs.c): the
# host bodies' callees and data come from the generated stub manifest, so this
# links the same definitions the shipped binary does.
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
ulimit -c 0
cd "$(dirname "$0")/../.."

STUBS=pc_port/build_native/stubs.c
if [ ! -f "$STUBS" ]; then
    echo "BATTLE OVERLAY DIFFERENTIAL FAIL missing $STUBS" >&2
    echo "  Build the port first: ./pc_port/build_port.sh" >&2
    exit 1
fi

OUT=${BATTLE_OVERLAY_DIFFERENTIAL_OUT:-$(mktemp -d /tmp/xeno-overlay-diff.XXXXXXXX)}
mkdir -p "$OUT"
printf 'BATTLE OVERLAY DIFFERENTIAL artifacts=%s\n' "$OUT"

# The allowlist is the single source of truth for both the names and the TUs.
LEAVES=$(python3 tools/scripts/battle_overlay_host_tus.py --leaves)
TUS=$(python3 tools/scripts/battle_overlay_host_tus.py --verify)
# Battle guest ABI TUs (include/battle_guest_abi.h): compiled with the strict
# int/pointer errors the port build uses for them, and in the mode's own
# regime (O0 = the port's gcc -O0; O2 = gcc -O2; UBSan = clang -O1 with
# -fsanitize=undefined, shift-base included), so the macro-ised matched C is
# itself exercised in all three regimes rather than only at -O0.
ABI_TUS=" $(python3 tools/scripts/battle_overlay_host_tus.py --abi-tus | tr '\n' ' ') "
read -r -a ABI_STRICT <<<"$(python3 tools/scripts/battle_overlay_host_tus.py --abi-strict-flags)"
# TUs with lifted port bodies (include/battle_lift.h; main64/65/72/73): not
# strict-typed (their matched C is the port's ordinary -w C), but compiled in
# the mode's regime like the ABI TUs -- gcc -O0, gcc -O2, clang -O1 with
# -fsanitize=undefined -- so the lifted bodies are proven at each, not only
# at the port's -O0.
LIFT_TUS=" $(python3 tools/scripts/battle_overlay_host_tus.py --lift-tus | tr '\n' ' ') "
python3 tools/scripts/battle_overlay_host_tus.py --check-abi-thunks || {
    echo "BATTLE OVERLAY DIFFERENTIAL FAIL typed bridge thunks disagree with the prototypes" >&2
    exit 1
}
# Every jal of a lifted body must be dispatchable by the port (a main-exe
# callee needs a bound, exported, non-stub native: the main executable's text
# is not in guest RAM, so the interpreter cannot run it).
python3 tools/scripts/check_battle_lift_jal_targets.py || {
    echo "BATTLE OVERLAY DIFFERENTIAL FAIL a lifted body calls a target the port cannot dispatch" >&2
    exit 1
}
python3 tools/scripts/battle_overlay_host_tus.py --leaf-types \
    > "$OUT/overlay_leaf_types.inc"
# Pointer-typed overlay globals: the sweep seeds the guest word with a live RAM
# address for these, or a body that dereferences one faults on the retail side
# and no case can ever prove it.
python3 tools/scripts/battle_overlay_host_tus.py --pointer-globals \
    > "$OUT/overlay_pointer_globals.inc"

# A stale manifest that still stubs an adopted leaf would link two definitions
# of it; say so instead of failing with a wall of duplicate-symbol errors.
for leaf in $LEAVES; do
    if grep -q "\"$leaf\"" "$STUBS"; then
        echo "BATTLE OVERLAY DIFFERENTIAL FAIL $leaf is a generated stub in $STUBS" >&2
        echo "  The manifest is stale; re-run ./pc_port/build_port.sh" >&2
        exit 1
    fi
done

bridge_elf=()
if [ -f build/out/slus_006.64.elf ]; then
    bridge_elf+=(--elf build/out/slus_006.64.elf)
fi
python3 tools/scripts/gen_battle_bridge_map.py "${bridge_elf[@]}" \
    --symbols config/symbol_addrs.slus_006.64.txt \
    --symbols linker/undefined_funcs_auto.battle.txt \
    --symbols linker/undefined_syms_auto.battle.txt \
    --symbols config/symbol_addrs.battle.txt \
    --out "$OUT/battle_bridge_map.inc"

COMMON=(-std=gnu17 -fno-pie -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C
    -DUSE_EXTENDED_PRIM_POINTERS=0 -include assert.h -ffunction-sections -fdata-sections
    -Ipc_port/include_shim -Iinclude -Ipc_port/src -I"$OUT"
    -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
# Must match pc_port/build_port.sh's GFLAGS: the body under test has to be the
# body the port ships.
HOST_FLAGS=(-std=gnu17 -fpermissive -DXENO_PC_PORT -DXENO_FIELD_OBJECT_OVERLAY
    -DXENO_BATTLE_OVERLAY_HOST_BODIES -DSKIP_ASM -D_LANGUAGE_C
    -DUSE_EXTENDED_PRIM_POINTERS=0 -include assert.h -w -O0 -g -m64 -fno-builtin)
HOST_INC=(-Ipc_port/include_shim -Iinclude -I"$OUT"
    -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)

gcc -c "$STUBS" -O0 -g -o "$OUT/stubs.o"

# Main-executable callees the adopted bodies reach, linked from the objects the
# port binary itself links, so both sides run the shipped code instead of a
# placeholder: HeapFree (memory.c), func_8001BD40 (temp3.c), the sprite-list
# builder func_8002675C (temp1f.c; not linked: real execution on sweep RAM
# does not terminate, so the test's h-frontier block defines a spy),
# GetStringEntry/SystemRenderStringEntry
# (system.c), rsin/rcos (PsyCross LIBGTE.C, the
# definitions the runtime's dlsym binds 0x8003F8B0 / 0x8003F8CC to) and the
# libgpu primitive helpers SetPolyFT4/GetTPage/GetClut/SetSemiTrans/...
# (PsyCross LIBGPU.C). Their own unresolved references still get counted
# placeholders below; LIBGPU's display/VRAM entry points (GR_*, PsyX_EndScene)
# therefore mark any case that reaches them inconclusive. rand() is the C library's on both sides here, reseeded
# with srand(1) before each run, so the two sides draw identical sequences.
PORT_REAL_OBJS=()
for obj in pc_port/build_native/obj/src_slus_006.64_system_memory.c.o \
           pc_port/build_native/obj/src_slus_006.64_system_system.c.o \
           pc_port/build_native/obj/src_slus_006.64_system_temp1.c.o \
           pc_port/build_native/obj/src_slus_006.64_system_temp3.c.o; do
    if [ ! -f "$obj" ]; then
        echo "BATTLE OVERLAY DIFFERENTIAL FAIL missing $obj (build the port first)" >&2
        exit 1
    fi
    PORT_REAL_OBJS+=("$obj")
done
# GTE for the battle guest ABI bodies (OuterProduct0, VectorNormal,
# OuterProduct12, reached by the main69.c mesh family): the port's own
# pc_port/src/psyq_compat.c object (only those three exported), over
# PsyCross's real GTE register file (PsyX_GTE.cpp) instead of the aborting
# MFC2/MTC2 placeholders.  Both sides reach the same native helpers, so this
# proves the battle C, not GTE fidelity (run_battle_gte_retail_test.sh does).
if [ ! -f pc_port/build_native/obj/psyq_compat.c.o ]; then
    echo "BATTLE OVERLAY DIFFERENTIAL FAIL missing psyq_compat.c.o (build the port first)" >&2
    exit 1
fi
# Only the three GTE helpers stay global; every other psyq_compat definition
# (rand, Vsync, ReadTIM, the pad hooks, ...) is localised, so the rest of the
# harness resolves exactly as before this object was linked.
objcopy --keep-global-symbol=OuterProduct0 --keep-global-symbol=OuterProduct12 \
    --keep-global-symbol=VectorNormal \
    pc_port/build_native/obj/psyq_compat.c.o "$OUT/psyq_compat_gte.o"
for gte_src in psx/INLINE_C.C gte/PsyX_GTE.cpp; do
    g++ -std=c++17 -fno-pie -O0 -fpermissive -w -DUSE_EXTENDED_PRIM_POINTERS=0 \
        -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx \
        -include pc_port/src/port_compat.h \
        -c "pc_port/extern/PsyCross/src/$gte_src" -o "$OUT/gte_${gte_src##*/}.o"
done
PORT_REAL_OBJS+=("$OUT/psyq_compat_gte.o" "$OUT/gte_INLINE_C.C.o" "$OUT/gte_PsyX_GTE.cpp.o")
PSYLIB=$(find pc_port/build -name libpsycross.a 2>/dev/null | head -1)
if [ -z "$PSYLIB" ]; then
    echo "BATTLE OVERLAY DIFFERENTIAL FAIL missing libpsycross.a (build the port first)" >&2
    exit 1
fi
mkdir -p "$OUT/psycross"
PSYLIB_ABS="$PWD/$PSYLIB"
(cd "$OUT/psycross" && ar x "$PSYLIB_ABS" LIBGTE.C.o LIBGPU.C.o)
# h-frontier: the test wraps DrawSync (a spy in the module-only scope, this
# definition everywhere else), so LIBGPU's own is renamed for it.
objcopy --redefine-sym DrawSync=prover_real_DrawSync "$OUT/psycross/LIBGPU.C.o"
PORT_REAL_OBJS+=("$OUT/psycross/LIBGTE.C.o" "$OUT/psycross/LIBGPU.C.o")
# h-frontier: the port's own packed work list (work_list_port.c), for the
# task-callback cases only.  Every global it defines or references is renamed
# prover_wl_* (except g_PsxRam, the heap, the C library and the runtime's
# callback dispatch), so nothing else here resolves to it and the generated
# placeholders do not change; the test's h-frontier block defines the renamed
# state it references and runs its real WorkListsAddTasks / update pumps only
# while those cases run.
WORK_LIST_OBJ=pc_port/build_native/obj/work_list_port.c.o
if [ ! -f "$WORK_LIST_OBJ" ]; then
    echo "BATTLE OVERLAY DIFFERENTIAL FAIL missing $WORK_LIST_OBJ (build the port first)" >&2
    exit 1
fi
nm -g "$WORK_LIST_OBJ" | awk '{print $NF}' | sort -u \
    | grep -vxE 'g_PsxRam|HeapAlloc|HeapFree|PcPort_BattleMipsDispatchCallback|abort|fprintf|stderr' \
    | awk '{print $1 " prover_wl_" $1}' > "$OUT/work_list_port.syms"
# The registered-jalr fixture must exercise the real unaligned callback.
# A4-aligned .text base preserves the GCC callback's odd offset instead of
# letting unrelated test layout happen to align it. Production object unchanged.
objcopy --set-section-alignment .text=4 --redefine-syms="$OUT/work_list_port.syms" "$WORK_LIST_OBJ" "$OUT/work_list_port_prover.o"
PORT_REAL_OBJS+=("$OUT/work_list_port_prover.o")

# main12.c is the mutation target for the negative control: it owns
# func_80079934, whose whole body is `*pValue += 4`.
python3 - "$OUT" <<'PY'
from pathlib import Path
import sys
out = Path(sys.argv[1])
src = Path("src/battle/main12.c").read_text()
old = "*pValue += 4;"
assert src.count(old) == 1, src.count(old)
(out / "mutant_main12.c").write_text(src.replace(old, "*pValue += 5;", 1))
# Second control: the store-path return rule (func_80079ED8's defined port
# return) must reject a body that returns anything else there.
src = Path("src/battle/main14.c").read_text()
old = "        ret = 0;\n#endif"
assert src.count(old) == 2, src.count(old)
(out / "mutant_main14.c").write_text(src.replace(old, "        ret = 1;\n#endif", 1))
# Third control (battle guest ABI): func_800A5A48 with its inside result
# inverted must be rejected on the mesh fixture.
src = Path("src/battle/main69.c").read_text()
old = "return ~cross.vy >> 31;"
assert src.count(old) == 1, src.count(old)
(out / "mutant_main69.c").write_text(src.replace(old, "return cross.vy >> 31;", 1))
# One mutant per further guest-ABI leaf, in two builds (a mutated callee is
# never in the same build as its mutated caller).  Each leaf must FAIL.
mutants = {
    "a": [("main69.c", "            return i;\n", "            return i + 1;\n"),       # 800A579C
          ("main69.c", "return tri;", "return tri ^ 1;"),                            # 800A5D54
          ("main69.c", "p->vy = 0;", "p->vy = 1;"),                                  # 800A5BE8
          ("main19.c", "s32 v = pRow[p[1]] + pRow[p[2]];", "s32 v = pRow[p[1]] + pRow[p[2]] + 1;"),  # 8007AE38
          ("main30.c", ")) + 1);", ")) + 2);"),                                     # 8007E8E0
          ("main33.c", "= 0xA0;", "= 0xA1;")],                                       # 8007FEC4
    "b": [("main69.c", "D_800D2F64 = 1;", "D_800D2F64 = 2;"),                        # 800A5914
          ("main69.c", "+ 0xC);", "+ 0xB);"),                                        # 800A5870
          ("main28.c", "D_800C3EB7[idx * 0x1C] = ", "D_800C3EB7[idx * 0x1C + 1] = ")],  # 8007E154
    # Codemod sweep batch 1 (ppBoard family): independent leaves, one build.
    "c": [("main19.c", "s32 v = pRow[p[1]] - pRow[p[2]];", "s32 v = pRow[p[1]] - pRow[p[2]] - 1;"),  # 8007AE98
          ("main19.c", "row[p[1]] & row[p[2]]", "row[p[1]] & row[p[1]]"),                    # 8007AFFC
          ("main19.c", "row[p[1]] | row[p[2]]", "row[p[1]] | row[p[2]] | 1"),                # 8007B040
          ("main19.c", "row[p[1]] ^ row[p[2]];", "row[p[1]] ^ row[p[2]] ^ 2;"),              # 8007B084
          ("main19.c", "s32 v = row[p[1]] + row[p[2]];", "s32 v = row[p[1]] + row[p[2]] + 1;"),  # 8007B0C8
          ("main19.c", "s32 v = row[p[1]] - row[p[2]];", "s32 v = row[p[1]] - row[p[2]] - 1;"),  # 8007B134
          ("main30.c", "return (pRow[p[1]] ^ p[2]) == 0;", "return (pRow[p[1]] ^ p[2]) != 0;"),  # 8007E954
          ("main30.c", "^ (i2 | i3s)) == 0;", "^ (i2 | i3)) == 0;"),                          # 8007E98C
          ("main30.c", "return (u32)p[2] >= pRow[p[1]];", "return (u32)p[2] > pRow[p[1]];"),  # 8007E9D0
          ("main30.c", "return (pRow[p[1]] ^ pRow[p[2]]) == 0;", "return (pRow[p[1]] ^ pRow[p[2]]) != 0;")],  # 8007EAC8
    # Wrap-faithful multiply and bare-DIVU siblings (main19). Each mutant is
    # the plausible wrong port: a clamp that treats the wrapped product as
    # unsigned, and a C-style "guarded" division that returns 0 on a zero
    # divisor instead of retail DIVU's LO/HI.
    "d": [("main19.c", "GMUL32(row[p[1]], row[p[2]]);\n\n    if (v > 0xFFFF)",
           "GMUL32(row[p[1]], row[p[2]]);\n\n    if ((u32)v > 0xFFFF)"),                     # 8007B198
          ("main19.c", "GDIVU(pRow[p[1]], pRow[p[2]])", "(pRow[p[2]] ? pRow[p[1]] / pRow[p[2]] : 0)"),  # 8007AF5C
          ("main19.c", "GREMU(pRow[p[1]], pRow[p[2]])", "(pRow[p[2]] ? pRow[p[1]] % pRow[p[2]] : 0)"),  # 8007AFAC
          ("main19.c", "GDIVU(row[p[1]], row[p[2]])", "(row[p[2]] ? row[p[1]] / row[p[2]] : 0)"),      # 8007B208
          ("main19.c", "GREMU(row[p[1]], row[p[2]])", "(row[p[2]] ? row[p[1]] % row[p[2]] : 0)")],     # 8007B264
    # Codemod sweep batch 2: main30 compare leaves: one mutant per leaf.
    "e": [
        ("main30.c", 'return (i2 | i3s) >= a;', 'return (i2 | i3s) > a;'),
        ("main30.c", 'return pRow[p[1]] >= (u32)p[2];', 'return pRow[p[1]] > (u32)p[2];'),
        ("main30.c", 'return a >= (i2 | i3s);', 'return a > (i2 | i3s);'),
        ("main30.c", '    u32 b = *GPTR(u16, (i2 << 1) + row);\n\n    return (a ^ b) == 0;', '    u32 b = *GPTR(u16, (i2 << 1) + row);\n\n    return (a ^ b) != 0;'),
        ("main30.c", '    u32 b = pRow[p[2]];\n\n    return b >= a;', '    u32 b = pRow[p[2]];\n\n    return b > a;'),
        ("main30.c", '    u32 b = *GPTR(u16, (i2 << 1) + row);\n\n    return b >= a;\n}\n/* func_8007EBD8', '    u32 b = *GPTR(u16, (i2 << 1) + row);\n\n    return b > a;\n}\n/* func_8007EBD8'),
        ("main30.c", '    u32 b = p[2];\n\n    return (a & b) != 0;', '    u32 b = p[2];\n\n    return (a | b) != 0;'),
        ("main30.c", 'return (a & (i2 + i3s)) != 0;', 'return (a & (i2 + i3)) != 0;'),
        ("main30.c", '    u32 b = pRow[p[2]];\n\n    return (a & b) != 0;', '    u32 b = pRow[p[2]];\n\n    return (a & b) == 0;'),
        ("main30.c", '    u32 b = *GPTR(u16, (i2 << 1) + row);\n\n    return (a & b) != 0;', '    u32 b = *GPTR(u16, (i2 << 1) + row);\n\n    return (a & b) == 0;')],
    # Codemod sweep batch 3: main30 compare leaves: one mutant per leaf.
    "f": [
        ("main30.c", 'return (pRow[p[1]] ^ p[2]) != 0;', 'return (pRow[p[1]] ^ p[2] ^ 1) != 0;'),
        ("main30.c", 'return (a ^ (i2 | i3s)) != 0;', 'return (a ^ (i2 | i3)) != 0;'),
        ("main30.c", 'return (pRow[p[1]] ^ pRow[p[2]]) != 0;', 'return (pRow[p[1]] ^ pRow[p[2]]) == 0;'),
        ("main30.c", '    u32 b = *GPTR(u16, (i2 << 1) + row);\n\n    return (a ^ b) != 0;', '    u32 b = *GPTR(u16, (i2 << 1) + row);\n\n    return (a ^ b) == 0;'),
        ("main30.c", '    u32 b = *GPTR(u32, (i2 << 2) + row);\n\n    return (a ^ b) == 0;', '    u32 b = *GPTR(u32, (i2 << 2) + row);\n\n    return (a ^ b) != 0;'),
        ("main30.c", '    u32 b = *GPTR(u32, (i2 << 2) + row);\n\n    return b >= a;', '    u32 b = *GPTR(u32, (i2 << 2) + row);\n\n    return (s32)b >= (s32)a;'),
        ("main22.c", "BattleCopyRow16(idx);\n\n    t[p[2]] = t[p[1]];", "BattleCopyRow16(idx);\n\n    t[p[2]] = t[p[1]] + 1;"),
        ("main22.c", "BattleCopyRow32(idx);\n\n    t[p[2]] = t[p[1]];", "BattleCopyRow32(idx);\n\n    t[p[2]] = t[p[1]] + 1;"),
        ("main14.c", "v = a2 + 1;", "v = a2 + 2;"),
        ("main14.c", "a1[a3 * 8 + 3] = (GLOAD(u8, a0))[2];", "a1[a3 * 8 + 3] = (GLOAD(u8, a0))[1];")],
    # Opening-battle hot set: one mutant per leaf, each must FAIL.
    "g": [
        ("mainl110.c", "*(u32*)(s + 0x78) = (u32)r;", "*(u32*)(s + 0x78) = (u32)(r + 1);"),
        ("mainc112.c", "BG_U32(actor + 8) = BG_U32(BG_U32(model + 4) + 0x64) << 16;",
         "BG_U32(actor + 8) = BG_U32(BG_U32(model + 4) + 0x64) << 15;"),
        ("main111.c", "u32 sprite = *(u32*)(task + 4);\n\n    if (BG_U8(0x800C3664u) != 0) {",
         "u32 sprite = *(u32*)(task + 4);\n\n    if (BG_U8(0x800C3664u) > 1) {"),
        ("main4.c", "SetShadeTex(p, 0);", "SetShadeTex(p, 1);"),
        ("main5.c", "    p[6] = 0x40;\n    *(u16*)(p + 0x16) |= 0x40;",
         "    p[6] = 0x40;\n    *(u16*)(p + 0x16) |= 0x41;"),
        ("main72.c", "    return (s32)flags;\n}", "    return (s32)(flags | 0x10);\n}"),
        ("main68.c", "    *(u16*)(e + 0x18) = raw;", "    *(u16*)(e + 0x18) = (u16)(raw ^ 1);")],
    # Lifted port bodies (include/battle_lift.h): callees of 800AAA20, so in
    # their own build.  Each mutant is a one-instruction slip.
    "h": [("main65.c", "r_s5 = r_s5 | 0x100u; /* 800A101C ori", "r_s5 = r_s5 | 0x180u; /* 800A101C ori"),
          ("main73.c", "bl_sw(r_v1 + 0x60u, r_v0); /* 800B0044 sw", "bl_sw(r_v1 + 0x60u, r_v0 + 1u); /* 800B0044 sw"),
          ("main64.c", "bl_sw(r_s2 + 0x20u, r_v0); /* 8009F040 sw", "bl_sw(r_s2 + 0x20u, r_v0 ^ 1u); /* 8009F040 sw"),
          ("main64.c", "bl_sw(r_s3 + 0x20u, r_v0); /* 8009F2CC sw", "bl_sw(r_s3 + 0x20u, r_v0 ^ 1u); /* 8009F2CC sw")],
    # 800AAD54 calls 8009EF3C/8009F1C4 (mutated in h): its own build, with
    # 800AE2A4 (neither calls the other).
    "i": [("main72.c", "bl_sh(r_s4 + 0x70u, r_v0); /* 800AADD8 sh", "bl_sh(r_s4 + 0x70u, r_v0 + 1u); /* 800AADD8 sh"),
          # 800AE2A4: the consumed-event cursor advance off by two.
          ("main72.c", "bl_sw(r_s3 + 0xA0u, r_v0); /* 800AEE58 sw", "bl_sw(r_s3 + 0xA0u, r_v0 + 2u); /* 800AEE58 sw")],
    # Spy control: 800AE2A4 passing a wrong sound id to the spied
    # func_80039E60 changes no RAM; only the spied call sequence shows it.
    "j": [("main72.c", "    /* 800AE9F0 jal func_80039E60 */\n    r_a0 = r_a0 + r_s4;",
           "    /* 800AE9F0 jal func_80039E60 */\n    r_a0 = r_a0 + r_s4 + 1u;")],
}
for group, edits in mutants.items():
    d = out / ("mutants-" + group)
    d.mkdir(exist_ok=True)
    texts = {}
    for tu, old, new in edits:
        text = texts.get(tu) or Path("src/battle/" + tu).read_text()
        assert text.count(old) == 1, (tu, old, text.count(old))
        texts[tu] = text.replace(old, new, 1)
    for tu, text in texts.items():
        (d / tu).write_text(text)
PY

mode_flags() {
    case "$1" in
        O0)    echo "-O0" ;;
        O2)    echo "-O2" ;;
        UBSan) echo "-O1 -fsanitize=undefined -fno-sanitize-recover=all -fno-sanitize=function" ;;
    esac
}

# Main-executable services the adopted lifted bodies (800AAD54, 800AE2A4)
# reach that this harness does not link (archive, sound, VRAM image upload).
# Instead of an inconclusive placeholder each gets a SPY: it does nothing,
# returns the fixed value, and records its name and its first <argc> argument
# words (prover_spy in the test).  Both sides reach the same spy through the
# same bridge, and compare() requires the two call sequences -- names, order,
# arguments -- to be identical.  name:argc:return.
SPIES=(
    ArchiveDataSync:0:0
    ArchiveDecodeAlignedSize:1:0x800
    ArchiveGetArchiveOffsetIndices:2:0
    ArchiveReadFileToBuffer:4:0
    ArchiveSetIndex:2:0
    SoundAddSedsEntry:1:0
    func_80039E60:1:0
    func_8003A2E4:2:0
    func_8002A498:1:0
    func_8002DDE4:7:0
    # h-battle (func_8008A9C0 closure): the battle input handlers' pad reset,
    # pause-letter, sfx (8008AA40) and debug-font services.  The services
    # whose results or side effects a case scripts -- ControllerGetType,
    # ControllerPopState, func_80036410, SoundMute/EnableAllSpuChannels,
    # GR_CopyVRAM -- are spies defined in the test's h-battle block.
    # func_8003747C (80089CCC's debug-font buffer, a path not provable here)
    # stays a placeholder: HeapReset calls it from the 8008ABB8 / 8007FEC4
    # pre_side heap set-up, which compare() does not exclude from the host
    # side's spy log.
    ControllerResetState:0:0
    GraphicsDrawPauseLetters:2:0
    func_80039DB8:1:0
    FontLoadFont:11:0
)

# build_link <mode> <host-object-dir> <main12 source> <output suffix>
build_link() {
    local mode="$1" hostdir="$2" main12="$3" suffix="${4:-}"
    local main14="${5:-src/battle/main14.c}"
    local main69="${6:-src/battle/main69.c}"
    local mutant_dir="${7:-}"
    local test_defs="${8:-}"
    local flags; flags=$(mode_flags "$mode")
    local target="$OUT/$mode$suffix"
    local err="$OUT/$mode$suffix-bridge.err"
    local stub_c="$OUT/$mode$suffix-bridge.c"

    rm -rf "$hostdir"; mkdir -p "$hostdir"
    for tu in $TUS; do
        local src="$tu"
        [ "$tu" = "src/battle/main12.c" ] && src="$main12"
        [ "$tu" = "src/battle/main14.c" ] && src="$main14"
        [ "$tu" = "src/battle/main69.c" ] && src="$main69"
        if [ -n "$mutant_dir" ] && [ -f "$mutant_dir/$(basename "$tu")" ]; then
            src="$mutant_dir/$(basename "$tu")"
        fi
        case "$ABI_TUS" in
            *" $tu "*)
                local abi_host=()
                local f
                for f in "${HOST_FLAGS[@]}"; do
                    case "$f" in -w|-fpermissive|-O0) ;; *) abi_host+=("$f") ;; esac
                done
                local cc=gcc
                [ "$mode" = UBSan ] && cc=clang
                # shellcheck disable=SC2086
                "$cc" -c "$src" "${abi_host[@]}" $flags "${ABI_STRICT[@]}" \
                    -I"$(dirname "$tu")" "${HOST_INC[@]}" \
                    -o "$hostdir/$(basename "$tu" .c).o"
                continue ;;
        esac
        case "$LIFT_TUS" in
            *" $tu "*)
                local lift_host=()
                local lf
                for lf in "${HOST_FLAGS[@]}"; do
                    case "$lf" in -O0) ;; -fpermissive) [ "$mode" = UBSan ] || lift_host+=("$lf") ;; *) lift_host+=("$lf") ;; esac
                done
                local lcc=gcc
                [ "$mode" = UBSan ] && lcc=clang
                # shellcheck disable=SC2086
                "$lcc" -c "$src" "${lift_host[@]}" $flags \
                    -I"$(dirname "$tu")" "${HOST_INC[@]}" \
                    -o "$hostdir/$(basename "$tu" .c).o"
                continue ;;
        esac
        gcc -c "$src" "${HOST_FLAGS[@]}" "${HOST_INC[@]}" \
            -o "$hostdir/$(basename "$tu" .c).o"
    done
    # shellcheck disable=SC2086
    clang "${COMMON[@]}" $flags -Wall -Wextra -Werror $test_defs \
        -DBATTLE_RUNTIME_SOURCE="\"$PWD/pc_port/src/battle_mips_runtime.c\"" -DPROVER_REAL_GTE \
        -c pc_port/tests/battle_overlay_host_differential_test.c \
        -o "$OUT/$mode.test.o"
    clang "${COMMON[@]}" $flags -Wall -Wextra -Werror \
        -c pc_port/src/battle_mips_adapter.c -o "$OUT/$mode.cpu.o"

    # Some callees of an adopted body are resolved by the port from PsyCross or
    # from game translation units this harness does not link. Rather than let the
    # link die, give each one a placeholder that fails loudly: a leaf that
    # reaches one is not provable here, and the run must say so instead of
    # reporting a match. The set comes from the linker itself, so nothing the C
    # library already provides gets a colliding second definition. The probe
    # link reports mangled names so C++ callees (PsyCross) get placeholders too.
    local extra=()
    if ! clang -no-pie -rdynamic $flags -Wl,--gc-sections -Wl,--no-demangle \
        "$OUT/$mode.test.o" "$OUT/$mode.cpu.o" "$hostdir"/*.o "${PORT_REAL_OBJS[@]}" "$OUT/stubs.o" \
        -ldl -lstdc++ -o "$target" 2>"$err"; then
        local missing
        missing=$(grep -oE "undefined reference to \`[A-Za-z0-9_]+'" "$err" \
            | sed "s/.*\`//; s/'//" | sort -u)
        if [ -z "$missing" ]; then
            cat "$err" >&2
            echo "BATTLE OVERLAY DIFFERENTIAL FAIL $mode link failed (no undefined refs)" >&2
            exit 1
        fi
        {
            echo '/* Generated by run_battle_overlay_host_differential_test.sh.'
            echo ' * Placeholders for callees only PsyCross or a non-linked game TU'
            echo ' * provides. The interpreted side resolves the same name through the'
            echo ' * bridge map and dlsym, so it reaches these placeholders too and both'
            echo ' * sides would compare an undefined return register. Every call is'
            echo ' * therefore counted in g_prover_placeholder_hits, and the prover'
            echo ' * treats any case that reached one as inconclusive, never as a match;'
            echo ' * a leaf with no proven case is reported UNPROVEN and fails the run. */'
            echo '#include <stdio.h>'
            echo 'extern unsigned g_prover_placeholder_hits;'
            echo 'extern unsigned long long prover_spy(const char *name, unsigned argc,'
            echo '                                     const unsigned long long *args, unsigned ret);'
            echo 'static void unresolved(const char *name) {'
            echo '    static const char *seen[256];'
            echo '    static int count;'
            echo '    int i;'
            echo '    g_prover_placeholder_hits++;'
            echo '    for (i = 0; i < count; i++) if (seen[i] == name) return;'
            echo '    if (count < 256) seen[count++] = name;'
            echo '    fprintf(stderr, "[prover] unavailable callee %s (case inconclusive)\n", name);'
            echo '}'
            for sym in $missing; do
                spy=""
                for s in "${SPIES[@]}"; do
                    [ "${s%%:*}" = "$sym" ] && spy="$s"
                done
                if [ -n "$spy" ]; then
                    # name:argc:return -- see SPIES above.
                    rest="${spy#*:}"
                    echo "unsigned long long $sym(unsigned long long a0, unsigned long long a1,"
                    echo "    unsigned long long a2, unsigned long long a3, unsigned long long a4,"
                    echo "    unsigned long long a5, unsigned long long a6, unsigned long long a7,"
                    echo "    unsigned long long a8, unsigned long long a9, unsigned long long a10,"
                    echo "    unsigned long long a11) {"
                    echo "    unsigned long long a[12] = {a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11};"
                    echo "    return prover_spy(\"$sym\", ${rest%%:*}u, a, ${rest#*:}u);"
                    echo "}"
                    continue
                fi
                case "$sym" in
                    D_*|g_*|jtbl_*|jpt_*|str_*|aD_*|aG_*|gteRegs|activeDispEnv|activeDrawEnv|currentDispEnv)
                        # 64 KiB, not 4 KiB: the runtime binds a shared
                        # global by its retail size (g_GameState is widened
                        # to 0x2358 in initialize_runtime), so a smaller
                        # placeholder let an interpreted read of
                        # g_GameState+0x22B6 run off the end of BSS.
                        echo "unsigned char $sym[0x10000];" ;;
                    *)
                        echo "void $sym(void) { unresolved(\"$sym\"); }" ;;
                esac
            done
        } > "$stub_c"
        clang -c "$stub_c" -O0 -o "$OUT/$mode$suffix-bridge.o"
        extra=("$OUT/$mode$suffix-bridge.o")
        printf 'BATTLE OVERLAY DIFFERENTIAL bridge-placeholders=%s\n' \
            "$(echo "$missing" | wc -l)"
    fi
    # shellcheck disable=SC2086
    clang -no-pie -rdynamic $flags -Wl,--gc-sections \
        "$OUT/$mode.test.o" "$OUT/$mode.cpu.o" "$hostdir"/*.o "${PORT_REAL_OBJS[@]}" "$OUT/stubs.o" \
        "${extra[@]}" -ldl -lstdc++ -o "$target"
}

# build_link <mode> <host-object-dir> <main12 source>
# BATTLE_OVERLAY_DIFFERENTIAL_MUTANT_DIR (verification only): a directory of
# src/battle TU copies under their own file names.  The mode runs compile
# those instead of src/battle's, so a deliberate mutant is judged against the
# whole allowlist without editing the tree.  Unset (the default): no change.
for mode in ${BATTLE_OVERLAY_DIFFERENTIAL_MODES:-O0 O2 UBSan}; do
    build_link "$mode" "$OUT/host-$mode" "src/battle/main12.c" "" \
        src/battle/main14.c src/battle/main69.c "${BATTLE_OVERLAY_DIFFERENTIAL_MUTANT_DIR:-}"
    status=0
    "$OUT/$mode" >"$OUT/$mode.log" 2>&1 || status=$?
    cat "$OUT/$mode.log"
    if [ "$status" -ne 0 ] || ! grep -q '^BATTLE OVERLAY DIFFERENTIAL PASS ' "$OUT/$mode.log"; then
        echo "BATTLE OVERLAY DIFFERENTIAL FAIL mode=$mode status=$status" >&2
        exit 1
    fi
done

# h-frontier: module-only fixtures have their own two separately run mutants.
# Battle-only controls mutate leaves excluded from that explicitly selected scope.
if [ "${BATTLE_OVERLAY_DIFFERENTIAL_MODULE0C4_ONLY:-0}" = "1" ]; then
    echo "BATTLE MODULE0C4 DIFFERENTIAL PASS (module-only; battle controls not run)"
    exit 0
fi

# BATTLE_OVERLAY_DIFFERENTIAL_CONTROLS=0 (verification only): stop after the
# mode runs, without the negative controls below.  The default runs them.
if [ "${BATTLE_OVERLAY_DIFFERENTIAL_CONTROLS:-1}" = "0" ]; then
    echo "BATTLE OVERLAY DIFFERENTIAL modes passed; negative controls skipped (BATTLE_OVERLAY_DIFFERENTIAL_CONTROLS=0)"
    exit 0
fi

# Negative control: a body that returns the wrong value must be rejected.
build_link O2 "$OUT/host-mutant" "$OUT/mutant_main12.c" "-mutant"
status=0
"$OUT/O2-mutant" >"$OUT/mutant.log" 2>&1 || status=$?
if [ "$status" -eq 0 ] || ! grep -q '^BATTLE OVERLAY DIFFERENTIAL FAIL ' "$OUT/mutant.log"; then
    cat "$OUT/mutant.log" >&2
    echo "BATTLE OVERLAY DIFFERENTIAL CONTROL FAILURE wrong-result body accepted" >&2
    exit 1
fi
grep -m1 '^BATTLE OVERLAY DIFFERENTIAL FAIL ' "$OUT/mutant.log" >&2

# Negative control 2: a store path that returns a nonzero value must be rejected.
build_link O2 "$OUT/host-mutant-store" src/battle/main12.c "-mutant-store" "$OUT/mutant_main14.c"
status=0
"$OUT/O2-mutant-store" >"$OUT/mutant-store.log" 2>&1 || status=$?
if [ "$status" -eq 0 ] || ! grep -q 'store-path v0' "$OUT/mutant-store.log"; then
    cat "$OUT/mutant-store.log" >&2
    echo "BATTLE OVERLAY DIFFERENTIAL CONTROL FAILURE store-path return rule accepted a nonzero return" >&2
    exit 1
fi
grep -m1 'store-path v0' "$OUT/mutant-store.log" >&2
# Negative control 3 (battle guest ABI): a macro-ised main69.c body with an
# inverted result must be rejected by the mesh fixture.
build_link O2 "$OUT/host-mutant-abi" src/battle/main12.c "-mutant-abi" src/battle/main14.c "$OUT/mutant_main69.c"
status=0
"$OUT/O2-mutant-abi" >"$OUT/mutant-abi.log" 2>&1 || status=$?
if [ "$status" -eq 0 ] || ! grep -q '^BATTLE OVERLAY DIFFERENTIAL FAIL func_800A5A48 ' "$OUT/mutant-abi.log"; then
    cat "$OUT/mutant-abi.log" >&2
    echo "BATTLE OVERLAY DIFFERENTIAL CONTROL FAILURE inverted func_800A5A48 accepted" >&2
    exit 1
fi
grep -m1 '^BATTLE OVERLAY DIFFERENTIAL FAIL func_800A5A48 ' "$OUT/mutant-abi.log" >&2
# The indirect (bg_jalr) case must see the mutant too: that is what shows the
# guest-address -> native map runs the native body rather than the interpreter.
if ! grep -q '^BATTLE OVERLAY DIFFERENTIAL FAIL func_800A5A48 indirect ' "$OUT/mutant-abi.log"; then
    echo "BATTLE OVERLAY DIFFERENTIAL CONTROL FAILURE indirect call did not reach the mutated native body" >&2
    exit 1
fi
grep -m1 'indirect-native' "$OUT/mutant-abi.log" >&2
# Negative controls 4-47: one mutant per further guest-ABI leaf.
for group in a b c d e f g h i j; do
    case "$group" in
        j) want="func_800AE2A4" ;;
        i) want="func_800AAD54 func_800AE2A4" ;;
        h) want="func_800A0838 func_800AFF9C func_8009EF3C func_8009F1C4" ;;
        g) want="func_800BA8F4 func_800BB13C func_800BAC50 func_80076AC8 func_80076C34 func_800AAA20 func_800A3E98" ;;
        f) want="func_8007ECDC func_8007ED14 func_8007ED58 func_8007ED98 func_8007EDE0 func_8007EE28 func_8007B98C func_8007B9C8 func_8007A828 func_8007A7BC" ;;
        e) want="func_8007EA08 func_8007EA4C func_8007EA84 func_8007EB08 func_8007EB50 func_8007EB90 func_8007EBD8 func_8007EC10 func_8007EC54 func_8007EC94" ;;
        d) want="func_8007B198 func_8007AF5C func_8007AFAC func_8007B208 func_8007B264" ;;
        c) want="func_8007AE98 func_8007AFFC func_8007B040 func_8007B084 func_8007B0C8 func_8007B134 func_8007E954 func_8007E98C func_8007E9D0 func_8007EAC8" ;;
        a) want="func_800A579C func_800A5D54 func_800A5BE8 func_8007AE38 func_8007E8E0 func_8007FEC4" ;;
        b) want="func_800A5914 func_800A5870 func_8007E154" ;;
    esac
    build_link O2 "$OUT/host-mutant-$group" src/battle/main12.c "-mutant-$group" \
        src/battle/main14.c src/battle/main69.c "$OUT/mutants-$group"
    status=0
    "$OUT/O2-mutant-$group" >"$OUT/mutant-$group.log" 2>&1 || status=$?
    for leaf in $want; do
        if [ "$status" -eq 0 ] || ! grep -q "^BATTLE OVERLAY DIFFERENTIAL FAIL $leaf " "$OUT/mutant-$group.log"; then
            echo "BATTLE OVERLAY DIFFERENTIAL CONTROL FAILURE mutated $leaf accepted" >&2
            exit 1
        fi
        grep -m1 "^BATTLE OVERLAY DIFFERENTIAL FAIL $leaf " "$OUT/mutant-$group.log" >&2
    done
done
if ! grep -q "^BATTLE OVERLAY DIFFERENTIAL FAIL func_800AE2A4 spy " "$OUT/mutant-j.log"; then
    echo "BATTLE OVERLAY DIFFERENTIAL CONTROL FAILURE spied-call argument mutant not caught by the spy comparison" >&2
    exit 1
fi
grep -m1 "^BATTLE OVERLAY DIFFERENTIAL FAIL func_800AE2A4 spy " "$OUT/mutant-j.log" >&2
# Control (r6.10 crash): the call service's old stack rule must abort the
# 800AAD54 case that runs on a 4-aligned / scratchpad stack, at its
# 0x80032498 (HeapChangeCurrentUser) jal, exactly as the game did.
build_link O2 "$OUT/host-jal-sp" src/battle/main12.c "-jal-sp" \
    src/battle/main14.c src/battle/main69.c "" "-DBATTLE_JAL_LEGACY_SP_CHECK"
status=0
"$OUT/O2-jal-sp" >"$OUT/jal-sp.log" 2>&1 || status=$?
if [ "$status" -eq 0 ] || ! grep -q 'func_800AAD54: jal failed 0x80032498' "$OUT/jal-sp.log"; then
    tail -5 "$OUT/jal-sp.log" >&2
    echo "BATTLE OVERLAY DIFFERENTIAL CONTROL FAILURE the legacy jal stack rule did not reproduce the 0x80032498 abort" >&2
    exit 1
fi
grep -m1 'jal failed 0x80032498' "$OUT/jal-sp.log" >&2
# Negative control 13 (typed bridge thunks): the pre-thunk bridge, which turns
# every address-shaped word into a host pointer, must fail the 800A5914
# tries<=0 pass-through case through the bridge.
build_link O2 "$OUT/host-legacy-bridge" src/battle/main12.c "-legacy-bridge" \
    src/battle/main14.c src/battle/main69.c "" "-DBATTLE_BRIDGE_LEGACY_ARGS"
status=0
"$OUT/O2-legacy-bridge" >"$OUT/legacy-bridge.log" 2>&1 || status=$?
if [ "$status" -eq 0 ] || ! grep -q '^BATTLE OVERLAY DIFFERENTIAL FAIL func_800A5914 bridge-domain ' "$OUT/legacy-bridge.log"; then
    cat "$OUT/legacy-bridge.log" >&2
    echo "BATTLE OVERLAY DIFFERENTIAL CONTROL FAILURE legacy bridge passed the 800A5914 pass-through case" >&2
    exit 1
fi
grep -m1 'bridge-domain func_800A5914' "$OUT/legacy-bridge.log" >&2
grep -m1 '^BATTLE OVERLAY DIFFERENTIAL FAIL func_800A5914 return ' "$OUT/legacy-bridge.log" >&2 || true
grep -m1 '^BATTLE OVERLAY DIFFERENTIAL FAIL func_800A5914 bridge-domain ' "$OUT/legacy-bridge.log" >&2
echo "BATTLE OVERLAY DIFFERENTIAL PASS (O0/O2/UBSan + wrong-result, store-path, 58 guest-ABI mutant, spy, jal-stack, indirect-native and legacy-bridge controls rejected)"
