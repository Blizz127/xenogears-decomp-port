"""Return physically attributed child-callback table copies in selected C sources."""
import hashlib
import re
import struct

ROW_DIGEST = "604b19b8756542d9d9914258e6f8e892346572603150a7df945cc96ca42ffe2d"
MASK_DIGEST = "443c7fecee62c4745ab7b772b188e3594036aa90886ddb318be75aa5e8c07bc5"

def _word(field):
    value = re.search(r"(?:func_|0[xX])([8][0-9A-Fa-f]{7})[uUlL]*\s*$", field)
    if value:
        return int(value[1],16)
    return 0 if re.fullmatch(r"(?:NULL|0[xX]0+[uUlL]*|0[uUlL]*)",field.strip()) else None

def _row(fields):
    values=[_word(f) for f in fields]
    if len(values)!=16 or any(v is None for v in values):
        return False
    return hashlib.sha256(struct.pack("<16I",*values)).hexdigest()==ROW_DIGEST

def scan(path,text):
    if 'func_' not in text and '0x8' not in text and '0X8' not in text:
        return []
    out=[]
    without_comments=re.sub(r"/\*.*?\*/|//[^\n]*", lambda m:"".join("\n" if x=="\n" else " " for x in m[0]),text, flags=re.S)
    def mark(offset,kind):out.append({"path":path,"line":text.count("\n",0,offset)+1,"kind":kind})
    for m in re.finditer(r"=\s*\{([^{};]*)\}",without_comments,re.S):
        if _row(m[1].rstrip().removesuffix(",").split(",")):
            mark(m.start(),"original indexed callback row in initializer")
    for m in re.finditer(r"/\*.*?\*/",text,re.S):
        comment=re.sub(r"\n\s*\*\s*"," ",m[0])
        linear=re.search(r"(?:\d+\s+entries\s*:\s*)?((?:[8][0-9A-Fa-f]{7}|0)(?:\s*,\s*(?:[8][0-9A-Fa-f]{7}|0)){15})\b",comment)
        if linear and _row([v if v.strip()=="0" else "0x"+v.strip() for v in linear[1].split(",")]):
            mark(m.start(),"original indexed callback row in comment")
        grouped=re.findall(r"\b([\d/\-]+)\s*=\s*(NULL|func_[0-9A-Fa-f]{8})",comment)
        if grouped:
            fields=[None]*16
            for slots,target in grouped:
                for part in slots.split("/"):
                    if not re.fullmatch(r"[0-9]{1,2}(?:-[0-9]{1,2})?",part): continue
                    limits=part.split("-")
                    if len(limits)>2:continue
                    first=int(limits[0]);end=int(limits[-1])
                    for i in range(first,min(end+1,16)):fields[i]=target
            if all(f is not None for f in fields) and _row(fields):
                mark(m.start(),"original indexed callback row in grouped comment")
    if "func_80025224" in text:
        fn=re.search(r"\bfunc_80025224\s*\([^()]*\)\s*\{(.*?)^\}",without_comments,re.S|re.M)
        if fn:
            for m in re.finditer(r"\b[A-Za-z_]\w*\s*=\s*(0[xX][0-9a-fA-F]+|\d+)[uUlL]*\s*;",fn[1]):
                number=int(m[1],16 if m[1].lower().startswith("0x") else 10) if len(m[1])<=18 else -1
                if 0<=number<65536 and hashlib.sha256(struct.pack("<H",number)).hexdigest()==MASK_DIGEST:
                    mark(fn.start(1)+m.start(),"original non-null callback mask in binder")
    return out
