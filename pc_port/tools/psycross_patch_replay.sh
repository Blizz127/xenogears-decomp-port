#!/usr/bin/env bash
#
# Strict PsyCross patch-replay checker.
#
# Intentional changes to the vendored (gitignored) PsyCross tree are only
# durable once they are represented in pc_port/patches/*.patch. This script
# proves that: it replays the committed patch series, in build_port.sh order,
# onto a pristine clone of the recorded vendor baseline and requires the result
# to match the live vendor tree byte for byte.
#
#   pc_port/tools/psycross_patch_replay.sh            # replay + compare
#   pc_port/tools/psycross_patch_replay.sh --keep DIR # also leave the replay tree
#
# Exit status is non-zero on a reject, on a patch that fails to apply, or on any
# file that the replay does not reproduce.
set -uo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
PSX="$ROOT/pc_port/extern/PsyCross"
PATCHES="$ROOT/pc_port/patches"
BUILD_SCRIPT="$ROOT/pc_port/build_port.sh"

KEEP=""
if [ "${1:-}" = "--keep" ]; then
    KEEP="${2:?--keep needs a directory}"
fi

command -v git >/dev/null 2>&1 || { echo "ERROR: git is required" >&2; exit 1; }
[ -d "$PSX/.git" ] || {
    echo "ERROR: $PSX is not a git worktree; run pc_port/build_port.sh once to" >&2
    echo "       create the vendor baseline commit." >&2
    exit 1
}

BASELINE="$(git -C "$PSX" rev-list --max-parents=0 HEAD | tail -1)"
echo "==> vendor baseline: $BASELINE ($(git -C "$PSX" log -1 --format=%s "$BASELINE"))"

WORK="${KEEP:-$(mktemp -d)}"
if [ -z "$KEEP" ]; then
    trap 'rm -rf "$WORK"' EXIT
fi
REPLAY="$WORK/psycross-replay"
rm -rf "$REPLAY"
git clone -q "$PSX" "$REPLAY"
git -C "$REPLAY" checkout -q "$BASELINE"

# The patch series and its flags live in build_port.sh; parse them so this
# checker can never drift out of order with the build.
mapfile -t SERIES < <(grep -E '^apply_psycross_patch "' "$BUILD_SCRIPT" \
    | sed -E 's|^apply_psycross_patch "\$ROOT/pc_port/patches/([^"]+)" "([^"]+)"( "([^"]+)")?.*|\1 \2 \4|')

[ "${#SERIES[@]}" -gt 0 ] || { echo "ERROR: no patch series found in build_port.sh" >&2; exit 1; }

fail=0
applied=0
skipped=0
echo "==> replaying ${#SERIES[@]} patches"
for entry in "${SERIES[@]}"; do
    read -r patch marker mode <<<"$entry"
    args=()
    [ "${mode:-}" = "unidiff-zero" ] && args+=(--unidiff-zero)
    if grep -Rqs "$marker" "$REPLAY" --exclude-dir=.git; then
        echo "    skip (in baseline)  $patch"
        skipped=$((skipped + 1))
        continue
    fi
    if git -C "$REPLAY" apply "${args[@]}" --reverse --check "$PATCHES/$patch" >/dev/null 2>&1; then
        echo "    skip (already)      $patch"
        skipped=$((skipped + 1))
        continue
    fi
    out="$(git -C "$REPLAY" apply "${args[@]}" --verbose "$PATCHES/$patch" 2>&1)"
    rc=$?
    if [ $rc -ne 0 ]; then
        echo "    FAILED rc=$rc        $patch"
        echo "$out" | sed 's/^/        /'
        fail=1
        continue
    fi
    echo "    applied             $patch"
    # Offsets are reported for information; rejects and fuzz are failures.
    offsets="$(echo "$out" | grep -iE "offset" || true)"
    [ -n "$offsets" ] && echo "$offsets" | sed 's/^/        note: /'
    bad="$(echo "$out" | grep -iE "reject|fuzz" || true)"
    if [ -n "$bad" ]; then
        echo "$bad" | sed 's/^/        WARNING: /'
        fail=1
    fi
    applied=$((applied + 1))
done

echo "==> comparing replay against the live vendor tree"
diffout="$(diff -rq --exclude=.git "$REPLAY" "$PSX" 2>&1)"
if [ -n "$diffout" ]; then
    echo "$diffout" | sed 's/^/    /'
    echo
    echo "REPLAY MISMATCH: the vendor tree carries changes the committed patch"
    echo "series does not reproduce. Fold them into pc_port/patches/ (see"
    echo "docs/ai_context/ACTIVE_HANDOFF.md, 'PsyCross patch durability')."
    fail=1
else
    echo "    identical"
fi

echo "==> applied=$applied skipped=$skipped"
[ -n "$KEEP" ] && echo "==> replay tree kept at $REPLAY"
if [ $fail -ne 0 ]; then
    echo "PSYCROSS PATCH REPLAY: FAIL"
    exit 1
fi
echo "PSYCROSS PATCH REPLAY: OK"
