#!/usr/bin/env python3
"""Rebuild archive 0x20/file 1 in isolation and compare it with retail.

The default gate reconstructs the complete module from Splat-generated assembly.
``--with-c`` additionally builds the current controller C wrapper and reports its
symbol placement, function bytes, and whole-module result independently.
All generated files and retail bytes remain in a retained /tmp directory.
"""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile
from typing import Any


ROOT = Path(__file__).resolve().parents[2]
DEFAULT_IMAGE = "localhost/xenogears-dev-toolchain:current"
MODULE_SIZE = 19_516
MODULE_SHA256 = "64668d85bf48dea46cf6d5f04b38e38ca887877cfef53961f0c26cd2d363b670"
MODULE_SHA1 = "846845462141838c9b94a4a61c315b0876bde352"
LOAD_ADDRESS = 0x801E5000
CONTROLLER_START = 0x801E6CE8
CONTROLLER_END = 0x801E71D4
CONTROLLER_OFFSET = CONTROLLER_START - LOAD_ADDRESS
CONTROLLER_SIZE = CONTROLLER_END - CONTROLLER_START
CONTROLLER_SHA256 = "1f11c9ac7117c0d702c6829cb3fb57806d73c6413eaec5f9fbf1ecca2b427a2e"
CONTROLLER_SYMBOL = "xeno_battle_command_file1_controller"

ASM_SOURCES = (
    "asm/battle_command_file1/data/dispatch.rodata.s",
    "asm/battle_command_file1/before_controller.s",
    "asm/battle_command_file1/message_controller.s",
    "asm/battle_command_file1/after_controller.s",
    "asm/battle_command_file1/data/module_data.data.s",
)
C_ASM_SOURCES = tuple(path for path in ASM_SOURCES if not path.endswith("message_controller.s"))
C_FILES = (
    "message_controller.c",
    "message_controller_impl.inc",
    "message_controller_bindings.inc",
    "xeno_battle_command_file1_controller.h",
)


class CheckError(RuntimeError):
    pass


def sha256(path: Path) -> str:
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def first_difference(actual: bytes, expected: bytes) -> dict[str, Any] | None:
    limit = min(len(actual), len(expected))
    for offset in range(limit):
        if actual[offset] != expected[offset]:
            return {
                "offset": offset,
                "actual": f"0x{actual[offset]:02X}",
                "expected": f"0x{expected[offset]:02X}",
            }
    if len(actual) != len(expected):
        return {"offset": limit, "actual": "EOF" if len(actual) == limit else "data",
                "expected": "EOF" if len(expected) == limit else "data"}
    return None


def run(command: list[str], *, cwd: Path, log: Path, manifest: dict[str, Any],
        check: bool = True) -> subprocess.CompletedProcess[str]:
    result = subprocess.run(command, cwd=cwd, text=True, stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT)
    log.parent.mkdir(parents=True, exist_ok=True)
    log.write_text(result.stdout, encoding="utf-8")
    manifest.setdefault("commands", []).append({
        "argv": command, "cwd": str(cwd), "log": str(log),
        "returncode": result.returncode,
    })
    if check and result.returncode:
        raise CheckError(f"command failed ({result.returncode}); see {log}")
    return result


def prepare_project(destination: Path, config_text: str, retail: Path,
                    *, assembly_controller: bool) -> None:
    destination.mkdir(parents=True)
    for directory in ("config", "disc", "tools", "logs"):
        (destination / directory).mkdir()
    shutil.copy2(retail, destination / "disc/battle_command_file1.bin")
    shutil.copy2(ROOT / "config/symbol_addrs.battle_command_file1.txt",
                 destination / "config/symbol_addrs.battle_command_file1.txt")
    shutil.copy2(ROOT / "config/symbol_addrs.slus_006.64.txt",
                 destination / "config/symbol_addrs.slus_006.64.txt")
    shutil.copytree(ROOT / "tools/splat_ext", destination / "tools/splat_ext",
                    dirs_exist_ok=True)
    if assembly_controller:
        old = "      - [0x1CE8, c, message_controller]"
        new = "      - [0x1CE8, asm, message_controller]"
        if config_text.count(old) != 1:
            raise CheckError("controller C subsegment is absent or ambiguous in config")
        config_text = config_text.replace(old, new)
    (destination / "config/battle_command_file1.yaml").write_text(config_text,
                                                                   encoding="utf-8")


def container_prefix(image: str, project: Path) -> list[str]:
    return [
        "podman", "run", "--rm", "--userns=keep-id", "--security-opt", "label=disable",
        "-v", f"{ROOT}:/repo:ro", "-v", f"{project}:/work", "-w", "/work", image,
    ]


def write_build_script(project: Path, *, c_controller: bool) -> Path:
    sources = C_ASM_SOURCES if c_controller else ASM_SOURCES
    lines = [
        "#!/bin/sh", "set -eu", "mkdir -p build/out",
    ]
    for source in sources:
        output = f"build/{source}.o"
        lines.extend([
            f"mkdir -p {Path(output).parent.as_posix()}",
            "mips-linux-gnu-as -EL -I /repo/include -march=r3000 -mtune=r3000 "
            f"-no-pad-sections -o {output} {source}",
        ])
    if c_controller:
        lines.extend([
            "mkdir -p build/src/battle_command_file1",
            "/repo/tools/gcc-2.6.0-psx/gcc -B/repo/tools/gcc-2.6.0-psx/ "
            "-I /repo/include -I src/battle_command_file1 -D_LANGUAGE_C "
            "-O2 -G0 -S "
            "-o build/src/battle_command_file1/message_controller.s "
            "src/battle_command_file1/message_controller.c",
            "python3 /repo/tools/maspsx/maspsx.py --use-comm-section "
            "--run-assembler -EL -I /repo/include -O2 -G0 -march=r3000 "
            "-mtune=r3000 -no-pad-sections "
            "-o build/src/battle_command_file1/message_controller.c.o "
            "build/src/battle_command_file1/message_controller.s",
        ])
    lines.extend([
        "mips-linux-gnu-ld -EL -nostdlib --no-check-sections "
        "-Map build/out/module.map -T linker/battle_command_file1.ld "
        "-T linker/undefined_syms_auto.battle_command_file1.txt "
        "-T linker/undefined_funcs_auto.battle_command_file1.txt "
        "-o build/out/module.elf",
        "mips-linux-gnu-objcopy -O binary build/out/module.elf build/out/module.bin",
        "mips-linux-gnu-nm -S -n build/out/module.elf > build/out/module.nm",
        "mips-linux-gnu-size -A build/out/module.elf > build/out/module.size",
    ])
    script = project / ("build_c.sh" if c_controller else "build_asm.sh")
    script.write_text("\n".join(lines) + "\n", encoding="utf-8")
    script.chmod(0o755)
    return script


def split_project(project: Path, manifest: dict[str, Any], *, assembly_controller: bool) -> None:
    run(["splat", "split", "config/battle_command_file1.yaml"], cwd=project,
        log=project / "logs/splat.log", manifest=manifest)
    expected_sources = ASM_SOURCES if assembly_controller else C_ASM_SOURCES
    expected = [project / path for path in expected_sources]
    missing = [str(path) for path in expected if not path.is_file()]
    if missing:
        raise CheckError(f"Splat did not generate expected assembly: {missing}")
    linker = project / "linker/battle_command_file1.ld"
    if not linker.is_file():
        raise CheckError("Splat did not generate the module linker script")


def parse_symbol(nm_path: Path) -> tuple[int, int] | None:
    pattern = re.compile(
        rf"^([0-9A-Fa-f]+)\s+([0-9A-Fa-f]+)\s+[Tt]\s+{re.escape(CONTROLLER_SYMBOL)}$"
    )
    for line in nm_path.read_text(encoding="utf-8").splitlines():
        match = pattern.match(line.strip())
        if match:
            return int(match.group(1), 16), int(match.group(2), 16)
    return None


def validate_config(config_text: str) -> None:
    required = (
        f"sha1: {MODULE_SHA1}",
        "target_path: disc/battle_command_file1.bin",
        "vram: 0x801E5000",
        "subalign: 4",
        "      - [0x1CE8, c, message_controller]",
        "      - [0x21D4, asm, after_controller]",
        "  - [0x4C3C]",
    )
    missing = [item for item in required if item not in config_text]
    if missing:
        raise CheckError(f"config is missing pinned layout entries: {missing}")
    if "generate_asm_macros_files" in config_text:
        raise CheckError("unsupported generate_asm_macros_files remains in config")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--disc", type=Path, default=ROOT / "disc/disc1.bin")
    parser.add_argument("--image", default=DEFAULT_IMAGE)
    parser.add_argument("--with-c", action="store_true",
                        help="also compile and compare the current controller C wrapper")
    parser.add_argument("--controller-source", type=Path,
                        help="wrapper source for --with-c (sibling .inc/header still come from repo)")
    args = parser.parse_args()
    if args.controller_source and not args.with_c:
        parser.error("--controller-source requires --with-c")

    work = Path(tempfile.mkdtemp(prefix="xeno-file1-build-check-", dir="/tmp"))
    work.chmod(0o755)
    manifest: dict[str, Any] = {
        "workdir": str(work), "repository": str(ROOT), "image": args.image,
        "pins": {
            "module_size": MODULE_SIZE, "module_sha256": MODULE_SHA256,
            "module_sha1": MODULE_SHA1, "load_address": f"0x{LOAD_ADDRESS:08X}",
            "controller_start": f"0x{CONTROLLER_START:08X}",
            "controller_end": f"0x{CONTROLLER_END:08X}",
            "controller_size": CONTROLLER_SIZE,
            "controller_sha256": CONTROLLER_SHA256,
        },
        "results": {},
    }
    manifest_path = work / "manifest.json"
    exit_code = 1
    try:
        config_path = ROOT / "config/battle_command_file1.yaml"
        extractor = ROOT / "tools/scripts/extract_battle_command_file1.py"
        config_text = config_path.read_text(encoding="utf-8")
        validate_config(config_text)
        manifest["inputs"] = {
            "config": str(config_path), "config_sha256": sha256(config_path),
            "extractor": str(extractor), "extractor_sha256": sha256(extractor),
            "disc": str(args.disc.resolve()),
            "checker_sha256": sha256(Path(__file__).resolve()),
            "symbol_maps": {
                name: sha256(ROOT / "config" / name) for name in (
                    "symbol_addrs.battle_command_file1.txt",
                    "symbol_addrs.slus_006.64.txt",
                )
            },
            "assembly_includes": {
                name: sha256(ROOT / "include" / name)
                for name in ("macro.inc", "gte_macros.inc")
            },
            "repo_mounted_tools": {
                name: sha256(ROOT / name) for name in (
                    "tools/gcc-2.6.0-psx/gcc",
                    "tools/gcc-2.6.0-psx/cc1",
                    "tools/maspsx/maspsx.py",
                )
            },
        }

        image_info = run(["podman", "image", "inspect", args.image, "--format",
                          "{{.Id}} {{.Digest}}"], cwd=ROOT,
                         log=work / "image-inspect.log", manifest=manifest)
        manifest["toolchain_image_identity"] = image_info.stdout.strip()
        versions = run(container_prefix(args.image, work) + ["sh", "-c",
            "mips-linux-gnu-as --version | head -1; "
            "mips-linux-gnu-ld --version | head -1; "
            "/repo/tools/gcc-2.6.0-psx/cc1 -version </dev/null 2>&1 | head -3 || true"],
            cwd=ROOT, log=work / "toolchain-versions.log", manifest=manifest)
        manifest["toolchain_versions"] = versions.stdout.splitlines()
        splat_version = run(["splat", "-V"], cwd=ROOT, log=work / "splat-version.log",
                            manifest=manifest)
        manifest["splat_version"] = splat_version.stdout.strip()

        retail = work / "retail-battle-command-file1.bin"
        extraction = run([sys.executable, str(extractor), "--disc", str(args.disc),
                          "--output", str(retail)], cwd=ROOT,
                         log=work / "extract.log", manifest=manifest)
        manifest["extraction"] = json.loads(extraction.stdout)
        retail_bytes = retail.read_bytes()
        if len(retail_bytes) != MODULE_SIZE or sha256(retail) != MODULE_SHA256:
            raise CheckError("extractor output does not match pinned full-module bytes")
        if hashlib.sha256(retail_bytes[CONTROLLER_OFFSET:
                                       CONTROLLER_OFFSET + CONTROLLER_SIZE]).hexdigest() != CONTROLLER_SHA256:
            raise CheckError("extractor output does not match pinned controller bytes")

        asm_project = work / "full-asm"
        prepare_project(asm_project, config_text, retail, assembly_controller=True)
        split_project(asm_project, manifest, assembly_controller=True)
        asm_linker = (asm_project / "linker/battle_command_file1.ld").read_text(encoding="utf-8")
        if "SUBALIGN(4)" not in asm_linker:
            raise CheckError("full-ASM generated linker lacks SUBALIGN(4)")
        asm_script = write_build_script(asm_project, c_controller=False)
        run(container_prefix(args.image, asm_project) + ["sh", f"/work/{asm_script.name}"],
            cwd=ROOT, log=asm_project / "logs/build.log", manifest=manifest)
        asm_binary = asm_project / "build/out/module.bin"
        asm_bytes = asm_binary.read_bytes()
        asm_pass = asm_bytes == retail_bytes
        manifest["results"]["full_asm_module"] = {
            "pass": asm_pass, "size": len(asm_bytes), "sha256": sha256(asm_binary),
            "first_difference": first_difference(asm_bytes, retail_bytes),
            "linker_sha256": sha256(asm_project / "linker/battle_command_file1.ld"),
        }
        print(f"BATTLE_COMMAND_FILE1_FULL_ASM_MODULE {'PASS' if asm_pass else 'FAIL'}")

        c_ok = True
        if args.with_c:
            try:
                c_project = work / "c-controller"
                prepare_project(c_project, config_text, retail, assembly_controller=False)
                source_dir = c_project / "src/battle_command_file1"
                source_dir.mkdir(parents=True)
                for filename in C_FILES:
                    source = ROOT / "src/battle_command_file1" / filename
                    if filename == "message_controller.c" and args.controller_source:
                        source = args.controller_source.resolve()
                    shutil.copy2(source, source_dir / filename)
                manifest["inputs"]["c_sources"] = {
                    name: sha256(source_dir / name) for name in C_FILES
                }
                split_project(c_project, manifest, assembly_controller=False)
                c_linker_path = c_project / "linker/battle_command_file1.ld"
                c_linker = c_linker_path.read_text(encoding="utf-8")
                subalign_pass = "SUBALIGN(4)" in c_linker
                c_script = write_build_script(c_project, c_controller=True)
                run(container_prefix(args.image, c_project) +
                    ["sh", f"/work/{c_script.name}"], cwd=ROOT,
                    log=c_project / "logs/build.log", manifest=manifest)
                c_binary = c_project / "build/out/module.bin"
                c_bytes = c_binary.read_bytes()
                symbol = parse_symbol(c_project / "build/out/module.nm")
                symbol_pass = symbol is not None and symbol[0] == CONTROLLER_START
                function_bytes = b""
                if symbol is not None:
                    offset = symbol[0] - LOAD_ADDRESS
                    if 0 <= offset <= len(c_bytes) and symbol[1] <= len(c_bytes) - offset:
                        function_bytes = c_bytes[offset:offset + symbol[1]]
                expected_controller = retail_bytes[CONTROLLER_OFFSET:
                                                   CONTROLLER_OFFSET + CONTROLLER_SIZE]
                function_pass = (symbol_pass and symbol is not None and
                                 symbol[1] == CONTROLLER_SIZE and
                                 function_bytes == expected_controller)
                module_pass = c_bytes == retail_bytes
                c_ok = subalign_pass and symbol_pass and function_pass and module_pass
                manifest["results"]["c_controller"] = {
                    "build_status": "PASS",
                    "linker_subalign_4": subalign_pass,
                    "symbol": None if symbol is None else {
                        "address": f"0x{symbol[0]:08X}", "size": symbol[1]},
                    "symbol_pass": symbol_pass,
                    "function_pass": function_pass,
                    "function_sha256": hashlib.sha256(function_bytes).hexdigest(),
                    "function_first_difference": first_difference(function_bytes,
                                                                    expected_controller),
                    "full_module_pass": module_pass, "full_module_size": len(c_bytes),
                    "full_module_sha256": sha256(c_binary),
                    "full_module_first_difference": first_difference(c_bytes, retail_bytes),
                    "linker_sha256": sha256(c_linker_path),
                }
                print("BATTLE_COMMAND_FILE1_C_BUILD PASS")
                print(f"BATTLE_COMMAND_FILE1_C_LINKER_SUBALIGN4 "
                      f"{'PASS' if subalign_pass else 'FAIL'}")
                print(f"BATTLE_COMMAND_FILE1_C_SYMBOL {'PASS' if symbol_pass else 'FAIL'}")
                print(f"BATTLE_COMMAND_FILE1_C_CONTROLLER_BYTES "
                      f"{'PASS' if function_pass else 'FAIL'}")
                print(f"BATTLE_COMMAND_FILE1_C_FULL_MODULE "
                      f"{'PASS' if module_pass else 'FAIL'}")
            except (CheckError, OSError, ValueError) as c_error:
                c_ok = False
                manifest["results"]["c_controller"] = {
                    "build_status": "ERROR", "error": str(c_error),
                }
                print(f"BATTLE_COMMAND_FILE1_C_BUILD ERROR: {c_error}")
                print("BATTLE_COMMAND_FILE1_C_LINKER_SUBALIGN4 NOT_RUN")
                print("BATTLE_COMMAND_FILE1_C_SYMBOL NOT_RUN")
                print("BATTLE_COMMAND_FILE1_C_CONTROLLER_BYTES NOT_RUN")
                print("BATTLE_COMMAND_FILE1_C_FULL_MODULE NOT_RUN")
        else:
            manifest["results"]["c_controller"] = {"status": "NOT_RUN"}
            print("BATTLE_COMMAND_FILE1_C_COMPARE NOT_RUN")

        exit_code = 0 if asm_pass and c_ok else 1
    except (CheckError, OSError, ValueError, json.JSONDecodeError) as error:
        manifest["error"] = str(error)
        print(f"BATTLE_COMMAND_FILE1_CHECK ERROR: {error}", file=sys.stderr)
    finally:
        manifest["exit_code"] = exit_code
        manifest_path.write_text(json.dumps(manifest, indent=2, sort_keys=True) + "\n",
                                 encoding="utf-8")
        print(f"BATTLE_COMMAND_FILE1_EVIDENCE {work}")
    return exit_code


if __name__ == "__main__":
    raise SystemExit(main())
