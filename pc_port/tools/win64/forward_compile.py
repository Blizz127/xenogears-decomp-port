#!/usr/bin/env python3
"""Cross-compile the selected public game sources into fresh AMD64 COFF objects."""

import argparse
import concurrent.futures
import hashlib
import json
import os
from pathlib import Path
import re
import shlex
import shutil
import subprocess
import sys

sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent
VENDOR = "pc_port/extern/PsyCross/"
RENDERER_HEADER_SOURCES = {
    VENDOR + "include/psx/" + name: "pc_port/renderer_headers/psx/" + name
    for name in ("libcd.h", "libgpu.h", "libspu.h")
}
DONOR_MANIFEST_SHA256 = "84103d76d9f7e7b8052125d3d5575597b649f13320de3ca333a1ccce2428e018"


def require(ok, reason):
    if not ok:
        raise ValueError(reason)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def verify_renderer_donor(profile):
    restored = profile["restored_inputs"]
    manifest = "".join(f"{value}  {key}\n" for key, value in sorted(restored.items()))
    require(sha(manifest.encode()) == profile["restored_manifest_sha256"] == DONOR_MANIFEST_SHA256,
            "Restored file list differs from the published Windows renderer manifest")


def pin(path, relative=None):
    data = path.read_bytes()
    return {
        "path": str(path.relative_to(relative) if relative is not None else path),
        "bytes": len(data),
        "sha256": sha(data),
    }


def git(root, *args):
    return subprocess.check_output(
        ["git", "--no-replace-objects", "-C", str(root), *args]
    )


def repo_info(root, commit):
    require(re.fullmatch(r"[a-f0-9]{40}", commit), "Expected a full source commit")
    require(
        git(root, "rev-parse", "HEAD").decode().strip() == commit,
        "Check out the selected build commit",
    )
    tree = git(root, "rev-parse", commit + "^{tree}").decode().strip()
    rows = git(root, "ls-tree", "-rz", commit).split(b"\0")
    blobs = {}
    for row in rows:
        if row:
            meta, path = row.split(b"\t", 1)
            mode, kind, digest = meta.split()
            if kind == b"blob" and mode in (b"100644", b"100755"):
                blobs[path.decode()] = digest.decode()
    return tree, blobs


def source_blob(path, root, blobs):
    relative = path.relative_to(root).as_posix()
    require(
        relative in blobs and path.is_file() and path.resolve() == path,
        "Missing committed source " + relative,
    )
    data = path.read_bytes()
    git_oid = hashlib.sha1(
        b"blob " + str(len(data)).encode() + b"\0" + data
    ).hexdigest()
    require(
        git_oid == blobs[relative], "Source differs from selected commit: " + relative
    )
    return data


def read_profile():
    profile = json.loads((HERE / "compile-profile.json").read_bytes())
    units = profile["units"]
    groups = profile["flags"]
    names = [row[0] for row in units]
    require(
        profile["schema"] == 1
        and profile["target"] == "x86_64-windows-gnu"
        and len(names) == len(set(names))
        and names[-1] == "battle_win32_host.c.o",
        "Unexpected target or object order",
    )
    for obj, path, group in units:
        require(
            Path(obj).name == obj
            and obj.endswith(".o")
            and not Path(path).is_absolute()
            and ".." not in Path(path).parts,
            "Invalid compilation unit " + obj,
        )
        flags = groups[group]
        require(
            flags[:1] == ["--target=x86_64-windows-gnu"]
            and flags[-6:] == ["-MF", "@DEPFILE@", "-c", "@UNIT@", "-o", "@OBJECT@"],
            "Invalid compile flags for " + obj,
        )
    return profile


def game_renderer_headers(renderer, root, blobs):
    expected = {}
    for key, source in RENDERER_HEADER_SOURCES.items():
        staged = renderer / key.removeprefix(VENDOR)
        data = source_blob(root / source, root, blobs)
        require(staged.resolve(strict=True) == staged and staged.read_bytes() == data,
                "Use the renderer stage built from this game commit: " + key)
        expected[key] = sha(data)
    return expected


def matching(text):
    result = {}
    for line in text.splitlines():
        obj, symbol, binding, *_ = line.split()
        key = Path(obj.removesuffix(":")).name, symbol
        require(key not in result, "Duplicate symbol record " + repr(key))
        result[key] = binding
    return result


def windows_headers(proof_path, out, commit, tree, units, groups):
    proof = json.loads(proof_path.read_bytes())
    require(
        proof["source"] == commit and proof["tree"] == tree,
        "Linux build source identity differs",
    )
    order = [row[0] for row in units]
    require(
        proof["game_order"] == order[:-1],
        "Linux/Windows object roster or order differs",
    )

    def contents(key):
        rec = proof["generated"][key]
        path = (proof_path.parent / rec["path"]).resolve(strict=True)
        expected = proof_path.parent / "generated" / key
        require(path == expected, "Expected a local Linux build product: " + key)
        data = path.read_bytes()
        require(
            len(data) == rec["bytes"] and sha(data) == rec["sha256"],
            "Changed generated Linux input: " + key,
        )
        return data

    contents("undef.txt")
    before = matching(contents("matching-before.nm").decode())
    after = matching(contents("matching-after.nm").decode())
    require(
        before.keys() == after.keys(),
        "Linux matching symbols differ across ownership pass",
    )
    weak = {
        obj: [] for obj, _, kind in units if "@BUILD@/@WEAK_HEADER@" in groups[kind]
    }
    for (obj, symbol), binding in before.items():
        change = after[(obj, symbol)]
        if binding != change:
            require(
                (binding, change) == ("T", "W")
                and obj in weak
                and re.fullmatch(r"[a-zA-Z_]\w*", symbol),
                "Unexpected Linux override " + obj + " " + symbol,
            )
            weak[obj].append(symbol)
    for obj, symbols in weak.items():
        (out / (obj + ".h")).write_text(
            "".join(f"#pragma weak {s}\n" for s in sorted(symbols))
        )
    battle = set(contents("battle_defined.txt").decode().splitlines())
    other = set(contents("port_defined.txt").decode().splitlines())
    observed = battle & other
    observed.update(
        re.findall(
            r"\bbattle_(func_[A-F0-9]{8})\b", contents("battle-undefined.nm").decode()
        )
    )
    # Apply the same namespace even if the reduced roster no longer contains
    # the object that originally made a particular symbol name collide.
    aliases = {}
    for line in contents("battle-namespace.tsv").decode("ascii").splitlines():
        old, new = line.split("\t")
        require(
            re.fullmatch(r"[A-Za-z_][A-Za-z_0-9]*", old)
            and new == "battle_" + old
            and old not in aliases,
            "Invalid Linux battle namespace mapping: " + line,
        )
        aliases[old] = new
    require(
        aliases and observed <= aliases.keys(),
        "Linux battle namespace misses an observed symbol",
    )
    (out / "battle_namespace.h").write_text(
        "".join(f"#define {s} {aliases[s]}\n" for s in sorted(aliases))
    )
    for key in ("battle_bridge_map.inc", "stubs.c"):
        (out / key).write_bytes(contents(key))
    return {p.name: pin(p, out) for p in out.iterdir() if p.is_file()}


def compile_one(unit, context):
    obj, rel, group = unit
    p = context["paths"]
    root = p["ROOT"]
    out = p["BUILD"]
    blobs = context["blobs"]
    generated_sources = context["generated_sources"]
    src = generated_sources.get(rel, root / rel)
    original = (
        src.read_bytes() if rel in generated_sources else source_blob(src, root, blobs)
    )
    dst = out / "obj" / obj
    deps = dst.with_suffix(".d")
    values = dict(p, UNIT=src, OBJECT=dst, DEPFILE=deps, WEAK_HEADER=obj + ".h")

    def expand(token):
        return re.sub(r"@([A-Z_]+)@", lambda m: str(values[m[1]]), token)

    args = [context["clang"], *[expand(x) for x in context["flags"][group]]]
    log = out / "logs" / (obj + ".log")
    with log.open("xb") as stream:
        proc = subprocess.run(
            args, cwd=root, env=context["env"], stdout=stream, stderr=subprocess.STDOUT
        )
    require(
        proc.returncode == 0,
        "Compilation failed for " + obj + " (see " + str(log) + ")",
    )
    require(dst.read_bytes()[:2] == b"\x64\x86", "Expected AMD64 COFF: " + obj)
    dep_text = deps.read_text().replace("\\\n", " ")
    _, sep, tail = dep_text.partition(": ")
    require(bool(sep), "Invalid compiler dependency file: " + str(deps))
    deps_by_path = {}
    for token in shlex.split(tail):
        path = (root / token).resolve(strict=True)
        for tag, base in context["origins"]:
            if path.is_relative_to(base):
                use = path.relative_to(base).as_posix()
                data = (
                    source_blob(path, root, blobs)
                    if tag == "source"
                    else path.read_bytes()
                )
                if tag == "renderer" and VENDOR + use in context["renderer_headers"]:
                    require(sha(data) == context["renderer_headers"][VENDOR + use],
                            "Game compiler read a different renderer header: " + use)
                deps_by_path[tag + "/" + use] = sha(data)
                break
        else:
            raise ValueError(
                "Unexpected dependency outside declared roots: " + str(path)
            )
    require(
        original == src.read_bytes(),
        "Translation unit changed during compilation: " + str(src),
    )
    result = pin(dst, out)
    result.update(
        name=obj,
        source=rel,
        source_sha256=sha(original),
        dependencies=deps_by_path,
        compiler_command=args,
        log=str(log.relative_to(out)),
    )
    return result


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("action", choices=["inventory", "compile"])
    ap.add_argument("--source-root", required=True, type=Path)
    ap.add_argument("--commit", required=True)
    ap.add_argument(
        "--renderer-root",
        type=Path,
        help="Effective PsyCross tree restored for Windows",
    )
    ap.add_argument(
        "--prefix",
        type=Path,
        help="Restored Windows SDL/OpenAL include and import prefix",
    )
    ap.add_argument(
        "--zig-lib",
        type=Path,
        help="Zig 0.13.0 lib directory containing libc/include/any-windows-any",
    )
    ap.add_argument("--clang", default="clang-21")
    ap.add_argument(
        "--linux",
        type=Path,
        help="Verified linux-link.json for this same new source revision",
    )
    ap.add_argument(
        "--out", type=Path, help="New private output directory for compilation"
    )
    ap.add_argument("--jobs", type=int, default=1)
    ap.add_argument(
        "--unit",
        action="append",
        default=[],
        help="Limit work to this object, repeatable; produces a subset receipt",
    )
    args = ap.parse_args()
    try:
        root = args.source_root.resolve(strict=True)
        tree, blobs = repo_info(root, args.commit)
        profile = read_profile()
        units = profile["units"]
        flags = profile["flags"]
        wanted = set(args.unit)
        require(wanted <= set(row[0] for row in units), "Unknown object selection")
        selected = [row for row in units if not wanted or row[0] in wanted]
        require(len(selected) > 0, "Empty compilation selection")
        sources = {rel for _, rel, _ in selected}
        required = sources.difference({"generated/stubs.c"})
        missing = sorted(required.difference(blobs))
        require(1 <= args.jobs <= 4, "Choose one to four compiler jobs")
        if args.action == "inventory":
            print(
                json.dumps(
                    dict(
                        source=args.commit,
                        tree=tree,
                        units=len(selected),
                        argument_groups=len(flags),
                        missing=missing,
                    ),
                    indent=2,
                )
            )
            return
        require(
            not missing,
            "Missing "
            + str(len(missing))
            + " source inputs; first: "
            + ", ".join(missing[:6]),
        )
        require(
            args.out and args.zig_lib and args.prefix and args.renderer_root,
            "Compilation needs --out, --zig-lib, --prefix and --renderer-root",
        )
        out = args.out.absolute()
        zig = args.zig_lib.resolve(strict=True)
        renderer = args.renderer_root.resolve(strict=True)
        prefix = args.prefix.resolve(strict=True)
        require(
            (zig / "libc/include/any-windows-any/assert.h").is_file()
            and (renderer / "include/psx/types.h").is_file(),
            "Missing target headers",
        )
        require((prefix / "include").is_dir(), "Missing external include prefix")
        renderer_headers = game_renderer_headers(renderer, root, blobs)
        need_linux = not wanted or any(
            row[0].startswith("battle_host_")
            or row[0] in ("stubs.o", "battle_mips_runtime.c.o", "battle_win32_host.c.o")
            or "@BUILD@/@WEAK_HEADER@" in flags[row[2]]
            for row in selected
        )
        require(
            not need_linux or args.linux,
            "These units require the same-source --linux handoff",
        )
        require(
            not wanted or "battle_win32_host.c.o" not in wanted,
            "Compile the final host resolver as part of a full build",
        )
        clang = shutil.which(args.clang)
        require(clang, "Clang is unavailable")
        release = subprocess.check_output([clang, "--version"], text=True).splitlines()[
            0
        ]
        require(re.search(r"version 21\.", release), "Expected LLVM Clang 21")
        resources = Path(
            subprocess.check_output([clang, "-print-resource-dir"], text=True).strip()
        ).resolve()
        out.mkdir()
        (out / "obj").mkdir()
        (out / "logs").mkdir()
        gen = {}
        if args.linux:
            linux = args.linux.resolve(strict=True)
            gen = windows_headers(linux, out, args.commit, tree, units, flags)
        else:
            linux = None
        generated_sources = {"generated/stubs.c": out / "stubs.c"}
        paths = dict(
            ROOT=root,
            BUILD=out,
            LINUX=linux.parent if linux else out,
            RENDERER=renderer,
            PREFIX=prefix,
            ZIGLIB=zig,
        )
        env = {
            k: v
            for k, v in os.environ.items()
            if k
            not in (
                "CPATH",
                "C_INCLUDE_PATH",
                "CPLUS_INCLUDE_PATH",
                "OBJC_INCLUDE_PATH",
                "DEPENDENCIES_OUTPUT",
                "SUNPRO_DEPENDENCIES",
                "CCC_OVERRIDE_OPTIONS",
                "SDKROOT",
            )
        }
        origins = [
            ("renderer", renderer),
            ("external", prefix),
            ("generated", out),
            ("source", root),
            ("toolchain/zig", zig),
            ("toolchain/clang", resources),
        ]
        ctx = dict(
            paths=paths,
            blobs=blobs,
            flags=flags,
            clang=clang,
            env=env,
            origins=origins,
            generated_sources=generated_sources,
            renderer_headers=renderer_headers,
        )
        selected_main = [row for row in selected if row[0] != "battle_win32_host.c.o"]
        with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as pool:
            rows = list(pool.map(lambda row: compile_one(row, ctx), selected_main))
        if not wanted:
            from forward_binding import derive_bindings

            generated = derive_bindings(
                root, out, out / "obj", [row["name"] for row in rows]
            )
            for artifact in generated:
                gen[artifact.name] = pin(artifact, out)
            rows.append(compile_one(units[-1], ctx))
        require(
            git(root, "rev-parse", "HEAD").decode().strip() == args.commit,
            "Source HEAD changed during build",
        )
        proof = dict(
            schema=1,
            source=args.commit,
            tree=tree,
            mode="subset" if wanted else "full",
            compiler=release,
            profile=pin(HERE / "compile-profile.json"),
            linux=pin(linux) if linux else None,
            generated=gen,
            objects=rows,
        )
        receipt = out / "windows-compile.json"
        receipt.write_text(json.dumps(proof, indent=2) + "\n")
        print(receipt, sha(receipt.read_bytes()))
    except (ValueError, OSError, KeyError, subprocess.CalledProcessError) as exc:
        ap.exit(1, "forward_compile: " + str(exc) + "\n")


if __name__ == "__main__":
    main()
