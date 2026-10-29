#!/usr/bin/env python3
"""Actual full system.c TU versus retail 34888 and original link leaves.

Only the same-TU text service is weakened for an explicit fixture boundary.
-fno-inline keeps that boundary observable at every optimization level.
Actual temp2.c and guest_prim_link.c packet bodies execute under instrumented
call wrappers. Unreferenced functions are discarded, as in the menu fixture.
"""
from pathlib import Path
import hashlib,json,os,shlex,shutil,subprocess,sys

ROOT=Path(os.environ.get("XENO_TEST_ROOT",Path(__file__).resolve().parents[2])).resolve()
HERE=Path(__file__).resolve().parent
SOURCE=Path(os.environ.get("WINDOW_RENDER_SOURCE",ROOT/"src/slus_006.64/system/system.c")).resolve()
TEMP2=Path(os.environ.get("WINDOW_LINK_TEMP2_SOURCE",ROOT/"src/slus_006.64/system/temp2.c")).resolve()
DOMAIN=Path(os.environ.get("WINDOW_LINK_DOMAIN_SOURCE",ROOT/"pc_port/src/guest_prim_link.c")).resolve()
HEADER_DIR=Path(os.environ.get("WINDOW_LINK_HEADER_DIR",ROOT/"pc_port/include_shim")).resolve()
OUT=Path(os.environ["WINDOW_RENDER_OUT"]).resolve()
assert not OUT.exists()
OUT.mkdir(parents=True)
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
disc=ROOT/"disc/SLUS_006.64"
assert sha(disc)=="dc0b2dd786203d4cce5927c5a3fc85a18f39a3f7406078860076ebb0bbae7119"
assert sha(ROOT/"pc_port/src/battle_mips_adapter.c")=="a87b5c7afaf102cba4ab9903d275df69cc4e9f8ad12142f8f69e379112fcbc67"
assert sha(ROOT/"pc_port/src/battle_mips_adapter.h")=="9924f3f892f0660b5b9c9cb0fb492d9180e451bfa1e727ec825c4f7232425315"
blob=disc.read_bytes()
body=blob[0x25088:0x25088+1572]
assert hashlib.sha256(body).hexdigest()=="e8d19a193ccbd49aefe85c2fedbc79a4189a637eca9861baa9012c7938a1f418"
paths=[SOURCE,TEMP2,DOMAIN,HEADER_DIR/"guest_prim_link.h",disc,HERE/"system_window_render_retail_test.c",Path(__file__).resolve(),
       ROOT/"pc_port/src/battle_mips_adapter.c",ROOT/"pc_port/src/battle_mips_adapter.h"]
pins={str(p):sha(p) for p in paths}
receipt=dict(source_pins=pins,commands=[],returncodes={},status="RUNNING",
             original_body_sha256=hashlib.sha256(body).hexdigest(),
             scope=__doc__,native_runtime_calls="none: direct host C with interpreter disabled; adapter counters must not change")
receipt["input_snapshots"]={}
for source,digest in pins.items():
    dest=OUT/"input-snapshot"/Path(source).relative_to("/")
    dest.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(source,dest)
    assert sha(dest)==digest
    receipt["input_snapshots"][source]=str(dest)
def save():(OUT/"receipt.json").write_text(json.dumps(receipt,indent=2)+"\n")
def run(label,args,expected=0):
    receipt["commands"].append(dict(label=label,argv=[str(x) for x in args]));save()
    with (OUT/(label+".log")).open("x") as f:
        r=subprocess.run(args,cwd=ROOT,stdout=f,stderr=subprocess.STDOUT,timeout=60)
    receipt["returncodes"][label]=r.returncode;save()
    if r.returncode!=expected:
        print((OUT/(label+".log")).read_text()[-5000:])
        raise RuntimeError((label,r.returncode,expected))
common=["-include","assert.h","-std=gnu17","-fno-pie","-fno-builtin","-fno-inline",
        "-DXENO_PC_PORT","-DXENO_FIELD_OBJECT_OVERLAY","-DSKIP_ASM","-D_LANGUAGE_C",
        "-DUSE_EXTENDED_PRIM_POINTERS=0","-ffunction-sections","-fdata-sections",
        "-I"+str(HEADER_DIR),"-Ipc_port/include_shim","-Iinclude","-Ipc_port/src",
        "-Ipc_port/extern/PsyCross/include","-Ipc_port/extern/PsyCross/include/psx"]
try:
    run("sanitizer-preflight",["bash","-c","source pc_port/tests/lib/sanitizer_preflight.sh; ubsan_preflight gcc clang"])
    modes=os.environ.get("WINDOW_RENDER_MODES","O0 O2 UBSan").split()
    for mode in modes:
        flags=["-"+mode] if mode!="UBSan" else ["-O1","-fsanitize=undefined","-fno-sanitize-recover=all"]
        compiler="/usr/bin/x86_64-linux-gnu-gcc-15"
        clang="/usr/bin/clang"
        run(mode+".source",[compiler,*common,*flags,"-fpermissive","-w","-c",str(SOURCE),"-o",str(OUT/(mode+".source.o"))])
        run(mode+".weaken",["objcopy","--weaken-symbol=func_80033DF0",str(OUT/(mode+".source.o"))])
        run(mode+".fixture",[compiler,*common,*flags,"-Wall","-Wextra","-Werror","-Wno-misleading-indentation","-c",str(HERE/"system_window_render_retail_test.c"),"-o",str(OUT/(mode+".test.o"))])
        run(mode+".cpu",[clang,*common,*flags,"-c","pc_port/src/battle_mips_adapter.c","-o",str(OUT/(mode+".cpu.o"))])
        for label,path,symbol in (("packet",TEMP2,"func_80031798=WindowBody_LinkPrim"),
                                  ("domain",DOMAIN,"PcPort_AddPrimDomainAware=WindowBody_AddPrim")):
            run(mode+"."+label+".compile",[compiler,*common,*flags,"-fpermissive","-w","-c",str(path),"-o",str(OUT/(mode+"."+label+".o"))])
            run(mode+"."+label+".rename",["objcopy","--redefine-sym="+symbol,str(OUT/(mode+"."+label+".o"))])
        run(mode+".link",[clang,"-no-pie",*flags,"-Wl,--gc-sections",str(OUT/(mode+".source.o")),str(OUT/(mode+".test.o")),str(OUT/(mode+".cpu.o")),str(OUT/(mode+".packet.o")),str(OUT/(mode+".domain.o")),"-o",str(OUT/mode)])
        args=[str(OUT/mode),str(disc)]
        if os.environ.get("WINDOW_RENDER_WITNESS_ONLY")=="1":args.append("witness")
        run(mode+".test",args,int(os.environ.get("WINDOW_RENDER_EXPECT","0")))
        print((OUT/(mode+".test.log")).read_text(),flush=True)
    if os.environ.get("WINDOW_RENDER_SKIP_CONTROLS","0")!="1":
        assert "O2" in modes
        src=SOURCE.read_text();start=src.index("void func_80034888(void*")
        end=src.index("\n#endif",start);body=src[start:end]
        wrap="if (rowCursor >= *(s16*)(pWindow + 0x0C))"
        reload="        pRows = (u8*)(uintptr_t)*(u32*)(pWindow + 0x28);\n"
        text_reload="            pRows = (u8*)(uintptr_t)*(u32*)(pWindow + 0x28);\n"
        assert body.count(wrap)==2 and body.count(reload)==3
        def nth(s,old,new,n):
            at=-1
            for _ in range(n):at=s.index(old,at+1)
            return s[:at]+new+s[at+len(old):]
        controls={
            "first-wrap":nth(body,wrap,"if (0)",1),
            "second-wrap":nth(body,wrap,"if (0)",2),
            "stuck-cursor":body.replace("        rowCursor++;\n",""),
            "stale-loop-rows":body.replace(reload,"",2),
            "stale-upload-rows":body.replace(text_reload,""),
            "masked-x":body.replace("(u32)(*(s16*)(pWindow + 0x04) - 7) |","((u32)(*(s16*)(pWindow + 0x04) - 7) & 0xFFFFu) |"),
            "width-64-threshold":body.replace("if (*(s16*)(row + 0x58) >= 0x41)","if (*(s16*)(row + 0x58) >= 0x40)"),
            "single-timer-tick":body.replace("    if (*(s16*)(pWindow + 0x84) != 0) {\n","    if (0) {\n",1),
        }
        receipt["mutants"]={}
        for name,mutant in controls.items():
            assert mutant!=body,name
            path=OUT/(name+".c");path.write_text(src[:start]+mutant+src[end:])
            receipt["mutants"][name]=sha(path)
            run(name+".source",[compiler,*common,"-O2","-fpermissive","-w","-c",str(path),"-o",str(OUT/(name+".o"))])
            run(name+".weaken",["objcopy","--weaken-symbol=func_80033DF0",str(OUT/(name+".o"))])
            run(name+".link",[clang,"-no-pie","-O2","-Wl,--gc-sections",str(OUT/(name+".o")),str(OUT/"O2.test.o"),str(OUT/"O2.cpu.o"),str(OUT/"O2.packet.o"),str(OUT/"O2.domain.o"),"-o",str(OUT/name)])
            args=[str(OUT/name),str(disc)]
            if name in ("first-wrap","second-wrap","stuck-cursor"):args.append("witness")
            run(name+".test",args,1)
            assert "WINDOW FAIL cases=" in (OUT/(name+".test.log")).read_text(),name
            print("WINDOW control rejected:",name,flush=True)
    for p,h in pins.items():assert sha(Path(p))==h,p
    receipt["status"]="EXPECTED RESULTS OBSERVED"
except BaseException as e:
    receipt["status"]="FAILED; OUTPUTS PRESERVED";receipt["error"]=repr(e);raise
finally:save()
