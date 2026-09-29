#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="${NPC_EVENT_BUILD_DIR:-$ROOT/pc_port/build_native/npc_event}"
CC="${CC:-gcc}"
mkdir -p "$BUILD_DIR"
cd "$ROOT"

BASE=(-std=gnu17 -fno-pie -no-pie -m64 -fno-builtin
      -DXENO_PC_PORT -DXENO_FIELD_OBJECT_OVERLAY -DSKIP_ASM -D_LANGUAGE_C
      -DUSE_EXTENDED_PRIM_POINTERS=0
      -include assert.h -fpermissive -w -ffunction-sections -fdata-sections)
INC=(-Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include
     -Ipc_port/extern/PsyCross/include/psx -Ipc_port/src)
SRC=(pc_port/tests/npc_event_prod_test.c
     src/field/main/misc6.c
     src/field/main/misc7.c
     src/field/dialogue/text_box.c
     src/field/scripts/variable_handlers.c
     pc_port/tests/vector_stubs.c)

compile_and_run() {
    local name="$1"
    local run_rc=0
    shift
    "$CC" "${BASE[@]}" "${INC[@]}" "$@" \
        -c src/field/dialogue/text_box_render.c \
        -o "$BUILD_DIR/${name}_render.o"
    objcopy --weaken-symbol=func_8007F8DC \
            --weaken-symbol=func_8007F814 \
            "$BUILD_DIR/${name}_render.o"
    "$CC" "${BASE[@]}" "${INC[@]}" "$@" "${SRC[@]}" \
        "$BUILD_DIR/${name}_render.o" \
        -Wl,--gc-sections \
        -o "$BUILD_DIR/$name"
    "$BUILD_DIR/$name" >"$BUILD_DIR/$name.stdout" \
        2>"$BUILD_DIR/$name.stderr" || run_rc=$?
    return "$run_rc"
}

for regime in O0 O2 UBSan; do
    flags=(-O0 -g)
    if [[ "$regime" == O2 ]]; then flags=(-O2); fi
    if [[ "$regime" == UBSan ]]; then
        flags=(-O2 -g -fsanitize=undefined -fno-sanitize-recover=all)
    fi
    compile_and_run "$regime" "${flags[@]}"
    rg -q '^NPC EVENT certificate PASS$' "$BUILD_DIR/$regime.stdout"
    test ! -s "$BUILD_DIR/$regime.stderr"
    rg -v '^\[xeno-port\] PSX RAM emulation:' "$BUILD_DIR/$regime.stdout" \
        >"$BUILD_DIR/$regime.normalized" || true
done
cmp "$BUILD_DIR/O0.normalized" "$BUILD_DIR/O2.normalized"
cmp "$BUILD_DIR/O0.normalized" "$BUILD_DIR/UBSan.normalized"
echo "NPC EVENT O0/O2/UBSAN PASS"

mutants=(
    'M1:NPC_EVENT_MUTANT_ALWAYS_DONE:walkwait.not.instant'
    'M2:NPC_EVENT_MUTANT_NO_IP:sleep.holds.then.advances'
    'M3:NPC_EVENT_MUTANT_MODE2_CIRCLE:walkwait.mode2.targets.actor'
)
for entry in "${mutants[@]}"; do
    label="${entry%%:*}"
    rest="${entry#*:}"
    define="${rest%%:*}"
    assertion="${rest#*:}"
    set +e
    compile_and_run "$label" -O0 -g -D"$define"
    rc=$?
    set -e
    if [[ "$rc" -eq 0 ]] || \
       ! rg -q "^ASSERTION ${assertion}$" "$BUILD_DIR/$label.stderr"; then
        echo "$label FAILED named mutant gate rc=$rc" >&2
        sed -n '1,80p' "$BUILD_DIR/$label.stderr" >&2 || true
        exit 1
    fi
    echo "$label DETECTED"
done
echo "NPC EVENT CERTIFICATE PASS; M1-M3 DETECTED"

SCRATCH="${NPC_EVENT_SCRATCH:-/tmp/grok-goal-92283f6cf396/implementer}"
mkdir -p "$SCRATCH/npc_event_cert"
cp -a "$BUILD_DIR"/. "$SCRATCH/npc_event_cert/"

{
  echo '=== CERT ==='
  rg '^NPC EVENT certificate PASS$' "$BUILD_DIR/O0.stdout" || true
  rg '^ASSERTION walkwait.not.instant$' "$BUILD_DIR/M1.stderr" || true
  rg '^ASSERTION sleep.holds.then.advances$' "$BUILD_DIR/M2.stderr" || true
  echo '=== LIVE1 ==='
  rg 'actor=16 ip=1081 op=0x26 wait=3' "$SCRATCH/npc_event_live_1.log" | head -1 || true
  rg 'actor=16 ip=1084' "$SCRATCH/npc_event_live_1.log" | head -1 || true
  rg 'actor=40 ip=5305 op=0x53.*pos=-509' "$SCRATCH/npc_event_live_1.log" | head -1 || true
  rg 'actor=40 ip=5309' "$SCRATCH/npc_event_live_1.log" | head -1 || true
  rg 'dialog-open box=0 str=20' "$SCRATCH/npc_event_live_1.log" | head -1 || true
  rg 'waitf=0xe' "$SCRATCH/npc_event_live_1.log" | head -1 || true
  rg 'f=131 box=0 vis=-1' "$SCRATCH/npc_event_live_1.log" | head -1 || true
  rg 'frame=131 actors=64 lock=0x0' "$SCRATCH/npc_event_live_1.log" | head -1 || true
  rg 'dialog-open box=0 str=21' "$SCRATCH/npc_event_live_1.log" | head -1 || true
  echo '=== LIVE2 ==='
  rg 'actor=16 ip=1081 op=0x26 wait=3' "$SCRATCH/npc_event_live_2.log" | head -1 || true
  rg 'actor=16 ip=1084' "$SCRATCH/npc_event_live_2.log" | head -1 || true
  rg 'actor=40 ip=5305 op=0x53.*pos=-509' "$SCRATCH/npc_event_live_2.log" | head -1 || true
  rg 'actor=40 ip=5309' "$SCRATCH/npc_event_live_2.log" | head -1 || true
  rg 'dialog-open box=0 str=20' "$SCRATCH/npc_event_live_2.log" | head -1 || true
  rg 'waitf=0xe' "$SCRATCH/npc_event_live_2.log" | head -1 || true
  rg 'f=131 box=0 vis=-1' "$SCRATCH/npc_event_live_2.log" | head -1 || true
  rg 'frame=131 actors=64 lock=0x0' "$SCRATCH/npc_event_live_2.log" | head -1 || true
  rg 'dialog-open box=0 str=21' "$SCRATCH/npc_event_live_2.log" | head -1 || true
} >"$SCRATCH/npc_event_evidence.txt"

cat >"$SCRATCH/NPC_EVENT_REVIEW.txt" <<'EOF'
src/field/main/misc6.c
src/field/main/misc7.c
src/field/main/misc8.c
src/field/dialogue/text_box.c
pc_port/tests/npc_event_prod_test.c
pc_port/tests/run_npc_event.sh
pc_port/tests/vector_stubs.c
EOF

echo "WROTE $SCRATCH/npc_event_evidence.txt"
echo "WROTE $SCRATCH/NPC_EVENT_REVIEW.txt"
