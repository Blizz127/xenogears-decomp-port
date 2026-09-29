#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
ulimit -c 0
cd "$(dirname "$0")/../.."

out=$(mktemp -d pc_port/build_native/host_video_mode_boot_test.XXXXXXXX)
echo "HOST VIDEO MODE BOOT artifacts: $out"
root=$(pwd)
container_image="${XENO_DEV_TOOLCHAIN_IMAGE:-localhost/xenogears-dev-toolchain:current}"

python3 - "$out" <<'PY'
from pathlib import Path
import hashlib
import json
import sys

out = Path(sys.argv[1])
main_source = Path("pc_port/src/port_main.c").read_text()
psyx_source = Path("pc_port/extern/PsyCross/src/PsyX_main.cpp").read_text()
etc_source = Path("pc_port/extern/PsyCross/src/psx/LIBETC.C").read_text()


def function(text, signature):
    start = text.index(signature)
    brace = text.index("{", start)
    depth = 0
    for index in range(brace, len(text)):
        depth += (text[index] == "{") - (text[index] == "}")
        if depth == 0:
            return text[start:index + 1] + "\n"
    raise SystemExit(f"HOST VIDEO MODE BOOT RED: unterminated {signature}")


def main_prefix(text):
    signature = "int main(int argc, char** argv)"
    start = text.index(signature)
    call = text.index("    PsyX_Initialise(", start)
    semicolon = text.index(";", call)
    return text[start:semicolon + 1] + "\n    return 0;\n}\n"


prefix = main_prefix(main_source)
production = "\n".join([
    function(psyx_source, "int PsyX_Sys_SetVMode(int mode)"),
    function(etc_source, "int SetVideoMode(int mode)"),
])
(out / "production.inc").write_text(production)
(out / "main_prefix.inc").write_text(prefix)

if "PsyX_Initialise(" not in prefix:
    raise SystemExit("HOST VIDEO MODE BOOT RED: main prefix lost PsyX_Initialise")
if "int PsyX_Sys_SetVMode(int mode)" not in production:
    raise SystemExit("HOST VIDEO MODE BOOT RED: missing actual PsyX_Sys_SetVMode")
if "int SetVideoMode(int mode)" not in production:
    raise SystemExit("HOST VIDEO MODE BOOT RED: missing actual SetVideoMode")

files = [
    "pc_port/tests/host_video_mode_boot_test.cpp",
    "pc_port/tests/run_host_video_mode_boot_test.sh",
    "pc_port/src/port_main.c",
    "pc_port/extern/PsyCross/src/PsyX_main.cpp",
    "pc_port/extern/PsyCross/src/psx/LIBETC.C",
]
(out / "provenance.json").write_text(json.dumps({
    "source_sha256": {
        path: hashlib.sha256(Path(path).read_bytes()).hexdigest() for path in files
    },
    "retail_pins": {
        "slus_exe_sha256": "dc0b2dd786203d4cce5927c5a3fc85a18f39a3f7406078860076ebb0bbae7119",
        "slus_exe_load_address": "0x80010000",
        "slus_exe_file_offset": "0x800",
        "video_mode_word_address": "0x80058990",
        "video_mode_word_hex": "00000000",
        "set_get_video_mode_bytes_sha256": "621cca57e89f7c1de5dd9d4eef36d11db599fc4300bc8c0e2aee0265ccae7a85",
    },
}, indent=2) + "\n")

# Keep the mutation transformations source-exact.  If the production call is
# absent the normal test is the intended RED; mutants become available once the
# parent installs the fix.  Never report an unchanged mutant as detected.
needle = "    SetVideoMode(0);\n    PsyX_Initialise("
if needle not in main_source:
    (out / "mutation_status").write_text(
        "RED_EXPECTED: missing SetVideoMode(0) immediately before PsyX_Initialise\n"
    )
else:
    if main_source.count(needle) != 1:
        raise SystemExit("HOST VIDEO MODE BOOT RED: expected exactly one startup call")
    mutants = {
        "missing-call": main_source.replace("    SetVideoMode(0);\n", "", 1),
        "pal-call": main_source.replace("    SetVideoMode(0);", "    SetVideoMode(1);", 1),
        "moved-after-core": main_source.replace(
            needle,
            "    PsyX_Initialise("
        ).replace(
            "    PsyX_Initialise(WINDOW_TITLE, SCREEN_WIDTH, SCREEN_HEIGHT, 0);\n",
            "    PsyX_Initialise(WINDOW_TITLE, SCREEN_WIDTH, SCREEN_HEIGHT, 0);\n"
            "    SetVideoMode(0);\n",
            1,
        ),
    }
    for name, mutated in mutants.items():
        if mutated == main_source:
            raise SystemExit(f"HOST VIDEO MODE BOOT RED: unchanged mutant {name}")
        if name == "missing-call" and "SetVideoMode(0);\n    PsyX_Initialise(" in mutated:
            raise SystemExit("HOST VIDEO MODE BOOT RED: missing-call replacement failed")
        if name == "pal-call" and "SetVideoMode(1);\n    PsyX_Initialise(" not in mutated:
            raise SystemExit("HOST VIDEO MODE BOOT RED: pal-call replacement failed")
        if name == "moved-after-core":
            if "PsyX_Initialise(WINDOW_TITLE, SCREEN_WIDTH, SCREEN_HEIGHT, 0);\n    SetVideoMode(0);" not in mutated:
                raise SystemExit("HOST VIDEO MODE BOOT RED: moved-after-core replacement failed")
        target = out / name
        target.mkdir()
        (target / "main_prefix.inc").write_text(main_prefix(mutated))
    (out / "mutation_status").write_text("MUTATIONS_EXACT=missing-call,pal-call,moved-after-core\n")
PY

compile_and_run() {
    local name="$1"
    local run_rc=0
    local run_in_container=0
    shift
    if ! g++ -std=c++11 -Wall -Wextra -Werror -Wno-unused-parameter "$@" \
        -I"$out" pc_port/tests/host_video_mode_boot_test.cpp \
        -o "$out/$name"; then
        if [[ "$name" != UBSan ]]; then
            return 1
        fi
        podman run --rm --userns=keep-id --security-opt label=disable \
            -v "$root:$root" -w "$root" "$container_image" \
            g++ -std=c++11 -Wall -Wextra -Werror -Wno-unused-parameter "$@" \
            -I"$out" pc_port/tests/host_video_mode_boot_test.cpp \
            -o "$out/$name" || return 1
        run_in_container=1
    fi
    if [[ "$run_in_container" -eq 1 ]]; then
        podman run --rm --userns=keep-id --security-opt label=disable \
            -v "$root:$root" -w "$root" "$container_image" \
            "$out/$name" >"$out/$name.stdout" \
            2>"$out/$name.stderr" || run_rc=$?
    else
        "$out/$name" >"$out/$name.stdout" 2>"$out/$name.stderr" || run_rc=$?
    fi
    return "$run_rc"
}

run_regime() {
    local regime="$1"
    shift
    compile_and_run "$regime" "$@"
    grep -q '^HOST VIDEO MODE BOOT PASS:' "$out/$regime.stdout"
}

red_seen=0
for regime in O0 O2 UBSan; do
    flags=(-O0 -g)
    if [[ "$regime" == O2 ]]; then flags=(-O2); fi
    if [[ "$regime" == UBSan ]]; then
        flags=(-O2 -g -fsanitize=undefined -fno-sanitize-recover=all)
    fi
    if run_regime "$regime" "${flags[@]}"; then
        echo "HOST VIDEO MODE BOOT $regime PASS"
    else
        rc=$?
        if [[ ! -s "$out/$regime.stderr" ]] || \
           ! grep -q '^ASSERTION core-start-observes-ntsc$' \
               "$out/$regime.stderr"; then
            echo "HOST VIDEO MODE BOOT $regime UNEXPECTED FAILURE (rc=$rc)" >&2
            sed -n '1,20p' "$out/$regime.stderr" >&2 || true
            exit 1
        fi
        red_seen=1
        echo "HOST VIDEO MODE BOOT $regime SEMANTIC RED (expected before startup fix; rc=$rc)"
        sed -n '1,12p' "$out/$regime.stderr"
    fi
done

if [[ "$red_seen" -ne 0 ]]; then
    echo "HOST VIDEO MODE BOOT RED: startup reaches core with PAL/default mode"
    exit 1
fi

cmp "$out/O0.stdout" "$out/O2.stdout"
cmp "$out/O0.stdout" "$out/UBSan.stdout"

if [[ "$(cat "$out/mutation_status")" != "MUTATIONS_EXACT=missing-call,pal-call,moved-after-core" ]]; then
    echo "HOST VIDEO MODE BOOT RED: mutation set unavailable or non-exact" >&2
    exit 1
fi

for mutant in missing-call pal-call moved-after-core; do
    cp "$out/production.inc" "$out/$mutant/production.inc"
    if g++ -std=c++11 -O2 -Wall -Wextra -Werror -Wno-unused-parameter \
        -I"$out/$mutant" pc_port/tests/host_video_mode_boot_test.cpp \
        -o "$out/$mutant/test"; then
        rc=0
        "$out/$mutant/test" >"$out/$mutant/result.stdout" \
            2>"$out/$mutant/result.stderr" || rc=$?
    else
        rc=125
    fi
    if [[ "$rc" -eq 0 ]]; then
        echo "HOST VIDEO MODE BOOT FAILED TO REJECT $mutant" >&2
        exit 1
    fi
    grep -Eq '^ASSERTION (core-start-observes-ntsc|startup-success)$' \
        "$out/$mutant/result.stderr"
    echo "HOST VIDEO MODE BOOT rejected $mutant"
done

echo "HOST VIDEO MODE BOOT O0/O2/UBSAN PASS; exact missing/PAL/moved mutants rejected"
