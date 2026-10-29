#!/usr/bin/env python3
"""Actual active window/text/heap/glyph/SDK upload API versus retail instructions."""
from pathlib import Path
import hashlib,json,os,shutil,subprocess,sys
ROOT=Path(os.environ.get("XENO_TEST_ROOT",Path(__file__).resolve().parents[2])).resolve()
HERE=Path(__file__).resolve().parent
OUT=Path(os.environ["ACTIVE_WINDOW_OUT"]).resolve()
OUT.mkdir()
def sha(p): return hashlib.sha256(p.read_bytes()).hexdigest()
source=Path(os.environ.get("ACTIVE_WINDOW_SOURCE",ROOT/"src/slus_006.64/system/system.c")).resolve()
fixture=Path(os.environ.get("ACTIVE_WINDOW_FIXTURE",HERE/"system_window_binding_test.c")).resolve()
assert sha(ROOT/"disc/SLUS_006.64")=="dc0b2dd786203d4cce5927c5a3fc85a18f39a3f7406078860076ebb0bbae7119"
assert sha(ROOT/"disc/battle.bin")=="1830b4ef1fe37129972fc310dfad534f8161d6c0b123e74254c3711334a3e291"
assert hashlib.sha256((ROOT/"disc/battle.bin").read_bytes()[0x5480:0x5480+156]).hexdigest()=="edc2a3322cee8dd9ff729859c6740c3c03fb02b603fb6a94c06dddc78e4b050a"
assert sha(ROOT/"pc_port/src/battle_mips_adapter.c")=="a87b5c7afaf102cba4ab9903d275df69cc4e9f8ad12142f8f69e379112fcbc67"
assert sha(ROOT/"pc_port/src/battle_mips_adapter.h")=="9924f3f892f0660b5b9c9cb0fb492d9180e451bfa1e727ec825c4f7232425315"
receipt=dict(status="RUNNING",source_pins={},commands=[],returncodes={},
             boundaries="Actual runtime typed dispatch and full 74F70 caller; GR_CopyVRAM transfer model; documented pointer canonicalization. Malformed spans are checked separately. No GPU/game acceptance or exhaustive fault/alias proof; volatile callee CPU registers excluded from API comparison.")
inputs=[source,fixture,Path(__file__).resolve(),ROOT/"pc_port/include_shim/system_window_guard.h",
    ROOT/"pc_port/src/battle_mips_runtime.c",ROOT/"pc_port/src/battle_window_binding_host.inc",ROOT/"pc_port/src/battle_window_render_host.inc",ROOT/"disc/SLUS_006.64",ROOT/"disc/battle.bin"]
inputs += [ROOT/p for p in ["src/slus_006.64/system/memory.c","src/slus_006.64/system/temp2.c",
    "pc_port/src/guest_prim_link.c","pc_port/include_shim/guest_prim_link.h",
    "pc_port/src/battle_mips_adapter.c","pc_port/src/battle_mips_adapter.h",
    "pc_port/extern/PsyCross/src/psx/LIBGPU.C"]]
for p in inputs:
    receipt["source_pins"][str(p)]=sha(p)
    d=OUT/"inputs"/p.relative_to("/");d.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(p,d)
def save(): (OUT/"receipt.json").write_text(json.dumps(receipt,indent=2)+"\n")
def run(label,args,expected=0):
    args=list(map(str,args));receipt["commands"].append(dict(label=label,argv=args));save()
    with (OUT/(label+".log")).open("x") as f:
        r=subprocess.run(args,cwd=ROOT,stdout=f,stderr=subprocess.STDOUT,timeout=60)
    receipt["returncodes"][label]=r.returncode;save()
    if r.returncode!=expected:
        print((OUT/(label+".log")).read_text()[-6000:],flush=True);raise RuntimeError((label,r.returncode,expected))
common=["-I"+str(HERE),"-fno-pie","-fno-builtin","-fno-inline","-ffunction-sections","-fdata-sections","-include","assert.h",
        "-DXENO_PC_PORT","-DXENO_FIELD_OBJECT_OVERLAY","-DSKIP_ASM","-D_LANGUAGE_C","-DUSE_EXTENDED_PRIM_POINTERS=0",
        "-Ipc_port/include_shim","-Iinclude","-Ipc_port/src","-Ipc_port/extern/PsyCross/include","-Ipc_port/extern/PsyCross/include/psx"]
try:
    run("sanitizer-preflight",["bash","-c","source pc_port/tests/lib/sanitizer_preflight.sh; ubsan_preflight gcc clang"])
    run("bridge-map",["python3","tools/scripts/gen_battle_bridge_map.py",
        "--symbols","config/symbol_addrs.slus_006.64.txt","--symbols","linker/undefined_funcs_auto.battle.txt",
        "--symbols","linker/undefined_syms_auto.battle.txt","--symbols","config/symbol_addrs.battle.txt",
        "--out",OUT/"battle_bridge_map.inc"])
    common.append("-I"+str(OUT))
    for mode in os.environ.get("ACTIVE_WINDOW_MODES","O0 O2 UBSan").split():
        flags=["-"+mode] if mode!="UBSan" else ["-O1","-fsanitize=undefined","-fno-sanitize-recover=all"]
        objects=[]
        units=[("system",source,None),("memory",ROOT/"src/slus_006.64/system/memory.c","HeapFree=WindowBody_HeapFree"),
               ("packet",ROOT/"src/slus_006.64/system/temp2.c","func_80031798=WindowBody_LinkPrim"),
               ("domain",ROOT/"pc_port/src/guest_prim_link.c","PcPort_AddPrimDomainAware=WindowBody_AddPrim"),
               ("fixture",fixture,None),("compat",ROOT/"pc_port/src/psyq_compat.c",None),("cpu",ROOT/"pc_port/src/battle_mips_adapter.c",None),
               ("gpu",ROOT/"pc_port/extern/PsyCross/src/psx/LIBGPU.C","LoadImage=WindowBody_LoadImage")]
        for label,path,rename in units:
            obj=OUT/(mode+"."+label+".o");objects.append(obj)
            cpp=label in ("gpu","shim")
            cc="/usr/bin/g++" if cpp else "/usr/bin/clang" if label in ("cpu","fixture","compat") else "/usr/bin/x86_64-linux-gnu-gcc-15"
            warnings=["-Wall","-Wextra","-Werror","-Wno-misleading-indentation"] if label in ("fixture","shim") else ["-fpermissive","-w"] if label!="cpu" else ["-w"]
            sdk_flags=subprocess.check_output(["pkg-config","--cflags","sdl2"],text=True).split() if cpp else []
            run(mode+"."+label,[cc,"-std=c++17" if cpp else "-std=gnu17",*common,*sdk_flags,*flags,*warnings,
                 "-MMD","-MF",OUT/(mode+"."+label+".d"),"-c",path,"-o",obj])
            if rename:run(mode+"."+label+".rename",["objcopy","--redefine-sym="+rename,obj])
        run(mode+".link",["/usr/bin/clang++","-no-pie",*flags,"-Wl,--gc-sections",*objects,"-ldl","-o",OUT/mode])
        run(mode+".test",[OUT/mode,ROOT/"disc/SLUS_006.64"],0)
        print((OUT/(mode+".test.log")).read_text(),flush=True)
    for name,h in receipt["source_pins"].items():
        p=Path(name);p=p if p.is_absolute() else ROOT/p
        assert sha(p)==h,p
    receipt["status"]="EXPECTED RESULTS OBSERVED"
except BaseException as e:
    receipt["status"]="FAILED; OUTPUTS PRESERVED";receipt["error"]=repr(e);raise
finally:
    deps={}
    for p in OUT.glob("*.d"):
        for x in p.read_text().replace("\\\n"," ").split(":",1)[1].split():
            q=Path(x);q=q if q.is_absolute() else ROOT/q
            if q.is_file():
                deps[str(q.resolve())]=sha(q)
                d=OUT/"inputs"/q.resolve().relative_to("/");d.parent.mkdir(parents=True,exist_ok=True)
                if not d.exists():shutil.copy2(q,d)
    receipt["dependency_pins"]=deps
    for cc in ["/usr/bin/x86_64-linux-gnu-gcc-15","/usr/bin/clang","/usr/bin/g++","/usr/bin/clang++"]:
        receipt.setdefault("tool_sha256",{})[cc]=sha(Path(cc))
    save()
