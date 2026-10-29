#!/usr/bin/env bash
# Run a list of warp / step sweep targets, N at a time, HEADLESS ONLY.
#   warp_sweep_run.sh BIN OUTROOT CKPT TARGETS_TSV [PARALLEL]
# TARGETS_TSV columns (tab separated, '#' comments):
#   id  kind  map  entrance  scenario  walk  name  [continuation k=v,k=v]
#   kind = warp | step ; for kind=step, `map`/`entrance` are the step's NEXT
#   location and `scenario` holds the step's writes as "addr:value,addr:value"
#   (decimal, no spaces); applied with `dev stepraw`.
# Each target gets OUTROOT/<id>/{target.json,run.log,route.jsonl,final_frame.bmp}.
# Judge with: python3 pc_port/tests/warp_sweep_analyze.py OUTROOT
unset DISPLAY WAYLAND_DISPLAY XAUTHORITY
set -u
BIN=$(readlink -f "$1"); OUTROOT=$(mkdir -p "$2" && readlink -f "$2"); CKPT=$(readlink -f "$3"); TSV=$4; PAR=${5:-4}
HERE="$(cd "$(dirname "$0")" && pwd)"
export XENO_DATA_DIR=${XENO_DATA_DIR:?XENO_DATA_DIR must point at the disc files}
one() {
    local id kind map ent scen walk name rest
    IFS=$'\t' read -r id kind map ent scen walk name cont <<<"$1"
    local d="$OUTROOT/$id"
    mkdir -p "$d"
    if [ "$kind" = warp ] || [ "$kind" = tablewarp ]; then
        printf '{"id":"%s","kind":"warp","map":%s,"entrance":%s,"scenario":%s,"walk":%s,"name":"%s"}\n' \
            "$id" "$map" "$ent" "${scen:--1}" "$walk" "$name" >"$d/target.json"
    else
        printf '{"id":"%s","kind":"step","map":%s,"entrance":%s,"writes":"%s","walk":%s,"name":"%s","continuation":"%s"}\n' \
            "$id" "$map" "$ent" "$scen" "$walk" "$name" "${cont:-}" >"$d/target.json"
    fi
    if [ "$kind" = warp ]; then
        bash "$HERE/warp_sweep_one.sh" "$BIN" "$d" "$CKPT" warp "$map" "$ent" "${scen:--1}"
    elif [ "$kind" = tablewarp ]; then
        bash "$HERE/warp_sweep_one.sh" "$BIN" "$d" "$CKPT" tablewarp "$scen"
    elif [ "$kind" = tablestep ] || [ "$kind" = tablestepx ]; then
        SWEEP_CONT_REGEX=$(python3 -c 'import sys
print(".*".join("\"%s\":%s" % (k, v if v.lstrip("-").isdigit() else "\"%s\"" % v) for k, _, v in (p.partition("=") for p in sys.argv[1].split(","))) if sys.argv[1] else "")' "${cont:-}") \
            bash "$HERE/warp_sweep_one.sh" "$BIN" "$d" "$CKPT" "$kind" "$scen"
    else
        # continuation "ev=var,addr=0,new=8" -> regex over the route.jsonl line
        SWEEP_CONT_REGEX=$(python3 -c 'import sys
print(".*".join("\"%s\":%s" % (k, v if v.lstrip("-").isdigit() else "\"%s\"" % v) for k, _, v in (p.partition("=") for p in sys.argv[1].split(","))) if sys.argv[1] else "")' "${cont:-}") \
            bash "$HERE/warp_sweep_one.sh" "$BIN" "$d" "$CKPT" "${SWEEP_STEPMODE:-cont}" "$map" "$ent" "$scen"
    fi
}
export -f one
export OUTROOT BIN CKPT HERE
grep -v '^#' "$TSV" | grep -v '^$' | xargs -P "$PAR" -d '\n' -I{} bash -c 'one "$1"' _ {}
