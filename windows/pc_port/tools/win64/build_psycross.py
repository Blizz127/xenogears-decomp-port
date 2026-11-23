#!/usr/bin/env python3
"""Cross-compile the replayed PsyCross SDK for native x86-64 Windows."""

import argparse
from concurrent.futures import ThreadPoolExecutor
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile


def sha256(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def output(command):
    return subprocess.check_output(command, text=True).strip()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sdk", "--sdk-dir", type=Path, required=True,
                        help="SDK selected by psycross_patch_replay.sh --select")
    parser.add_argument("--output", "--out", type=Path, required=True,
                        help="directory for libpsycross.a and sdk-receipt.json")
    parser.add_argument("--sdl-include", type=Path, required=True, help="directory containing SDL.h")
    parser.add_argument("--openal-include", type=Path, required=True, help="directory containing AL/alext.h")
    parser.add_argument("--zig", default="zig")
    parser.add_argument("--jobs", type=int, default=4)
    args = parser.parse_args()
    if args.jobs < 1:
        parser.error("--jobs must be positive")
    port = Path(__file__).resolve().parents[2]
    sdk = args.sdk.resolve()
    sdl = args.sdl_include.resolve()
    al = args.openal_include.resolve()
    for header in (sdl / "SDL.h", al / "AL/alext.h", sdk / "include/psx/libgpu.h"):
        if not header.is_file():
            parser.error(f"missing header {header}")
    if "PsyX_OtTagMatchesHostSlot" not in (sdk / "include/psx/libgpu.h").read_text():
        parser.error("the selected SDK lacks Win64 ordering-table layout support")
    sources = sorted(s for s in (sdk / "src").rglob("*") if s.suffix in (".c", ".C", ".cpp"))
    if not sources:
        parser.error(f"no SDK sources in {sdk / 'src'}")
    repo = port.parent
    commit = output(["git", "-C", repo, "rev-parse", "HEAD"])
    environment = os.environ.copy()
    environment.setdefault("SOURCE_DATE_EPOCH", output(["git", "-C", repo, "show", "-s", "--format=%ct", "HEAD"]))
    args.output.mkdir(parents=True, exist_ok=True)
    sources_by_member = {s.name + ".o": s for s in sources}
    if len(sources_by_member) != len(sources):
        parser.error("ambiguous SDK archive member names")
    fixed = ["-target", "x86_64-windows-gnu", "-O2", "-gdwarf-4", "-fdebug-compilation-dir=.",
             "-DUSE_EXTENDED_PRIM_POINTERS=0", "-DSDL_MAIN_HANDLED",
             "-Wno-c++11-narrowing", "-Wno-register", "-Wno-format-security",
             "-ffunction-sections", "-fdata-sections"]
    prefix_maps = [(repo, "."), (sdk, "pc_port/extern/PsyCross"),
                   (sdl, "SDL/include"), (al, "OpenAL/include")]
    # Zig's intrinsic and MinGW system headers also appear in DWARF.
    zig_env = json.loads(output([args.zig, "env"]))
    prefix_maps.append((Path(zig_env["lib_dir"]).resolve(), "zig/lib"))
    for old, new in prefix_maps:
        fixed += [f"-ffile-prefix-map={old}={new}"]
    includes = [sdk / "include", sdk / "include/psx", port / "include",
                port / "include_shim", sdk / "src", sdl, al]
    for path in includes:
        fixed += ["-I", str(path)]
    with tempfile.TemporaryDirectory(dir=args.output) as temporary:
        work = Path(temporary).resolve()

        def compile_unit(member_and_source):
            member, source = member_and_source
            cpp = source.suffix in (".C", ".cpp")
            language = ["c++", "-x", "c++", "-std=gnu++17", "-fno-exceptions"] if cpp else [
                "cc", "-x", "c", "-std=gnu11"]
            # LIBAPI calls the guest Psy-Q entrypoint. PsyX_main uses the
            # homonymous KERNEL32 critical-section function through its DLL import.
            if source.relative_to(sdk).as_posix() == "src/psx/LIBAPI.C":
                language.append("-DEnterCriticalSection=XenoPsyQEnterCriticalSection")
            subprocess.run([args.zig, *language, *fixed, "-c", source, "-o", work / member],
                           check=True, env=environment)
            return {"source": source.relative_to(sdk).as_posix(), "member": member,
                    "source_sha256": sha256(source), "object_sha256": sha256(work / member)}

        with ThreadPoolExecutor(max_workers=args.jobs) as executor:
            units = list(executor.map(compile_unit, sorted(sources_by_member.items())))
        archive = work / "libpsycross.a"
        subprocess.run([args.zig, "ar", "rcs", archive, *(work / u["member"] for u in units)], check=True)
        receipt = {"source_commit": commit, "target": "x86_64-windows-gnu",
                   "compiler": output([args.zig, "version"]),
                   "source_date_epoch": environment["SOURCE_DATE_EPOCH"],
                   "archive_sha256": sha256(archive), "license_sha256": sha256(sdk / "LICENSE"),
                   "units": units}
        (work / "sdk-receipt.json").write_text(json.dumps(receipt, indent=2) + "\n")
        archive.replace(args.output / archive.name)
        (work / "sdk-receipt.json").replace(args.output / "sdk-receipt.json")
        print(f"{len(units)} AMD64 COFF objects, libpsycross.a SHA-256 {receipt['archive_sha256']}")


if __name__ == "__main__":
    main()
