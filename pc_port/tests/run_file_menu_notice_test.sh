#!/usr/bin/env bash
# file_menu_notice — regression certificate for the field menu's File-entry
# notice (pc_port/src/file_menu_notice.c).
#
# The notice used to be a direct SDL_ShowSimpleMessageBox call on the game
# thread.  That call is modal, so when the dialog cannot be surfaced (no window
# manager mapping zenity onto the game's display) the game thread never returns:
# the frame stops being redrawn and no input is ever read.  The Blackmoon Forest
# route hung exactly there - POSDIAG frozen at (-496,0,-1276) with owner=0x80,
# two byte-identical screenshots 25 s apart, stack in SDL_Zenity_ShowMessageBox.
#
# The policy under test (never call the modal dialog on the calling thread,
# dispatch once through the platform hook, clean up on failure) is SDL-free so it
# can be built and run here.  The runner mutates the production source and
# requires every mutant to be rejected.
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
ulimit -c 0
cd "$(dirname "$0")/../.."
SOURCE=${FILE_MENU_NOTICE_SOURCE:-"$PWD/pc_port/src/file_menu_notice.c"}
SOURCE=$(realpath "$SOURCE")
OUT=${FILE_MENU_NOTICE_OUT:-$(mktemp -d /tmp/xeno-file-menu-notice.XXXXXXXX)}
mkdir -p "$OUT"
printf 'FILE MENU NOTICE source=%s artifacts=%s\n' "$SOURCE" "$OUT"

python3 - "$OUT" "$SOURCE" <<'PY'
from pathlib import Path
from hashlib import sha256
import json, re, sys
out = Path(sys.argv[1]); source = Path(sys.argv[2])

menu = Path('src/menu/main/misc.c').read_text()
assert 'PcPort_NotifyUnsupportedFileMenu();' in menu, \
    'the field menu no longer calls PcPort_NotifyUnsupportedFileMenu'
# The call must stay on the File branch (menu1Choice == 1).
m = re.search(r'if \(g_Menu->menu1Choice == 1\) \{\s*\n\s*'
              r'PcPort_NotifyUnsupportedFileMenu\(\);', menu)
assert m, 'the notice call moved off the File (menu1Choice == 1) branch'

header = Path('pc_port/src/file_menu_notice.h').read_text()
assert 'PcPort_FileMenuNoticeStartThread' in header, \
    'the dispatch hook disappeared from the header'

source_text = source.read_text()
assert 'PcPort_FileMenuNoticeStartThread(PcPort_FileMenuNoticeThreadMain' in \
    source_text, 'the policy no longer dispatches through the thread hook'
# The dialog may only run from the thread body, never from the request entry.
entry_body = source_text.split('void PcPort_NotifyUnsupportedFileMenu(void)')[1]
assert 'PcPort_FileMenuNoticeDialog(' not in entry_body, \
    'the request entry calls the modal dialog directly again'

paths = [source, Path('pc_port/src/file_menu_notice.h'),
         Path('pc_port/tests/file_menu_notice_test.c'),
         Path('pc_port/tests/run_file_menu_notice_test.sh'),
         Path('src/menu/main/misc.c')]
(out / 'provenance.json').write_text(json.dumps({
    'scope': 'field menu File-entry notice dispatch (never modal on the game thread)',
    'retail_call_site': 'src/menu/main/misc.c menu1Choice == 1',
    'source_pins': {str(p.resolve()): sha256(p.read_bytes()).hexdigest() for p in paths},
}, indent=2) + '\n')
print('FILE MENU NOTICE provenance: menu1Choice==1 -> policy -> thread hook PASS')
PY

TEST_SRC=pc_port/tests/file_menu_notice_test.c
COMMON=(-std=gnu17 -Wall -Wextra -Werror -Ipc_port/src)

build_and_run() {
    local mode="$1" cc="$2" src="$3"; shift 3
    "$cc" "${COMMON[@]}" "$@" "$TEST_SRC" "$src" -o "$OUT/$mode"
    local status=0
    "$OUT/$mode" > "$OUT/$mode.log" 2>&1 || status=$?
    cat "$OUT/$mode.log"
    return "$status"
}

echo "== production regimes =="
for mode in O0 O2; do
    if ! build_and_run "$mode" gcc "$SOURCE" "-$mode"; then
        printf 'FILE MENU NOTICE REGIME FAILURE %s\n' "$mode" >&2; exit 1
    fi
    grep -q '^FILE MENU NOTICE PASS ' "$OUT/$mode.log" || {
        echo "FILE MENU NOTICE REGIME FAILURE $mode PASS line missing" >&2; exit 1; }
    printf '  %s (gcc): PASS\n' "$mode"
done

cat > "$OUT/ubsan_probe.c" <<'EOF'
int main(void) { return 0; }
EOF
ubsan_cc=""
if gcc -std=gnu17 -O1 -fsanitize=undefined -fno-sanitize-recover=all \
        "$OUT/ubsan_probe.c" -o "$OUT/ubsan_probe.gcc" >/dev/null 2>&1; then
    ubsan_cc=gcc
    printf 'UBSan regime: gcc (probe linked)\n'
elif clang -std=gnu17 -O1 -fsanitize=undefined -fno-sanitize-recover=all \
        "$OUT/ubsan_probe.c" -o "$OUT/ubsan_probe.clang" >/dev/null 2>&1; then
    ubsan_cc=clang
    printf 'UBSan regime: clang (gcc runtime probe failed)\n'
else
    echo 'FILE MENU NOTICE UBSAN UNAVAILABLE: neither gcc nor clang can build -fsanitize=undefined' >&2
    exit 1
fi
if ! build_and_run UBSan "$ubsan_cc" "$SOURCE" -O1 \
        -fsanitize=undefined -fno-sanitize-recover=all; then
    echo 'FILE MENU NOTICE REGIME FAILURE UBSan' >&2; exit 1
fi
grep -q '^FILE MENU NOTICE PASS ' "$OUT/UBSan.log" || {
    echo 'FILE MENU NOTICE REGIME FAILURE UBSan PASS line missing' >&2; exit 1; }
printf '  UBSan (%s): PASS\n' "$ubsan_cc"

python3 - "$OUT" "$SOURCE" <<'PY'
from pathlib import Path
import json, sys
out = Path(sys.argv[1]); source = Path(sys.argv[2]).read_text()
dispatch = ('    if (!PcPort_FileMenuNoticeStartThread('
            'PcPort_FileMenuNoticeThreadMain,\n'
            '                                          notice)) {')
changes = {
    # The original defect: show the modal dialog on the calling thread.
    'dialog-inline': (
        dispatch,
        '    PcPort_FileMenuNoticeDialog(notice->title, notice->text);\n'
        '    if (0) {'),
    # Never dispatch at all.
    'dispatch-skipped': (dispatch, '    if (1) {'),
    # Free a context that was never allocated.
    'oom-frees': (
        '    if (notice == 0) {\n',
        '    if (notice == 0) {\n'
        '        PcPort_FileMenuNoticeFree(notice);\n'),
    # Leak the context when the dispatch is refused.
    'failure-leaks': (
        '            PcPort_FileMenuNoticeLog("File menu notice skipped: no thread\\n");\n'
        '        if (PcPort_FileMenuNoticeFree != 0)\n'
        '            PcPort_FileMenuNoticeFree(notice);\n',
        '            PcPort_FileMenuNoticeLog("File menu notice skipped: no thread\\n");\n'),
    # Drift the text the player is told to act on.
    'title-drift': (
        'const char *const PcPort_FileMenuNoticeTitle = "File menu not implemented";',
        'const char *const PcPort_FileMenuNoticeTitle = "File menu unavailable";'),
}
manifest = {}
for name, (old, new) in changes.items():
    assert source.count(old) == 1, (name, source.count(old))
    code = source.replace(old, new, 1)
    assert code != source, name
    (out / (name + '.c')).write_text(code)
    manifest[name] = name
(out / 'negative-controls.json').write_text(json.dumps(manifest, indent=2) + '\n')
print('FILE MENU NOTICE mutants written:', ' '.join(manifest))
PY

echo "== mutants (each must be rejected) =="
MUTANTS=(dialog-inline dispatch-skipped oom-frees failure-leaks title-drift)
for mutant in "${MUTANTS[@]}"; do
    set +e
    build_and_run "$mutant" gcc "$OUT/$mutant.c" -O0 -w > "$OUT/$mutant.log" 2>&1
    rc=$?
    set -e
    if [ "$rc" -eq 0 ] || ! grep -q '^FILE MENU NOTICE FAIL ' "$OUT/$mutant.log"; then
        echo "FILE MENU NOTICE MUTANT NOT DETECTED: $mutant rc=$rc" >&2
        cat "$OUT/$mutant.log" >&2
        exit 1
    fi
    printf 'FILE MENU NOTICE mutant rejected: %s\n' "$mutant"
done

python3 - "$SOURCE" <<'PY'
from pathlib import Path
from hashlib import sha256
import sys
print('FILE MENU NOTICE source pin unchanged:',
      sha256(Path(sys.argv[1]).read_bytes()).hexdigest()[:16])
PY
echo 'FILE MENU NOTICE PASS (3 regimes, 5/5 mutants rejected)'
