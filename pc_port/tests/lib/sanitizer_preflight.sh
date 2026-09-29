# shellcheck shell=bash
#
# UBSan preflight for the pc_port differential harness.
#
# WHY THIS EXISTS
# ---------------
# PORT_GOAL_AND_PLAN.md names the harness at "O0/O2/UBSan" as the verification
# bar that justifies NOT byte-matching.  It is the thing this project trades
# byte parity away for, so a third of that bar silently not executing is
# expensive.
#
# Before this guard, on a host without the UBSan runtime the UBSan regime
# failed at the LINK step -- late, after the O0 and O2 regimes had already
# printed their PASS lines -- with nothing but:
#
#     /usr/bin/ld.bfd: cannot find /usr/lib64/libubsan.so.1.0.0
#     collect2: error: ld returned 1 exit status
#
# The runner did then exit nonzero (every runner uses `set -e`, so none of them
# ever printed a false PASS).  The failure was in the SUMMARY, not the script:
# three separate workers in a single session read that output -- two PASS lines
# followed by a linker error -- and recorded "harness green at O0/O2/UBSan,
# UBSan environmental".  The regime had never executed on this host at all.
#
# This turns that into an explicit, early, unmissable NOT RUN, before any
# regime prints anything, so there is no PASS line above it to misread.
#
# It does not weaken any check: the exit is still nonzero, exactly as the link
# failure was.  It only makes the reason legible and moves it to the front.
#
# USAGE
#     source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"
#     ubsan_preflight              # probes ${CC:-gcc}
#     ubsan_preflight clang        # probe a specific compiler
#
# Exit status 97 is reserved for "a required sanitizer regime cannot run", so
# a sweep driver can tell it apart from a genuine test failure.

UBSAN_PREFLIGHT_EXIT=97

ubsan_preflight() {
    local ccs=("$@")
    [ "${#ccs[@]}" -eq 0 ] && ccs=("${CC:-gcc}")

    local probe cc rc
    probe="$(mktemp -d "${TMPDIR:-/tmp}/ubsan-preflight.XXXXXX")" || return 0
    printf 'int main(void){return 0;}\n' >"$probe/p.c"

    for cc in "${ccs[@]}"; do
        rc=0
        command -v "$cc" >/dev/null 2>&1 || rc=127
        if [ "$rc" -eq 0 ]; then
            "$cc" -fsanitize=undefined -fno-sanitize-recover=all \
                "$probe/p.c" -o "$probe/p" >"$probe/log" 2>&1 || rc=$?
        fi
        # linking is not enough -- the runtime has to load too
        if [ "$rc" -eq 0 ]; then
            "$probe/p" >/dev/null 2>"$probe/runlog" || rc=$?
        fi
        [ "$rc" -eq 0 ] && continue

        _ubsan_preflight_report "$cc" "$rc" "$probe"
        rm -rf "$probe"
        exit "$UBSAN_PREFLIGHT_EXIT"
    done

    rm -rf "$probe"
}

_ubsan_preflight_report() {
    local cc="$1" rc="$2" probe="$3"
    local suite="${0##*/}"
    local msg
    msg=$(
        echo "############################################################"
        echo "### UBSAN NOT RUN -- THIS IS NOT A PASS"
        echo "###"
        echo "### suite    : $suite"
        echo "### compiler : $cc"
        if [ "$rc" -eq 127 ]; then
            echo "### reason   : compiler '$cc' is not on PATH"
        else
            echo "### reason   : -fsanitize=undefined could not be built/run"
            echo "###            (exit $rc)"
        fi
        echo "###"
        echo "### The O0 and O2 regimes of this suite were NOT attempted."
        echo "### Do not record this run as \"O0/O2/UBSan green\"."
        echo "###"
        echo "### This host needs a UBSan runtime.  Known-good environment:"
        echo "###   the podman toolchain container (Ubuntu 24.04, gcc 13,"
        echo "###   libubsan1) -- UBSan works there.  A bare Fedora host"
        echo "###   needs:  sudo dnf install libubsan"
        echo "###   See docs/ai_context/HARNESS_REGIMES.md"
        echo "###"
        [ -s "$probe/log" ] && { echo "### toolchain output:";
            sed 's/^/###   /' "$probe/log" | head -12; }
        echo "############################################################"
    )
    # Full banner on stderr; a single unmissable marker on stdout too, so a
    # scraper that only captures stdout still cannot read the run as a pass.
    # One line rather than the whole banner, so `2>&1` does not double it.
    printf 'UBSAN NOT RUN -- THIS IS NOT A PASS -- suite=%s cc=%s (details on stderr)\n' \
        "$suite" "$cc"
    printf '%s\n' "$msg" >&2
}
