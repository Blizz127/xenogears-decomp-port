"""Verify fixed-base AMD64 PE data against its actual LLD /lldmap contributions."""
import argparse
import os
import re
import struct
from pathlib import Path

LIMIT = 0xffffff
REQUIRED = {"g_PsxRam":"w", "g_FieldRenderContexts":"w", "xenoForwardOtEnd":"r"}

def require(condition, message):
    if not condition:
        raise ValueError(message)

def check(binary, map_path, projects, required=REQUIRED):
    image = Path(binary).read_bytes()
    require(image[:2] == b'MZ', "missing DOS header")
    pe = struct.unpack_from('<I', image, 60)[0]
    require(image[pe:pe+4] == b'PE\0\0', "missing PE signature")
    machine, count, _, _, _, opt_size, chars = struct.unpack_from('<HHIIIHH', image, pe+4)
    opt = pe + 24
    require(machine == 0x8664 and struct.unpack_from('<H',image,opt)[0] == 0x20b, "not AMD64 PE32+")
    base = struct.unpack_from('<Q',image,opt+24)[0]
    size = struct.unpack_from('<I',image,opt+56)[0]
    dll = struct.unpack_from('<H',image,opt+70)[0]
    require(chars & 1 and not dll & 0x60 and struct.unpack_from('<II',image,opt+112+8*5) == (0,0), "PE is relocatable")
    require(0 < base < LIMIT and base + size <= 0x100000000, "data pointer truncation above 4 GiB")
    sections = {}
    for i in range(count):
        offset=opt+opt_size+40*i
        name, length, rva = struct.unpack_from('<8sII',image,offset)
        flags = struct.unpack_from('<I',image,offset+36)[0]
        sections[name.rstrip(b'\0').decode()] = (rva,length,flags)
    map_sects, seen, project_data, named = {},set(),[],{}
    out = owner = None
    projects = {Path(p).name for p in projects}
    for line in Path(map_path).read_text().splitlines()[1:]:
        match = re.match(r'^([0-9a-fA-F]+)\s+([0-9a-fA-F]+)\s+\d+ (\s*)(.+)$',line)
        if not match: continue
        rva,length = (int(x,16) for x in match.group(1,2))
        indent,label=match.group(3,4)
        if not indent:
            out=label
            map_sects[out]=(rva,length)
            owner=None
        elif token:=re.fullmatch(r'(.+):\(([^()]*)\)',label):
            obj,in_sec=token.groups()
            owner=(rva,length,out,obj,in_sec)
            if Path(obj).name in projects:
                seen.add(Path(obj).name)
                if length and (in_sec.split('$')[0] in ('.bss','.data','.sdata','.sbss') or out=='.data'):
                    project_data.append(owner)
        elif label in required and owner is not None:
            start, extent, section, origin, in_sec=owner
            if extent and start <= rva < start + extent:
                named.setdefault(label,set()).add((rva,start+extent,section))
    for key in ('.text','.rdata','.data'):
        require(key in sections and map_sects.get(key)==sections[key][:2], f"{key} link map and PE disagree")
    require(projects and projects <= seen, f"missing project inputs from link map: {sorted(projects-seen)[:8]}")
    require(project_data, "map has no writable project contributions")
    for start,length,out,obj,sec in project_data:
        require(0 < base+start < base+start+length <= LIMIT, f"compact range exceeded: {obj} {sec} [{base+start:#x},{base+start+length:#x})")
    for symbol,mode in required.items():
        matches=named.get(symbol,set())
        require(len(matches)==1, f"{symbol}: no unique PE link-map range {matches}")
        start,end,section=next(iter(matches))
        require(bool(sections[section][2]&0x80000000)==(mode=='w'), f"{symbol}: unexpected PE section {section}")
        require(0 < base+start < base+end <= LIMIT, f"{symbol}: compact range exceeded")
    high=max(base + a + n for a,n,*_ in project_data)
    return f"PE fixed at {base:#x}, image end {base+size:#x}; {len(project_data)} project data inputs end at or below {high:#x}; {len(named)} named packet ranges pass"

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('binary',type=Path)
    parser.add_argument('map',type=Path)
    parser.add_argument('project_objects',nargs='+')
    args=parser.parse_args()
    print(check(args.binary,args.map,args.project_objects))
