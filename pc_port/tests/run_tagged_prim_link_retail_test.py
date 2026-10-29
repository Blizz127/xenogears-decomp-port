#!/usr/bin/env python3
"""Actual full temp2 TU + domain linker versus the nine original instructions."""
from pathlib import Path
import hashlib,json,os,shutil,subprocess,tempfile
ROOT=Path(os.environ.get("XENO_TEST_ROOT",Path(__file__).resolve().parents[2])).resolve()
HERE=Path(__file__).resolve().parent
OUT=Path(os.environ.get("TAGGED_LINK_OUT",str(Path(tempfile.gettempdir())/("xeno-tagged-link-"+os.urandom(8).hex())))).resolve()
SOURCE=Path(os.environ.get("TAGGED_LINK_SOURCE",ROOT/"src/slus_006.64/system/temp2.c")).resolve()
LINK=Path(os.environ.get("TAGGED_LINK_DOMAIN_SOURCE",ROOT/"pc_port/src/guest_prim_link.c")).resolve()
HEADER_DIR=Path(os.environ.get("TAGGED_LINK_HEADER_DIR",ROOT/"pc_port/include_shim")).resolve()
assert not OUT.exists();OUT.mkdir(parents=True)
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
disc=ROOT/"disc/SLUS_006.64"
assert sha(disc)=="dc0b2dd786203d4cce5927c5a3fc85a18f39a3f7406078860076ebb0bbae7119"
assert sha(ROOT/"pc_port/src/battle_mips_adapter.c")=="a87b5c7afaf102cba4ab9903d275df69cc4e9f8ad12142f8f69e379112fcbc67"
assert sha(ROOT/"pc_port/src/battle_mips_adapter.h")=="9924f3f892f0660b5b9c9cb0fb492d9180e451bfa1e727ec825c4f7232425315"
paths=[SOURCE,LINK,HEADER_DIR/"guest_prim_link.h",disc,Path(__file__).resolve(),HERE/"tagged_prim_link_retail_test.c",ROOT/"pc_port/src/battle_mips_adapter.c",ROOT/"pc_port/src/battle_mips_adapter.h"]
pins={str(p):sha(p) for p in paths}
receipt=dict(source_pins=pins,commands=[],returncodes={},status="RUNNING",input_snapshots={},
             original_body_sha256=hashlib.sha256(disc.read_bytes()[0x21f98:0x21f98+36]).hexdigest(),
             scope=__doc__)
for n,h in pins.items():
    p=OUT/"input-snapshot"/Path(n).relative_to("/")
    p.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(n,p);assert sha(p)==h
    receipt["input_snapshots"][n]=str(p)
def save():(OUT/"receipt.json").write_text(json.dumps(receipt,indent=2)+"\n")
def run(label,args,expected=0):
    receipt["commands"].append(dict(label=label,argv=[str(x) for x in args]));save()
    with (OUT/(label+".log")).open("x") as f:
        p=subprocess.run(args,cwd=ROOT,stdout=f,stderr=subprocess.STDOUT,timeout=90)
    receipt["returncodes"][label]=p.returncode;save()
    if p.returncode!=expected:
        print((OUT/(label+".log")).read_text()[-4000:]);raise RuntimeError((label,p.returncode,expected))
gcc="/usr/bin/x86_64-linux-gnu-gcc-15";clang="/usr/bin/clang"
common=["-std=gnu17","-fno-pie","-fno-builtin","-DXENO_PC_PORT","-DXENO_FIELD_OBJECT_OVERLAY",
        "-DSKIP_ASM","-D_LANGUAGE_C","-DUSE_EXTENDED_PRIM_POINTERS=0","-include","assert.h",
        "-ffunction-sections","-fdata-sections","-I"+str(HEADER_DIR),
        "-Ipc_port/include_shim","-Iinclude","-Ipc_port/src",
        "-Ipc_port/extern/PsyCross/include","-Ipc_port/extern/PsyCross/include/psx"]
try:
    run("sanitizer-preflight",["bash","-c","source pc_port/tests/lib/sanitizer_preflight.sh; ubsan_preflight gcc clang"])
    for mode in os.environ.get("TAGGED_LINK_MODES","O0 O2 UBSan").split():
        flags=["-"+mode] if mode!="UBSan" else ["-O1","-fsanitize=undefined","-fno-sanitize-recover=all"]
        for label,path,compiler,extra in [
            ("source",SOURCE,gcc,["-fpermissive","-w"]),
            ("domain",LINK,gcc,["-Wall","-Wextra","-Werror"]),
            ("test",HERE/"tagged_prim_link_retail_test.c",gcc,["-Wall","-Wextra","-Werror"]),
            ("cpu",ROOT/"pc_port/src/battle_mips_adapter.c",clang,["-Wall","-Wextra","-Werror"]),
        ]:
            run(mode+"."+label+".compile",[compiler,*common,*flags,*extra,"-c",str(path),"-o",str(OUT/(mode+"."+label+".o"))])
        run(mode+".link",[clang,"-no-pie",*flags,"-Wl,--gc-sections",
                          *(str(OUT/(mode+"."+n+".o")) for n in ("source","domain","test","cpu")),"-o",str(OUT/mode)])
        args=[str(OUT/mode),str(disc)]
        if os.environ.get("TAGGED_LINK_WITNESS_ONLY")=="1":args.append("witness")
        run(mode+".test",args,int(os.environ.get("TAGGED_LINK_EXPECT","0")))
        print((OUT/(mode+".test.log")).read_text(),flush=True)
    if os.environ.get("TAGGED_LINK_SKIP_CONTROLS","0")!="1":
        assert (OUT/"O2.source.o").exists()
        source=LINK.read_text();a=source.index("void PcPort_LinkTaggedPrimDomainAware(")
        prefix,body=source[:a],source[a:]
        changes={
            "masked-old":("u32 old = *(u32 *)ot;","u32 old = *(u32 *)ot & 0x00FFFFFFu;"),
            "retain-ot-tag":("*(u32 *)ot = link & 0x00FFFFFFu;","*(u32 *)ot = (old & 0xFF000000u) | (link & 0x00FFFFFFu);"),
            "reverse-store-order":("*(u32 *)ot = link & 0x00FFFFFFu;\n    *(u32 *)prim = old | tag;","*(u32 *)prim = old | tag;\n    *(u32 *)ot = link & 0x00FFFFFFu;"),
            "host-link-in-guest":("link = PsxMemory_GuestAddr(prim);","link = (u32)(uintptr_t)prim;"),
            "mixed-domain-write":("            return;","            /* incorrect fallthrough */"),
            "wrong-tag":("*(u32 *)prim = old | tag;","*(u32 *)prim = old | (tag ^ 0x01000000u);"),
        }
        receipt["mutants"]={}
        for name,(old,new) in changes.items():
            assert body.count(old)==1,(name,body.count(old))
            path=OUT/(name+".c");path.write_text(prefix+body.replace(old,new))
            receipt["mutants"][name]=sha(path)
            run(name+".compile",[gcc,*common,"-O2","-Wall","-Wextra","-Werror","-c",str(path),"-o",str(OUT/(name+".o"))])
            run(name+".link",[clang,"-no-pie","-O2","-Wl,--gc-sections",str(OUT/"O2.source.o"),str(OUT/(name+".o")),str(OUT/"O2.test.o"),str(OUT/"O2.cpu.o"),"-o",str(OUT/name)])
            args=[str(OUT/name),str(disc)]
            if name!="mixed-domain-write":args.append("witness")
            run(name+".test",args,1)
            assert "TAGGED LINK FAIL cases=" in (OUT/(name+".test.log")).read_text()
            print("TAGGED LINK control rejected:",name,flush=True)
    for n,h in pins.items():assert sha(Path(n))==h,n
    receipt["status"]="EXPECTED RESULTS OBSERVED"
except BaseException as e:
    receipt["status"]="FAILED; OUTPUTS PRESERVED";receipt["error"]=repr(e);raise
finally:save()
