#!/usr/bin/env bash
set -u

EXE="${1:-./build/xenogears_pc_port}"
RUN_SECONDS="${RUN_SECONDS:-20}"
STAMP="$(date +%Y%m%d-%H%M%S)"

OUTDIR="logs/run-$STAMP"
SHOTDIR="screenshots/run-$STAMP"
mkdir -p "$OUTDIR" "$SHOTDIR"

BUILD_LOG="$OUTDIR/build.log"
RUN_LOG="$OUTDIR/run.log"
SUMMARY="$OUTDIR/summary.txt"

echo "[capture] exe=$EXE"
echo "[capture] run_seconds=$RUN_SECONDS"
echo "[capture] outdir=$OUTDIR"
echo "[capture] shotdir=$SHOTDIR"

take_screenshot() {
  local out="$1"

  if command -v grim >/dev/null 2>&1; then
    grim "$out"
    return $?
  fi

  if command -v spectacle >/dev/null 2>&1; then
    spectacle -b -n -o "$out"
    return $?
  fi

  if command -v gnome-screenshot >/dev/null 2>&1; then
    gnome-screenshot -f "$out"
    return $?
  fi

  if command -v scrot >/dev/null 2>&1; then
    scrot "$out"
    return $?
  fi

  if command -v import >/dev/null 2>&1; then
    import -window root "$out"
    return $?
  fi

  echo "[capture] ERROR: no screenshot tool found: tried grim, spectacle, gnome-screenshot, scrot, import" >&2
  return 127
}

echo "[capture] build started"
{
  if [ -d build ]; then
    cmake --build build -j"$(nproc)"
  else
    echo "No build/ directory found. Skipping build step."
  fi
} >"$BUILD_LOG" 2>&1
BUILD_RC=$?
echo "[capture] build exit=$BUILD_RC"

if [ "$BUILD_RC" -ne 0 ]; then
  {
    echo "BUILD FAILED"
    echo "build_rc=$BUILD_RC"
    echo "build_log=$BUILD_LOG"
  } | tee "$SUMMARY"
  exit "$BUILD_RC"
fi

if [ ! -x "$EXE" ]; then
  {
    echo "RUN FAILED: executable not found or not executable"
    echo "exe=$EXE"
    echo "build_log=$BUILD_LOG"
  } | tee "$SUMMARY"
  exit 2
fi

echo "[capture] launching game"
"$EXE" >"$RUN_LOG" 2>&1 &
PID=$!

sleep 5
SHOT1="$SHOTDIR/screenshot-5s.png"
take_screenshot "$SHOT1" || true

sleep "$RUN_SECONDS"
SHOT2="$SHOTDIR/screenshot-final.png"
take_screenshot "$SHOT2" || true

if kill -0 "$PID" >/dev/null 2>&1; then
  echo "[capture] process still running; terminating pid=$PID"
  kill "$PID" >/dev/null 2>&1 || true
  sleep 2
  kill -9 "$PID" >/dev/null 2>&1 || true
  RUN_RC=124
else
  wait "$PID"
  RUN_RC=$?
fi

{
  echo "capture_complete=1"
  echo "build_rc=$BUILD_RC"
  echo "run_rc=$RUN_RC"
  echo "build_log=$BUILD_LOG"
  echo "run_log=$RUN_LOG"
  echo "screenshot_5s=$SHOT1"
  echo "screenshot_final=$SHOT2"
  echo
  echo "Recent run log tail:"
  tail -n 80 "$RUN_LOG" || true
} | tee "$SUMMARY"

exit 0