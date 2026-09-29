#!/usr/bin/env bash
# Field menu party-sprite VRAM save/restore tables (D_800ADCB0 / D_800ADCC8).
#
# The tables live in retail field .data and were missing from the port's
# migrated blob, so the stub generator zero-filled them: func_800799D4's six
# save blits and six restore blits all degenerated to
# MoveImage({0,0,64,32}, 0, 0) and the party sprite texture pages were neither
# saved before MenuMain nor restored after it -- a party member came back from
# the menu with transparent texels and stopped being visible.
#
# The oracle is disc/field.bin.  The shipping tables are zero storage that
# pc_port/src/data_field.c's load table fills from disc/field.bin at startup;
# the test checks those load rows, then renders the same bytes from the disc
# into C in the (git-ignored) build directory so the mutants below can rewrite
# exactly those values.  No retail values are committed.
#
# Regimes: -O0, -O2 and UBSan (differential), then four mutants that must each
# BUILD and then FAIL AT RUNTIME.  A mutant that fails to compile is treated as
# a broken control and aborts the run -- a control that never links proves
# nothing.
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

cd "$(dirname "$0")/../.."
OUT=pc_port/build_native/field_menu_party_vram_retail_test
rm -rf "$OUT"
mkdir -p "$OUT"

test -f disc/field.bin

# Pin the retail oracle: the 0x30 bytes at field.bin 0x3E1C0 (RAM 0x800ADCB0,
# field overlay base 0x8006FAF0) covering both tables back to back.
read -r hash _ < <(dd if=disc/field.bin bs=1 skip=$((0x3E1C0)) count=$((0x30)) status=none | sha256sum)
test "$hash" = 8bd7b18aa22af76fe507ac3d499b40b21ff2609d854cd09a9befa28e325c9eac

# The rect geometry the test replays comes from func_800799D4.  Fail loudly if
# misc4.c stops using 0x40 x 0x20 for the party blits.
grep -q 'rect\.w = 0x40;' src/field/main/misc4.c
grep -q 'rect\.h = 0x20;' src/field/main/misc4.c
grep -q 'pFieldRects = D_800ADCB0;' src/field/main/misc4.c
grep -q 'pFieldRects = D_800ADCC8;' src/field/main/misc4.c

# The shipping definitions: zero storage plus load rows at the retail offsets.
grep -q '^unsigned short D_800ADCB0\[12\];' pc_port/src/data_field.c
grep -q '^unsigned short D_800ADCC8\[12\];' pc_port/src/data_field.c
grep -q 'XENO_RD(D_800ADCB0, XENO_RD_FIELD, XENO_RD_FIELD_OFF(0x800ADCB0u), 0x18),' pc_port/src/data_field.c
grep -q 'XENO_RD(D_800ADCC8, XENO_RD_FIELD, XENO_RD_FIELD_OFF(0x800ADCC8u), 0x18),' pc_port/src/data_field.c
# Render the loaded values (from the disc) as the definitions the replay links.
python3 - "$OUT/tables.c" <<'PY'
import struct, sys
raw = open('disc/field.bin', 'rb').read()[0x3E1C0:0x3E1F0]
vals = struct.unpack('<24H', raw)
with open(sys.argv[1], 'w') as out:
    for name, part in (('D_800ADCB0', vals[:12]), ('D_800ADCC8', vals[12:])):
        out.write('unsigned short %s[12] = {\n' % name)
        for i in range(0, 12, 2):
            out.write('    0x%04X, 0x%04X,\n' % (part[i], part[i + 1]))
        out.write('};\n')
PY
# Both tables must actually have been rendered (12 values each + 2 braces lines).
test "$(grep -c '0x' "$OUT/tables.c")" -ge 12
grep -q 'D_800ADCB0' "$OUT/tables.c"
grep -q 'D_800ADCC8' "$OUT/tables.c"

cp pc_port/tests/field_menu_party_vram_retail_test.c "$OUT/body.c"

build_and_run() {
    local tag="$1"; shift
    local body="$1"; shift
    local tables="$1"; shift
    local flags=("$@")

    : "${CC:=gcc}"
    "$CC" "${flags[@]}" -Wall -Wextra -Werror -c "$body" -o "$OUT/$tag.test.o"
    "$CC" "${flags[@]}" -Wall -Wextra -Werror -c "$tables" -o "$OUT/$tag.tables.o"
    "$CC" "${flags[@]}" "$OUT/$tag.test.o" "$OUT/$tag.tables.o" -o "$OUT/$tag.bin"
}

for regime in O0 O2 UBSan; do
    case "$regime" in
        O0) flags=(-O0 -g) ;;
        O2) flags=(-O2) ;;
        UBSan) flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all) ;;
    esac
    # The host GCC installation has no libasan/libubsan shared objects.  Use
    # the available Clang runtime for this regime only; the tested sources and
    # assertions are otherwise identical to O0/O2.
    if [[ "$regime" == UBSan ]]; then
        CC="${UBSAN_CC:-clang}" build_and_run "$regime" "$OUT/body.c" \
            "$OUT/tables.c" "${flags[@]}"
    else
        build_and_run "$regime" "$OUT/body.c" "$OUT/tables.c" "${flags[@]}"
    fi
    "$OUT/$regime.bin"
done

# ---- mutants: each MUST build, then MUST fail at runtime ----------------------
# Columns: which file the mutation lands in, so a control that breaks the disc
# oracle and one that breaks only the blit replay are both represented.
for mutant in zeroed swapped selfbackup stride rect_h; do
    body="$OUT/body.c"
    tables="$OUT/tables.c"
    case "$mutant" in
        # The exact pre-fix state: the stub generator's zero-filled placeholders.
        # Fails the disc oracle, the geometry checks AND the round trip.
        zeroed)
            sed 's/0x0[0-9A-Fa-f]*,/0x0000,/g' "$OUT/tables.c" > "$OUT/$mutant.tables.c"
            tables="$OUT/$mutant.tables.c" ;;
        # Byte-order transcription error: x and y swapped in the field table.
        swapped)
            sed -e 's/^    0x0000, 0x00E0,$/    0x00E0, 0x0000,/' \
                -e 's/^    0x0040, 0x00E0,$/    0x00E0, 0x0040,/' \
                -e 's/^    0x0080, 0x00E0,$/    0x00E0, 0x0080,/' \
                -e 's/^    0x00C0, 0x00E0,$/    0x00E0, 0x00C0,/' \
                -e 's/^    0x0100, 0x00E0,$/    0x00E0, 0x0100,/' \
                -e 's/^    0x0100, 0x01E0,$/    0x01E0, 0x0100,/' \
                "$OUT/tables.c" > "$OUT/$mutant.tables.c"
            tables="$OUT/$mutant.tables.c" ;;
        # Backup rects placed on top of the field pages, so the menu's wipe
        # destroys the saved copy too: geometry AND round trip must reject.
        selfbackup)
            sed -e 's/^    0x02C0, 0x0000,$/    0x0000, 0x00E0,/' \
                -e 's/^    0x02C0, 0x0020,$/    0x0040, 0x00E0,/' \
                -e 's/^    0x02C0, 0x0040,$/    0x0080, 0x00E0,/' \
                -e 's/^    0x02C0, 0x0060,$/    0x00C0, 0x00E0,/' \
                -e 's/^    0x02C0, 0x0080,$/    0x0100, 0x00E0,/' \
                -e 's/^    0x02C0, 0x00A0,$/    0x0100, 0x01E0,/' \
                "$OUT/tables.c" > "$OUT/$mutant.tables.c"
            tables="$OUT/$mutant.tables.c" ;;
        # Backup rows spaced 0x10 apart instead of 0x20: adjacent backups
        # overlap and half of each saved page is lost.
        stride)
            sed -e 's/^    0x02C0, 0x0020,$/    0x02C0, 0x0010,/' \
                -e 's/^    0x02C0, 0x0040,$/    0x02C0, 0x0020,/' \
                -e 's/^    0x02C0, 0x0060,$/    0x02C0, 0x0030,/' \
                -e 's/^    0x02C0, 0x0080,$/    0x02C0, 0x0040,/' \
                -e 's/^    0x02C0, 0x00A0,$/    0x02C0, 0x0050,/' \
                "$OUT/tables.c" > "$OUT/$mutant.tables.c"
            tables="$OUT/$mutant.tables.c" ;;
        # Retail-correct tables, wrong blit height: only half of each party page
        # is saved and restored.  The disc oracle passes; the round trip must
        # still reject, proving the replay is doing real work.
        rect_h)
            sed 's/^#define PARTY_RECT_H 0x20$/#define PARTY_RECT_H 0x10/' \
                "$OUT/body.c" > "$OUT/$mutant.body.c"
            body="$OUT/$mutant.body.c" ;;
    esac

    if cmp -s "$OUT/body.c" "$body" && cmp -s "$OUT/tables.c" "$tables"; then
        echo "MENU PARTY VRAM CONTROL BROKEN: mutant '$mutant' changed nothing" >&2
        exit 1
    fi

    # A control that does not compile is not a rejection.  Build it separately
    # and fail the run if the build itself breaks.
    if ! build_and_run "mut_$mutant" "$body" "$tables" -O2 2> "$OUT/$mutant.build.log"; then
        echo "MENU PARTY VRAM CONTROL BROKEN: mutant '$mutant' failed to BUILD" >&2
        cat "$OUT/$mutant.build.log" >&2
        exit 1
    fi

    if "$OUT/mut_$mutant.bin" > "$OUT/$mutant.log" 2>&1; then
        echo "MENU PARTY VRAM MUTANT SURVIVED: $mutant" >&2
        cat "$OUT/$mutant.log" >&2
        exit 1
    fi
    grep -q 'MENU PARTY VRAM FAIL' "$OUT/$mutant.log"
    echo "  mutant $mutant: built, then rejected at runtime"
done

# The pre-fix bug and the wrong-height control must each be caught by the blit
# replay itself, not only by the disc byte comparison.
grep -q 'did not survive the menu round trip' "$OUT/zeroed.log"
grep -q 'did not survive the menu round trip' "$OUT/selfbackup.log"
grep -q 'did not survive the menu round trip' "$OUT/rect_h.log"

echo 'MENU PARTY VRAM: PASS (O0/O2/UBSan) + 5 mutants built-and-rejected'
