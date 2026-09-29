#!/usr/bin/env python3
"""Run the reviewed retail/file-1 controller differential.

The default path extracts archive 0x20/file 1 from the local retail disc with
the repository extractor.  ``--archive`` exists for an explicitly supplied,
already validated payload; it is not a default or a repository dependency.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
from datetime import datetime, timezone


ROOT = Path(__file__).resolve().parents[2]
TEST = Path(__file__).resolve().with_name("battle_command_file1_controller_retail_test.c")
ADAPTER_C = ROOT / "pc_port/src/battle_mips_adapter.c"
ADAPTER_H = ROOT / "pc_port/src/battle_mips_adapter.h"
EXTRACTOR = ROOT / "tools/scripts/extract_battle_command_file1.py"
DEFAULT_SOURCE = ROOT / (
    "src/battle_command_file1/message_controller_impl.inc"
)
DISC = ROOT / "disc/disc1.bin"

DISC_SHA256 = "39c547a9afc6da15d847ef81a2c6cea1a6516bdfa562cf13b0999b04e8598bda"
PAYLOAD_SHA256 = "64668d85bf48dea46cf6d5f04b38e38ca887877cfef53961f0c26cd2d363b670"
CONTROLLER_SHA256 = "1f11c9ac7117c0d702c6829cb3fb57806d73c6413eaec5f9fbf1ecca2b427a2e"
PAYLOAD_SIZE = 0x4C3C
CONTROLLER_START = 0x1CE8
CONTROLLER_END = 0x21D4


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def hash_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def source_pin(path: Path) -> dict[str, object]:
    return {"path": str(path), "sha256": sha256(path), "size": path.stat().st_size}


def checked_payload(path: Path) -> dict[str, object]:
    data = path.read_bytes()
    if len(data) != PAYLOAD_SIZE:
        raise RuntimeError(f"payload size mismatch: {len(data)} != {PAYLOAD_SIZE}")
    payload_hash = hash_bytes(data)
    if payload_hash != PAYLOAD_SHA256:
        raise RuntimeError(f"payload SHA256 mismatch: {payload_hash}")
    controller_hash = hash_bytes(data[CONTROLLER_START:CONTROLLER_END])
    if controller_hash != CONTROLLER_SHA256:
        raise RuntimeError(f"controller slice SHA256 mismatch: {controller_hash}")
    return {
        "path": str(path),
        "size": len(data),
        "sha256": payload_hash,
        "controller_range": [hex(CONTROLLER_START), hex(CONTROLLER_END)],
        "controller_sha256": controller_hash,
    }


def run_command(command: list[str], log: Path, *, expected: int | None = None) -> subprocess.CompletedProcess[str]:
    result = subprocess.run(command, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    log.write_text(result.stdout)
    if expected is not None and result.returncode != expected:
        raise RuntimeError(f"command exited {result.returncode}, expected {expected}: {' '.join(command)}")
    return result


def compile_binary(
    compiler: str,
    source: Path,
    include_dir: Path,
    output: Path,
    optimization: str,
    *,
    ubsan: bool = False,
) -> list[str]:
    command = [
        compiler,
        "-std=c17",
        "-Wall",
        "-Wextra",
        "-Werror",
        f"-I{ROOT / 'pc_port/src'}",
        f"-I{include_dir}",
        "-fno-pie",
        "-no-pie",
        "-DAUDIT_YIELDS=3",
        str(TEST),
        str(ADAPTER_C),
        "-x",
        "c",
        str(source),
        optimization,
    ]
    if ubsan:
        command.extend(["-fsanitize=undefined", "-fno-sanitize-recover=all"])
    command.extend(["-o", str(output)])
    return command


def checked_coverage(output: str) -> dict[str, int]:
    rows = [line.split() for line in output.splitlines() if line.startswith("PC ")]
    if len(rows) != 315 or any(len(row) != 4 for row in rows):
        raise RuntimeError(f"retail PC coverage mismatch: rows={len(rows)}")
    if any(int(row[2]) <= 0 for row in rows):
        raise RuntimeError("retail PC coverage contains an unvisited slot")
    branches = [row for row in rows if int(row[3]) != 0]
    if len(branches) != 21 or any(int(row[3]) != 3 for row in branches):
        raise RuntimeError(f"retail branch coverage mismatch: rows={len(branches)}")
    return {"retail_slots_covered": len(rows), "branches_both_outcomes": len(branches)}


MUTATIONS: dict[str, tuple[str, str]] = {
    "wrong_constructor_window": (
        "func_80032F54(D_800D2DAC,",
        "func_80032F54(D_800D2D28,",
    ),
    "wrong_string_table": (
        "selected_string = GetStringEntry(D_800D3340, arg0);",
        "selected_string = GetStringEntry(D_800D2DAC, arg0);",
    ),
    "wrong_queue_window": (
        "func_80034714(D_800D2DAC, selected_string);",
        "func_80034714(D_800D2D28, selected_string);",
    ),
    "stale_queue_window": (
        "        selected_string = GetStringEntry(D_800D3340, arg0);\n"
        "        func_80034714(D_800D2DAC, selected_string);",
        "        {\n"
        "            xbcf1_u8 *stale_window = D_800D2DAC;\n"
        "            selected_string = GetStringEntry(D_800D3340, arg0);\n"
        "            func_80034714(stale_window, selected_string);\n"
        "        }",
    ),
    "wrong_constructor_rows": (
        "XBCF1_CONTROL(D_800D3278)->rows)));",
        "XBCF1_CONTROL(D_800D3278)->units)));",
    ),
    "missing_window_reset": (
        "func_80034614(D_800D2DAC);",
        "(void)D_800D2DAC;",
    ),
    "missing_teardown": (
        "func_800346D4(D_800D2DAC);",
        "(void)D_800D2DAC;",
    ),
    "missing_default_restore_word": (
        "for (i = 0; i < 5; i++)",
        "for (i = 0; i < 4; i++)",
    ),
    "missing_close_c9": (
        "XBCF1_U8(D_800D2D28, 0xC9) = 0;",
        "(void)D_800D2D28;",
    ),
    "missing_open_c9": (
        "XBCF1_U8(D_800D2D28, 0xC9) = 1;",
        "(void)D_800D2D28;",
    ),
    "missing_cycle_reset": (
        "xeno_battle_command_file1_cycle_9C1C = 4;",
        "/* review negative control: missing cycle reset */",
    ),
}


def make_mutants(source: Path, directory: Path) -> list[Path]:
    text = source.read_text()
    mutants = []
    for name, (old, new) in MUTATIONS.items():
        count = text.count(old)
        if count != 1:
            raise RuntimeError(f"mutation anchor {name} count={count}")
        path = directory / f"{name}.c"
        path.write_text(text.replace(old, new, 1))
        mutants.append(path)
    return mutants


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, default=DEFAULT_SOURCE)
    parser.add_argument("--archive", type=Path, help="explicit validated archive payload for development")
    parser.add_argument("--output", type=Path, help="preserved output directory; default is unique")
    args = parser.parse_args()

    out = args.output.resolve() if args.output else Path(tempfile.mkdtemp(prefix="xeno-file1-controller-retail-"))
    if out.exists() and any(out.iterdir()):
        raise RuntimeError(f"refusing non-empty output directory: {out}")
    out.mkdir(parents=True, exist_ok=True)
    build = out / "build"
    logs = out / "logs"
    mutants_dir = out / "mutants"
    build.mkdir()
    logs.mkdir()
    mutants_dir.mkdir()
    source_header = args.source.resolve().parent / "xeno_battle_command_file1_controller.h"
    if not source_header.is_file():
        raise RuntimeError(f"missing source header beside candidate: {source_header}")

    manifest: dict[str, object] = {
        "started_utc": datetime.now(timezone.utc).isoformat(),
        "repo": str(ROOT),
        "runner": source_pin(Path(__file__).resolve()),
        "fixture": source_pin(TEST),
        "adapter_c": source_pin(ADAPTER_C),
        "adapter_h": source_pin(ADAPTER_H),
        "source": source_pin(args.source.resolve()),
        "source_header": source_pin(source_header),
        "python": {"executable": sys.executable, "version": sys.version},
        "pins": {
            "disc_sha256": DISC_SHA256,
            "payload_sha256": PAYLOAD_SHA256,
            "controller_sha256": CONTROLLER_SHA256,
            "payload_size": PAYLOAD_SIZE,
            "controller_range": [hex(CONTROLLER_START), hex(CONTROLLER_END)],
        },
        "audit": {
            "audit_yields": 3,
            "retail_slots_required": 315,
            "branches_both_outcomes_required": 21,
        },
        "output": str(out),
    }

    source_bindings = args.source.resolve().parent / "message_controller_bindings.inc"
    if source_bindings.is_file():
        manifest["source_bindings"] = source_pin(source_bindings)

    try:
        compiler = shutil.which("clang")
        if not compiler:
            raise RuntimeError("clang is required")
        compiler_version = subprocess.run([compiler, "--version"], text=True, stdout=subprocess.PIPE, check=True).stdout
        manifest["compiler"] = {"path": compiler, "version": compiler_version}
        if not EXTRACTOR.is_file():
            raise RuntimeError(f"missing extractor: {EXTRACTOR}")
        if not DISC.is_file():
            raise RuntimeError(f"missing retail disc: {DISC}")
        disc_pin = source_pin(DISC)
        if disc_pin["sha256"] != DISC_SHA256:
            raise RuntimeError(f"disc SHA256 mismatch: {disc_pin['sha256']}")
        manifest["disc"] = disc_pin

        payload = out / "archive20_file1.bin"
        if args.archive:
            shutil.copyfile(args.archive.resolve(), payload)
            manifest["payload_source"] = {"mode": "explicit", **source_pin(args.archive.resolve())}
        else:
            extraction = run_command(
                [sys.executable, str(EXTRACTOR), "--disc", str(DISC), "--output", str(payload)],
                out / "extract.log",
                expected=0,
            )
            manifest["payload_source"] = {"mode": "extractor", "extractor": source_pin(EXTRACTOR), "stdout": extraction.stdout}
        manifest["payload"] = checked_payload(payload)

        include_dir = args.source.resolve().parent
        for name, optimization, ubsan in (("O0", "-O0", False), ("O2", "-O2", False), ("UBSan", "-O1", True)):
            binary = build / f"differential_{name}"
            compile_log = logs / f"{name}.compile.log"
            command = compile_binary(compiler, args.source.resolve(), include_dir, binary, optimization, ubsan=ubsan)
            run_command(command, compile_log, expected=0)
            result = run_command([str(binary), str(payload)], logs / f"{name}.run.log", expected=0)
            if "candidate differential GREEN cases=463" not in result.stdout:
                raise RuntimeError(f"{name} lacks 463-case GREEN marker")
            if "DIFF FAIL" in result.stdout or "overflow" in result.stdout.lower():
                raise RuntimeError(f"{name} emitted differential/overflow failure")
            manifest.setdefault("coverage", []).append(
                {"name": name, **checked_coverage(result.stdout)}
            )

        mutant_results = []
        for mutant in make_mutants(args.source.resolve(), mutants_dir):
            name = mutant.stem
            binary = build / f"mutant_{name}"
            run_command(
                compile_binary(compiler, mutant, include_dir, binary, "-O2"),
                logs / f"mutant-{name}.compile.log",
                expected=0,
            )
            result = run_command([str(binary), str(payload)], logs / f"mutant-{name}.run.log")
            if result.returncode != 1 or "DIFF FAIL" not in result.stdout:
                raise RuntimeError(
                    f"mutant {name} did not fail semantically: exit={result.returncode}"
                )
            mutant_results.append({"name": name, "exit": result.returncode, "marker": "DIFF FAIL"})
        manifest["mutants"] = mutant_results
        for key in ("runner", "fixture", "adapter_c", "adapter_h", "source",
                    "source_header", "source_bindings"):
            if key in manifest:
                pin = manifest[key]
                if sha256(Path(pin["path"])) != pin["sha256"]:
                    raise RuntimeError(f"source changed during regression: {pin['path']}")
        manifest["status"] = "PASS"
        return 0
    finally:
        manifest["finished_utc"] = datetime.now(timezone.utc).isoformat()
        (out / "manifest.json").write_text(json.dumps(manifest, indent=2, sort_keys=True) + "\n")


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as error:
        print(f"ERROR: {error}", file=sys.stderr)
        raise SystemExit(1)
