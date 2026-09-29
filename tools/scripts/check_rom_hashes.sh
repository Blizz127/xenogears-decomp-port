#!/usr/bin/env bash
# check_rom_hashes.sh — the full-ROM checksum gate.
#
# Builds every pinned matching artifact FROM CLEAN and verifies them
# against config/checksum.sha (the RETAIL overlay hashes — ground truth,
# never re-pinned by this script or anything else).
#
# Why from-clean, always: ninja incrementality across checkout hops produced
# convincing false greens twice during the 2026-07-24 matching-side
# diagnostic. A gate that can pass on stale artifacts is worse than no gate,
# so this one deletes build/ and linker/ and rebuilds via the documented
# `make build` flow (which includes the gears regeneration and the
# ApplyMatrixSV sed that raw ninja lacks). Cost is a few minutes; that is
# the price of an answer you can trust.
#
# This gate is matching-side only. It does not touch, and must never block,
# the port workflow (pc_port/build_port.sh, the menu suite, the watchdogs).
#
# Exit: 0 iff every pinned artifact matches. Nonzero on any mismatch or
# build failure.
set -u -o pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"

if [ ! -f gears.toml ] || [ ! -f config/checksum.sha ]; then
    echo "rom-check: run from the xenogears-decomp repo (gears.toml + config/checksum.sha not found)" >&2
    exit 2
fi
case "$ROOT" in
    *xenogears-decomp*) ;;
    *) echo "rom-check: gears locates the project by a path component named" >&2
       echo "           'xenogears-decomp'; this tree is at: $ROOT" >&2
       exit 2 ;;
esac
if ! command -v mips-linux-gnu-ld >/dev/null 2>&1; then
    echo "rom-check: MIPS toolchain not on PATH — run inside the build container" >&2
    echo "           (e.g. distrobox enter xenogears-dev -- make rom-check)" >&2
    exit 2
fi

# Known-red ledger. Purely informational: it makes an expected FAIL legible.
# It does not and must not gate anything. Keep it current: an artifact that
# passes has no business here (slus_006.64, member_change_menu and shop_menu
# were removed on 2026-09-28 once they matched retail again).
known_red() {
    case "$1" in
        *field*)               echo "known red at HEAD (first-red unscanned; includes the jtbl_8006FC88 literal-u32 era)" ;;
        */menu.bin)            echo "known red: WIP overlay bring-up since cedab7d (first-red unscanned); the pin is the retail menu.bin" ;;
        *)                     echo "not in the known-red ledger — NEW drift, investigate before anything else" ;;
    esac
}

# Preserve the objdiff baseline: the documented `make build` begins with
# `make clean`, which deletes expected/. That baseline belongs to the
# per-object workflow and is none of this gate's business.
EXPECTED_SAVED=""
if [ -d expected ]; then
    EXPECTED_SAVED="$(mktemp -d "${TMPDIR:-/tmp}/romcheck-expected.XXXXXX")"
    mv expected "$EXPECTED_SAVED/expected"
fi
restore_expected() {
    if [ -n "$EXPECTED_SAVED" ] && [ -d "$EXPECTED_SAVED/expected" ] && [ ! -d expected ]; then
        mv "$EXPECTED_SAVED/expected" expected
    fi
    [ -n "$EXPECTED_SAVED" ] && rmdir "$EXPECTED_SAVED" 2>/dev/null
}
trap restore_expected EXIT

echo "rom-check: clean rebuild (rm -rf build linker; make build) ..."
rm -rf build linker
if ! make build > /tmp/romcheck-build.log 2>&1; then
    echo "rom-check: BUILD FAILED — tail of /tmp/romcheck-build.log:" >&2
    tail -15 /tmp/romcheck-build.log >&2
    exit 3
fi

echo
echo "rom-check: verifying pinned artifacts against config/checksum.sha"
echo "           (pins are the RETAIL overlay hashes; a FAIL means the build"
echo "            drifted from retail, never that the pin is wrong)"
echo
fails=0
while read -r pin path; do
    [ -z "${path:-}" ] && continue
    if [ ! -f "$path" ]; then
        printf 'FAIL  %-36s missing (build did not produce it)\n' "$path"
        fails=$((fails + 1))
        continue
    fi
    got="$(sha256sum "$path" | cut -d' ' -f1)"
    size="$(stat -c%s "$path")"
    if [ "$got" = "$pin" ]; then
        printf 'PASS  %-36s %s (%s bytes)\n' "$path" "${got:0:16}" "$size"
    else
        printf 'FAIL  %-36s built %s… vs pin %s… (%s bytes)\n' "$path" "${got:0:8}" "${pin:0:8}" "$size"
        printf '      -> %s\n' "$(known_red "$path")"
        fails=$((fails + 1))
    fi
done < config/checksum.sha

echo
if [ "$fails" -eq 0 ]; then
    echo "rom-check: ALL PINNED ARTIFACTS MATCH RETAIL."
    exit 0
else
    echo "rom-check: $fails pinned artifact(s) DRIFTED from retail."
    echo "           Repairing means re-matching the drifted code, never re-pinning."
    exit 1
fi
