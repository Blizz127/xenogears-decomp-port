#!/usr/bin/env python3
"""Compile the Windows renderer from the restored public sources and the game checkout."""

import argparse
import concurrent.futures
import hashlib
import json
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import sys

from forward_compile import (
    RENDERER_HEADER_SOURCES, VENDOR, git, repo_info, require, source_blob, verify_renderer_donor,
)


HERE = Path(__file__).resolve().parent
SYSTEM_ROOTS = ("include", "libc/include/any-windows-any", "libcxx/include")
IGNORED_ENV = {
    "CPATH",
    "C_INCLUDE_PATH",
    "CPLUS_INCLUDE_PATH",
    "OBJC_INCLUDE_PATH",
    "LIBRARY_PATH",
    "DEPENDENCIES_OUTPUT",
    "SUNPRO_DEPENDENCIES",
    "CCC_OVERRIDE_OPTIONS",
    "SDKROOT",
    "CFLAGS",
    "CXXFLAGS",
}


def digest(path):
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def fingerprint(path):
    return dict(path=str(path), bytes=path.stat().st_size, sha256=digest(path))


def read_profile():
    profile = json.loads((HERE / "renderer-profile.json").read_bytes())
    restored = profile["restored_inputs"]
    verify_renderer_donor(profile)
    members = {Path(path).name + ".o": path for path in profile["sources"]}
    require(
        len(members) == len(profile["sources"]) == len(profile["unit_inputs"]) == 22
        and list(members) == sorted(members)
        and set(members) == set(profile["unit_inputs"]),
        "The member roster differs from the Windows renderer profile",
    )
    available = set(restored) | set(profile["external_headers"])
    measured = {key for keys in profile["unit_inputs"].values() for key in keys}
    require(
        available == measured
        and len(restored) == 74
        and len(profile["external_headers"]) == 54,
        "Declared inputs differ from the measured Windows renderer profile",
    )
    return profile, members


def stage_inputs(root, restored, prefix, stage, blobs, profile):
    """Preserve the renderer's relative includes into the project's src directory."""
    effective = {}
    for key, expected in profile["restored_inputs"].items():
        source = restored / key
        require(source.resolve(strict=True) == source, "Unexpected restored symlink: " + key)
        restored_data = source.read_bytes()
        require(
            hashlib.sha256(restored_data).hexdigest() == expected,
            "Wrong restored compiler input: " + key,
        )
        if key in RENDERER_HEADER_SOURCES:
            data = source_blob(root / RENDERER_HEADER_SOURCES[key], root, blobs)
        elif key.startswith(VENDOR):
            data = restored_data
        else:
            data = source_blob(root / key, root, blobs)
        target = stage / key
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(data)
        effective[key] = hashlib.sha256(data).hexdigest()
    for key, expected in profile["external_headers"].items():
        path = prefix / "include" / key
        require(
            path.resolve(strict=True) == path and digest(path) == expected,
            "Wrong Windows SDL/OpenAL header: " + key,
        )
    return effective


def compiler_environment(out, commit, root):
    env = {
        k: v
        for k, v in os.environ.items()
        if k not in IGNORED_ENV and not k.startswith("ZIG_")
    }
    env.update(
        LC_ALL="C",
        SOURCE_DATE_EPOCH=git(root, "show", "-s", "--format=%ct", commit)
        .decode()
        .strip(),
        ZIG_LOCAL_CACHE_DIR=str(out / "zig-local-cache"),
        ZIG_GLOBAL_CACHE_DIR=str(out / "zig-global-cache"),
    )
    return env


def compile_renderer(args):
    root = args.source_root.resolve(strict=True)
    tree, blobs = repo_info(root, args.commit)
    profile, members = read_profile()
    require(1 <= args.jobs <= 4, "Choose one to four compiler jobs")
    require(not args.unit or args.unit in members, "Unknown Windows renderer object")
    if not args.unit:
        for filename in (
            "build_renderer.py",
            "renderer-profile.json",
            "forward_compile.py",
        ):
            path = root / "pc_port/tools/win64" / filename
            require(
                source_blob(path, root, blobs) == (HERE / filename).read_bytes(),
                "Full build recipe differs from the selected commit: " + filename,
            )
    restored = args.restored.resolve(strict=True)
    prefix = args.prefix.resolve(strict=True)
    zig_path = shutil.which(args.zig)
    require(zig_path, "Zig executable unavailable")
    zig = Path(zig_path).resolve(strict=True)
    require(
        digest(zig) == profile["zig_executable_sha256"],
        "Zig executable differs from the pinned Linux x64 toolchain",
    )
    out = args.out.resolve()
    out.mkdir()
    for name in ("objects", "raw", "deps", "logs"):
        (out / name).mkdir()
    env = compiler_environment(out, args.commit, root)

    def output(*cmd):
        return subprocess.check_output(cmd, cwd=out, env=env, text=True).strip()

    objcopy = shutil.which("llvm-objcopy-21")
    require(
        objcopy and "LLVM version 21." in output(objcopy, "--version"),
        "LLVM objcopy 21 is required",
    )
    require(output(zig, "version") == profile["zig"] == "0.13.0", "Expected Zig 0.13.0")
    zig_env = json.loads(output(zig, "env"))
    require(
        Path(zig_env["zig_exe"]).resolve(strict=True) == zig,
        "Toolchain executable identity changed",
    )
    zig_lib = Path(zig_env["lib_dir"]).resolve(strict=True)
    stage = out / "inputs"
    effective = stage_inputs(root, restored, prefix, stage, blobs, profile)
    sdk = stage / VENDOR
    port = stage / "pc_port"
    external = prefix / "include"
    sdl = external / "SDL2"
    maps = (
        (stage, "."),
        (sdk, "pc_port/extern/PsyCross"),
        (sdl, "SDL/include"),
        (external, "OpenAL/include"),
        (zig_lib, "zig/lib"),
    )
    flags = [
        "-target",
        "x86_64-windows-gnu",
        "-O2",
        "-gdwarf-4",
        "-fdebug-compilation-dir=.",
        "-DUSE_EXTENDED_PRIM_POINTERS=0",
        "-DSDL_MAIN_HANDLED",
        "-Wno-c++11-narrowing",
        "-Wno-register",
        "-Wno-format-security",
        "-ffunction-sections",
        "-fdata-sections",
        *(f"-ffile-prefix-map={path}={public}" for path, public in maps),
    ]
    for directory in (
        sdk / "include",
        sdk / "include/psx",
        port / "include",
        port / "include_shim",
        sdk / "src",
        sdl,
        external,
    ):
        flags.extend(["-I", str(directory)])

    def compile_member(member):
        rel = members[member]
        source = sdk / rel
        obj = out / "objects" / member
        raw = out / "raw" / member
        dep = out / "deps" / (member + ".d")
        log = out / "logs" / (member + ".log")
        if source.suffix in (".cpp", ".C"):
            language = ["c++", "-x", "c++", "-std=gnu++17", "-fno-exceptions"]
        else:
            language = ["cc", "-x", "c", "-std=gnu11"]
        if rel == "src/psx/LIBAPI.C":
            language.append("-DEnterCriticalSection=XenoPsyQEnterCriticalSection")
        cmd = [
            str(zig),
            *language,
            *flags,
            "-MD",
            "-MF",
            str(dep),
            "-MT",
            "unit",
            "-c",
            str(source),
            "-o",
            str(raw),
        ]
        with log.open("xb") as stream:
            run = subprocess.run(
                cmd, cwd=stage, env=env, stdout=stream, stderr=subprocess.STDOUT
            )
        require(
            run.returncode == 0,
            "Failed renderer compile: " + member + " (see " + str(log) + ")",
        )
        # Zig writes host paths and full compiler arguments into CodeView even
        # for a DWARF build. Keep the DWARF and the code in the archive.
        normalize = [
            objcopy,
            "--remove-section=.debug$S",
            "--remove-section=.debug$T",
            "--remove-section=.debug$H",
            str(raw),
            str(obj),
        ]
        subprocess.run(normalize, cwd=stage, env=env, check=True)
        with obj.open("rb") as stream:
            require(
                stream.read(2) == b"\x64\x86", "Expected native AMD64 COFF: " + member
            )
        deptext = dep.read_text().replace("\\\n", " ")
        target, sep, inputs = deptext.partition(":")
        require(
            target == "unit" and sep, "Unexpected renderer compiler dependency receipt"
        )
        measured = {}
        external_inputs = {}
        system = {}
        for word in shlex.split(inputs):
            path = (stage / word).resolve(strict=True)
            if path.is_relative_to(stage):
                key = path.relative_to(stage).as_posix()
                require(
                    key in effective and digest(path) == effective[key],
                    "Unexpected renderer input: " + key,
                )
                measured[key] = effective[key]
            elif path.is_relative_to(external):
                key = path.relative_to(external).as_posix()
                value = digest(path)
                require(
                    profile["external_headers"].get(key) == value,
                    "Unexpected multimedia input: " + key,
                )
                external_inputs[key] = value
            elif path.is_relative_to(zig_lib):
                key = path.relative_to(zig_lib)
                require(
                    any(key.is_relative_to(p) for p in SYSTEM_ROOTS),
                    "Unknown toolchain header: " + str(key),
                )
                system[key.as_posix()] = digest(path)
            else:
                raise ValueError(
                    "Renderer compiler read an undeclared input: " + str(path)
                )
        require(
            VENDOR + rel in measured
            and set(measured) | set(external_inputs)
            == set(profile["unit_inputs"][member]),
            "Measured renderer inputs differ from the public profile: " + member,
        )
        return dict(
            member=member,
            source=rel,
            source_sha256=effective[VENDOR + rel],
            object_sha256=digest(obj),
            raw_object_sha256=digest(raw),
            normalize_command=normalize,
            compiler_inputs=sorted(measured),
            external_inputs=external_inputs,
            system_headers=system,
            depfile=fingerprint(dep),
            command=cmd,
            cwd=str(stage),
        )

    selected = [args.unit] if args.unit else list(members)
    with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as pool:
        units = list(pool.map(compile_member, selected))
    require(
        git(root, "rev-parse", "HEAD").decode().strip() == args.commit,
        "Checkout changed during compilation",
    )
    union = {k: effective[k] for unit in units for k in unit["compiler_inputs"]}
    receipt = dict(
        schema=1,
        mode="subset" if args.unit else "full",
        source_commit=args.commit,
        source_tree=tree,
        target=profile["target"],
        compiler=profile["zig"],
        toolchain=fingerprint(zig),
        units=units,
        compiler_inputs=union,
        source_files={key: path for key, path in RENDERER_HEADER_SOURCES.items() if key in union},
        source_date_epoch=env["SOURCE_DATE_EPOCH"],
        profile=fingerprint(HERE / "renderer-profile.json"),
    )
    if not args.unit:
        require(
            union == effective,
            "Full renderer compilation omits an authorized source input",
        )
        archive = out / "libpsycross.a"
        command = [
            str(zig),
            "ar",
            "rcs",
            str(archive),
            *[str(out / "objects" / name) for name in selected],
        ]
        subprocess.run(command, cwd=stage, env=env, check=True)
        require(
            output(zig, "ar", "t", archive).splitlines() == selected,
            "Renderer archive member order changed",
        )
        for unit in units:
            data = subprocess.check_output(
                [zig, "ar", "p", archive, unit["member"]], cwd=stage, env=env
            )
            require(
                hashlib.sha256(data).hexdigest() == unit["object_sha256"],
                "Archived object differs: " + unit["member"],
            )
        receipt.update(archive_sha256=digest(archive), archive_command=command)
    destination = out / ("sdk-subset.json" if args.unit else "sdk-receipt.json")
    destination.write_text(json.dumps(receipt, indent=2) + "\n")
    print(destination, digest(destination))


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    for flag in ("source-root", "restored", "prefix", "out"):
        parser.add_argument("--" + flag, required=True, type=Path)
    parser.add_argument("--commit", required=True)
    parser.add_argument("--zig", default="zig")
    parser.add_argument("--jobs", type=int, default=1)
    parser.add_argument(
        "--unit", help="Compile only this .o member and write a subset receipt"
    )
    try:
        compile_renderer(parser.parse_args())
    except (ValueError, OSError, KeyError, subprocess.CalledProcessError) as exc:
        parser.exit(1, "build_renderer: " + str(exc) + "\n")
