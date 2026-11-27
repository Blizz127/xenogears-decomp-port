"""Build the Linux native port from one checkout, local renderer and local disc split."""
import argparse
import concurrent.futures
import hashlib
import functools
import json
import os
from pathlib import Path
import re
import shlex
import shutil
import subprocess
import sys
import tempfile

RENDERER_LIST_SHA256 = "3260a25aaf91161074e190e14639fc51239942d8184a7f83101403df18ba90e3"
RENDERER_SOURCE_FILES = {
    "pc_port/extern/PsyCross/include/psx/" + name: "pc_port/renderer_headers/psx/" + name
    for name in ("libgpu.h", "libspu.h", "libcd.h")
}
GENERATED_INPUTS = {"battle_bridge_map.inc", "stubs.c"}

def sha256(path):
    with path.open("rb") as fp:
        return hashlib.file_digest(fp, "sha256").hexdigest()

def renderer_input(repo, tracked, key, donor_path, donor_digest):
    donor = donor_path.read_bytes()
    if hashlib.sha256(donor).hexdigest() != donor_digest:
        raise ValueError(f"restored renderer changed during preparation: {key}")
    selected = RENDERER_SOURCE_FILES.get(key)
    if selected is None:
        if key.startswith("pc_port/extern/PsyCross/"):
            return donor
        if not key.startswith(("pc_port/src/", "pc_port/include_shim/")):
            raise ValueError(f"unexpected renderer companion: {key}")
        selected = key
    path = repo / selected
    if path.resolve(strict=True) != path:
        raise ValueError(f"indirect project renderer input: {selected}")
    data = path.read_bytes()
    blob = hashlib.sha1(b"blob " + str(len(data)).encode() + b"\0" + data).hexdigest()
    if tracked.get(selected) != blob:
        raise ValueError(f"uncommitted project renderer input: {selected}")
    return data

def renderer_package_proof(records, objdir, archive, binary, commit, ar):
    members = [Path(item).name for item in records]
    measured_members = subprocess.check_output([ar, "t", archive], text=True).splitlines()
    if len(set(members)) != len(members) or members != measured_members:
        raise ValueError("Linux renderer archive does not match compiler member order")
    inputs, units, hashes = {}, {}, {}
    for unit, member in zip(records, members):
        obj = objdir / unit
        compiled = json.loads(Path(str(obj) + ".inputs.json").read_bytes())
        content = subprocess.check_output([ar, "p", archive, member])
        digest = hashlib.sha256(content).hexdigest()
        if compiled["object"] != unit or digest != compiled["post_compile_sha256"] or digest != sha256(obj):
            raise ValueError(f"Linux renderer member differs from compiler output: {member}")
        dependencies = []
        for key, value in compiled["inputs"].items():
            group, rel = key.split("/", 1)
            if group == "toolchain":
                continue
            if group == "renderer":
                rel = "pc_port/extern/PsyCross/" + rel
            elif group not in ("project", "renderer-companion"):
                raise ValueError(f"unexpected renderer dependency {key}")
            if inputs.setdefault(rel, value) != value:
                raise ValueError(f"inconsistent renderer compiler source {rel}")
            dependencies.append(rel)
        if not dependencies:
            raise ValueError(f"renderer member has no measured compiler source: {member}")
        units[member] = sorted(set(dependencies))
        hashes[member] = digest
    if not RENDERER_SOURCE_FILES.keys() <= inputs.keys():
        raise ValueError("renderer compile evidence omits a project header")
    return dict(platform="linux", source_commit=commit, binary_sha256=sha256(binary),
                sdk_sha256=sha256(archive), compiler_inputs=inputs, source_files=RENDERER_SOURCE_FILES,
                members=members,
                member_sha256=hashes, units=units)

def build(args):
    repo = args.source.resolve()
    renderer = args.renderer.resolve()
    out = args.out.resolve()
    if renderer.parent.name!="extern" or renderer.parent.parent.name!="pc_port":
        raise ValueError("use the pc_port/extern/PsyCross directory produced by prepare_renderer.py")
    renderer_support = renderer.parents[2]
    spec = json.loads(Path(__file__).with_name("linux-build.json").read_text())
    generated = out / "generated"
    objects = out / "objects"
    paths = {"@source@":repo, "@renderer@":renderer, "@generated@":generated, "@obj@":objects}
    def expand(word):
        for token, base in paths.items():
            word = word.replace(token, str(base))
        return word
    renderer_files = {}
    for name in spec["renderer_sources"]:
        path = (renderer_support / name).resolve(strict=True)
        if not path.is_relative_to(renderer_support) or path in renderer_files:
            raise ValueError(f"invalid renderer input {name}")
        renderer_files[path] = sha256(path)
    manifest = "".join(
        f"{digest}  {path.relative_to(renderer_support).as_posix()}\n"
        for path, digest in sorted(renderer_files.items(), key=lambda row: row[0].as_posix())
    )
    if hashlib.sha256(manifest.encode()).hexdigest() != RENDERER_LIST_SHA256:
        raise ValueError("restored Linux renderer sources differ from the published input hashes")
    selection = subprocess.check_output(["git","--no-replace-objects","-C",repo,"rev-parse","HEAD"],text=True).strip()
    committed = subprocess.check_output(["git","--no-replace-objects","-C",repo,"ls-tree","-rz",selection])
    tracked={}
    for record in committed.split(b"\0"):
        if record:
            info,path=record.split(b"\t",1)
            mode,kind,oid=info.split()
            if kind==b"blob" and mode in (b"100644", b"100755"):
                tracked[os.fsdecode(path)]=oid.decode()
    def prepare_effective_renderer():
        # Vendor sources include project headers by relative path. Give each build
        # its own sibling layout so these includes use the selected Git revision.
        nonlocal renderer, renderer_support, renderer_files
        out.mkdir(parents=True, exist_ok=True)
        upstream_root = renderer_support
        selected = {}
        with tempfile.TemporaryDirectory(prefix=".renderer-", dir=out) as scratch:
            staging = Path(scratch) / "inputs"
            for path, expected in renderer_files.items():
                rel = path.relative_to(upstream_root)
                key = rel.as_posix()
                data = renderer_input(repo, tracked, key, path, expected)
                selected[rel] = hashlib.sha256(data).hexdigest()
                dest = staging / rel
                dest.parent.mkdir(parents=True, exist_ok=True)
                dest.write_bytes(data)
            manifest = "".join(f"{value}  {rel.as_posix()}\n" for rel, value in sorted(selected.items()))
            target = out / ("renderer-inputs-" + hashlib.sha256(manifest.encode()).hexdigest())
            try:
                staging.rename(target)
            except OSError:
                # Concurrent focused builds may have prepared this same input set.
                if not target.is_dir() or target.is_symlink():
                    raise
                actual = {p.relative_to(target): sha256(p) for p in target.rglob("*") if p.is_file()}
                if actual != selected:
                    raise ValueError("existing renderer input directory differs from the selected sources")
        renderer_support = target
        renderer = target / "pc_port/extern/PsyCross"
        renderer_files = {target / rel: value for rel, value in selected.items()}
        paths["@renderer@"] = renderer
    @functools.lru_cache(maxsize=None)
    def input_identity(path, size, mtime):
        data=path.read_bytes()
        if len(data)!=size:
            raise ValueError(f"build input changed size: {path}")
        digest=hashlib.sha256(data).hexdigest()
        for root,group in ((generated,"generated"),(renderer,"renderer"),
                           (renderer_support,"renderer-companion"),(repo,"project")):
            if path.is_relative_to(root):
                key=path.relative_to(root).as_posix()
                if group == "generated" and key not in GENERATED_INPUTS:
                    raise ValueError(f"unapproved generated compiler input {key}")
                if group in ("renderer", "renderer-companion") and renderer_files.get(path) != digest:
                    raise ValueError(f"unverified restored compiler input {key}")
                if group=="renderer-companion" and not key.startswith(("pc_port/src/","pc_port/include_shim/")):
                    raise ValueError(f"unknown renderer companion dependency {key}")
                if group in ("project","renderer-companion"):
                    git_id=hashlib.sha1(b"blob "+str(size).encode()+b"\0"+data).hexdigest()
                    if tracked.get(key)!=git_id:
                        raise ValueError(f"uncommitted compiler input {key}")
                return group+"/"+key,digest
        if path.is_relative_to(Path("/usr")) or path.is_relative_to(Path("/lib")):
            return "toolchain"+str(path),digest
        raise ValueError(f"compiler input outside source, renderer or toolchain: {path}")
    def pin_input(path):
        path=path.resolve()
        stat=path.stat()
        return input_identity(path,stat.st_size,stat.st_mtime_ns)
    def source(unit):
        rel = spec["units"][unit]["source"]
        return repo / expand(rel)
    def compile_one(unit):
        item = spec["units"][unit]
        compiler, flags = spec["profiles"][item["profile"]]
        exe = args.cxx if compiler=="c++" else args.cc
        obj = objects / unit
        obj.parent.mkdir(parents=True, exist_ok=True)
        # The prefix maps also cover externally prepared renderer and split directories.
        prefix = ["-ffile-prefix-map="+str(renderer_support)+"=.",
                  "-ffile-prefix-map="+str(renderer)+"=./pc_port/extern/PsyCross",
                  "-ffile-prefix-map="+str(out)+"=./build"]
        depfile=Path(str(obj)+".d")
        command=[exe,*map(expand,flags),*prefix,"-c",source(unit),"-o",obj,"-MD","-MF",depfile]
        subprocess.run(command,cwd=repo,check=True)
        dependency_text=depfile.read_text().replace(chr(92)+chr(10)," ")
        _,separator,inputs=dependency_text.partition(":")
        if not separator:
            raise ValueError(f"malformed dependency file {depfile}")
        files=dict(pin_input(repo/path) for path in shlex.split(inputs))
        receipt=dict(object=unit,compiler=str(Path(shutil.which(exe)).resolve()),
                     arguments=list(map(str,command)),direct_input=spec["units"][unit]["source"],
                     inputs=files,post_compile_sha256=sha256(obj))
        Path(str(obj)+".inputs.json").write_text(json.dumps(receipt,indent=2)+"\n")
        return unit
    def transform(unit):
        obj = objects / unit
        for profile in spec["units"][unit]["transforms"]:
            subprocess.run([args.objcopy,*spec["transform_profiles"][profile],obj],cwd=repo,check=True)
        return unit
    if args.unit:
        if args.unit not in spec["units"]:
            raise ValueError(f"unknown manifest unit {args.unit}")
        prepare_effective_renderer()
        compile_one(args.unit)
        transform(args.unit)
        print("Compiled",args.unit,sha256(objects/args.unit))
        return
    inputs = sorted({str(source(n).relative_to(repo)) if source(n).is_relative_to(repo)
                     else spec["units"][n]["source"] for n in spec["units"]
                     if n!="stubs.o" and not source(n).is_file()})
    if args.check_inputs:
        print(json.dumps(dict(objects=len(spec["units"]),unavailable_source_files=inputs),indent=2))
        return 1 if inputs else 0
    if inputs:
        raise ValueError(f"{len(inputs)} manifest source files unavailable; see --check-inputs")
    if not args.split:
        raise ValueError("supply the directory containing your local Disc 1 split via --split")
    if out.exists() and any(out.iterdir()):
        raise ValueError("full builds require an empty output directory")
    generated.mkdir(parents=True,exist_ok=True)
    objects.mkdir(parents=True,exist_ok=True)
    prepare_effective_renderer()
    generator = repo/"pc_port/tools/host_abi/generate.py"
    split = args.split.resolve()
    # Keep classic locale for the native linker's undefined-name diagnostics.
    env = dict(os.environ, LC_ALL="C")
    def bridge(phase):
        subprocess.run([sys.executable,generator,"bridge","--phase",phase,
                        "--main-symbols",repo/"config/symbol_addrs.slus_006.64.txt",
                        "--battle-functions",split/"linker/undefined_funcs_auto.battle.txt",
                        "--battle-symbols",split/"linker/undefined_syms_auto.battle.txt",
                        "--out",generated/"battle_bridge_map.inc"],check=True,cwd=repo)
    def link(name,trial=False):
        obj_args = [expand(i) for i in spec["link"] if not trial or i!="@obj@/stubs.o"]
        result=subprocess.run([args.cc,*obj_args,"-Wl,-Map="+str(out/(name+".map")),"-o",out/name],
                              cwd=repo,env=env,stderr=subprocess.PIPE,text=True)
        (out/(name+".link.log")).write_text(result.stderr)
        if not trial:
            result.check_returncode()
        return result
    def git(*command):
        return subprocess.check_output(["git","--no-replace-objects","-C",repo,*command],text=True).strip()
    if git("status","--porcelain","--untracked-files=no"):
        raise ValueError("full builds require committed source")
    commit,tree=selection,git("rev-parse",selection+"^{tree}")
    bridge("cold")
    with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as pool:
        list(pool.map(compile_one,(n for n in spec["units"] if n!="stubs.o")))
    order=[i.split("/",1)[1] for i in spec["link"] if i.endswith(".o")]
    battle=[n for n in order if n.startswith("battle_host_")]
    matching=[n for n in order if n.startswith("src_")]
    other=[n for n in order if n not in battle and n not in ("stubs.o","port_main.o")]
    def nm(names,flags):
        return subprocess.check_output(["nm",*flags,*names],cwd=objects,text=True).splitlines()
    def write_lines(name,records):
        (generated/name).write_text("".join(line+"\n" for line in records))
    def bindings(names,allowed=None):
        result=set()
        for line in nm(names,["-g","--defined-only"]):
            fields=line.split()
            if len(fields)==3 and (allowed is None or fields[1] in allowed):
                result.add(fields[2])
        return sorted(result)
    namespaces=[]
    for name in battle:
        renames=[]
        for group in spec["units"][name]["transforms"]:
            flags=spec["transform_profiles"][group]
            if len(flags)%2 or any(flag!="--redefine-sym" for flag in flags[::2]):
                raise ValueError("unexpected battle object transformation")
            renames.extend(flags[1::2])
        namespaces.append(tuple(renames))
    if not namespaces or len(set(namespaces))!=1:
        raise ValueError("battle translation units require a common symbol namespace")
    entries=[pair.split("=",1) for pair in namespaces[0]]
    if any(len(pair)!=2 or not all(value.isidentifier() for value in pair) for pair in entries):
        raise ValueError("invalid battle namespace entry")
    write_lines("battle-namespace.tsv",["\t".join(pair) for pair in entries])
    write_lines("battle_defined.txt",bindings(battle,{"T","D","B","W"}))
    write_lines("port_defined.txt",bindings(other))
    write_lines("matching-before.nm",nm(matching,["-A","-P","-g","--defined-only"]))
    for name in order:
        if name!="stubs.o":
            transform(name)
    write_lines("matching-after.nm",nm(matching,["-A","-P","-g","--defined-only"]))
    write_lines("battle-undefined.nm",nm(battle,["-A","-P","-u"]))
    subprocess.run([args.ar,"rcs",objects/"libpsycross.a",*(objects/n for n in spec["renderer_order"])],check=True)
    result=link("trial",True)
    if result.returncode==0:
        raise ValueError("trial link unexpectedly succeeded before host storage generation")
    undefined=sorted(set(re.findall(r"undefined reference to [`']([A-Za-z_0-9]+)'",result.stderr)))
    missing=generated/"undef.txt"
    missing.write_text("".join(n+"\n" for n in undefined))
    subprocess.run([sys.executable,generator,"stubs","--permit-unused-storage","--undefined",missing,"--out",generated/"stubs.c"],
                   cwd=repo,check=True)
    bridge("settled")
    compile_one("stubs.o")
    compile_one("battle_mips_runtime.c.o")
    link("xeno-port")
    # Keep the unstripped binary and link map locally for binary origin auditing.
    subprocess.run([args.strip,"--strip-debug","-o",out/"xeno-port.release",out/"xeno-port"],check=True)
    def artifact(path):
        return dict(path=str(path),bytes=path.stat().st_size,sha256=sha256(path))
    if git("rev-parse","HEAD")!=commit or git("status","--porcelain","--untracked-files=no"):
        raise ValueError("source checkout changed during the build")
    proof=dict(source=commit,tree=tree,elf=artifact(out/"xeno-port"),
               release=artifact(out/"xeno-port.release"),bridge=artifact(generated/"battle_bridge_map.inc"),
               game_order=order,wraps=[a.removeprefix("-Wl,--wrap=") for a in spec["link"] if a.startswith("-Wl,--wrap=")],
               generated={p.name:artifact(p) for p in sorted(generated.iterdir()) if p.is_file()},
               objects={n:dict(file=artifact(objects/n),
                               build=artifact(Path(str(objects/n)+".inputs.json")),
                               transforms=[spec["transform_profiles"][i] for i in item["transforms"]])
                        for n,item in spec["units"].items()},
               renderer_archive=artifact(objects/"libpsycross.a"))
    renderer_proof = out / "linux-renderer-inputs.json"
    coverage = renderer_package_proof(spec["renderer_order"], objects, objects/"libpsycross.a",
                                      out/"xeno-port.release", commit, args.ar)
    coverage["source_tree"] = tree
    renderer_proof.write_text(json.dumps(coverage, indent=2) + "\n")
    proof.update(binary=proof["release"], sdk=proof["renderer_archive"], renderer_inputs=artifact(renderer_proof))
    (out/"linux-link.json").write_text(json.dumps(proof,indent=2)+"\n")
    print("Linked xeno-port.release",proof["release"]["sha256"])

if __name__=="__main__":
    p=argparse.ArgumentParser(description=__doc__)
    for name in ("source","renderer","out"):
        p.add_argument("--"+name,type=Path,required=True)
    p.add_argument("--split",type=Path)
    p.add_argument("--jobs",type=int,choices=range(1,5),default=1)
    group=p.add_mutually_exclusive_group()
    group.add_argument("--unit")
    group.add_argument("--check-inputs",action="store_true")
    for name,tool in (("cc","gcc"),("cxx","g++"),("ar","ar"),("objcopy","objcopy"),("strip","strip")):
        p.add_argument("--"+name,default=tool)
    sys.exit(build(p.parse_args()))
