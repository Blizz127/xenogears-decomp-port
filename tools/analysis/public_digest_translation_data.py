"""Recognize complete confirmed source fixtures by their canonical values."""
import hashlib
import json
from functools import lru_cache
from pathlib import Path
import re
from public_digest_renderer_fields import scan_source
from public_digest_world_origin import check_world_source
from public_digest_callback_row import scan as callback_rows

# Numeric lists are accepted in either C or Python source. Width and byte order
# reflect the represented original values, independent of spelling and file name.
NUMBER = rb'[+-]?(?:0[xX][0-9a-fA-F]+|0[bB][01]+|[0-9]+)[uUlL]*'
RUN = re.compile(rb'(?<![A-Za-z0-9_])'+NUMBER+rb'(?:\s*,\s*'+NUMBER+rb'){3,}(?![A-Za-z0-9_])')
TOKEN = re.compile(NUMBER)
HEX = re.compile(rb"(?<![A-Fa-f0-9])(?:[A-Fa-f0-9]{2}){8,}(?![A-Fa-f0-9])")
ESCAPED = re.compile(rb'(?:\\x[0-9a-fA-F]{2}){8,}')
LISTING = re.compile(rb'(?im)^\s*(?:\*|//)?\s*(8[0-9a-f]{7})\s+([0-9a-f]{8})\s+[a-z][a-z0-9.]*\b')
SPLAT_SAMPLE = re.compile(rb'(?i)\b([0-9a-f]{4,8})\s+(8[0-9a-f]{7})\s+([0-9a-f]{8})\s*\*\s*/\s*([a-z]+)\s+([^\n]+)')
COMMENTS = re.compile(rb'/\*.*?\*/|//[^\n]*', re.S)
C_WORDS = re.compile(rb'(?:0[xX][a-fA-F0-9]+|[a-zA-Z_$][a-zA-Z0-9_$]*|[0-9]+|[^\s])')
# Require all three comparisons in the same disjunction, or all seven
# opcode-value pairs in one literal initializer. Single common scalars are valid.
TRIPLE_WORD = re.compile(rb'!=\s*('+NUMBER+rb')')
OR_TRIPLE = re.compile(rb'\b[A-Za-z_]\w*\s*!=\s*'+NUMBER+rb'(?:\s*\|\|\s*[A-Za-z_]\w*\s*!=\s*'+NUMBER+rb'){2}\b')
PAIR = rb'\{\s*'+NUMBER+rb'\s*,\s*'+NUMBER+rb'\s*\}'
PAIRS = re.compile(PAIR+rb'(?:\s*,\s*'+PAIR+rb'){6,}')
# Eight consecutive nonzero single-bit little-endian 16-bit button masks.
# A full digest comparison distinguishes the original ordering.
BUTTON_MASK_RUN = re.compile(rb'(?=(?:[\x01\x02\x04\x08\x10\x20\x40\x80]\x00){8})')
STRING = re.compile(rb'"(?:\\.|[^"\\\r\n])*"|\'(?:\\.|[^\'\\\r\n])*\'')
def sha(data):
    return hashlib.sha256(data).hexdigest()

@lru_cache(maxsize=1)
def rules():
    data=json.loads(Path(__file__).with_name('public_translation_data.json').read_bytes())
    spans={}
    for sig,row in data['spans'].items():
        spans.setdefault(row['length'],{})[sig]=row['label']
    return spans,data

def integer(token):
    digits=token.rstrip(b'uUlL')
    magnitude=digits.lstrip(b'+-').lower()
    base=16 if magnitude.startswith(b'0x') else 2 if magnitude.startswith(b'0b') else 8 if len(magnitude)>1 and re.fullmatch(rb"0[0-7]+",magnitude) else 10
    return int(digits,base)

def check_translation_data(name, data):
    spans, extra = rules()
    hits=set()
    def inspect(view, width=1, listing=False):
        for size, signatures in spans.items():
            if size>len(view) or size%width or (size<16 and not listing):
                continue
            for start in range(0,len(view)-size+1,width):
                label=signatures.get(sha(view[start:start+size]))
                if label:
                    hits.add('confirmed original fixture '+label)
    if len(data)<16:
        return []
    if b'\0' in data:
        for match in BUTTON_MASK_RUN.finditer(data):
            start = match.start()
            if sha(data[start:start+16]) == extra['controller_masks']:
                return ['confirmed original fixture controller button mask table']
        return []
    for found in HEX.finditer(data):
        inspect(bytes.fromhex(found[0].decode()),1)
    for found in ESCAPED.finditer(data):
        inspect(bytes.fromhex(found[0].replace(b'\\x',b'').decode()))
    if b',' in data:
        clean=COMMENTS.sub(b' ',data)
        for found in RUN.finditer(clean):
            try:
                values=[integer(x[0]) for x in TOKEN.finditer(found[0])]
            except ValueError:
                continue
            for w in (1,2,4):
                limit=1<<(8*w)
                if all(-(limit//2)<=n<limit for n in values):
                    inspect(b''.join((n%limit).to_bytes(w,'little') for n in values),w)
    previous=None
    words=bytearray()
    for row in LISTING.finditer(data):
        address=int(row[1],16)
        if previous!=address-4:
            inspect(words,4,True); words.clear()
        words.extend(int(row[2],16).to_bytes(4,'little')); previous=address
    inspect(words,4,True)
    if b'*' in data:
        for row in SPLAT_SAMPLE.finditer(data):
            # A solitary opcode is common. Require the recorded file offset, PC,
            # storage-order word, mnemonic and operands together.
            key=b' '.join(C_WORDS.findall(b' '.join(row.groups()).lower()))
            label=extra['listing_examples'].get(sha(key))
            if label:
                hits.add('confirmed original listing example '+label)
    if b'||' in data:
        for row in OR_TRIPLE.finditer(data):
            values=TRIPLE_WORD.findall(row[0])
            if len(values)==3 and all(len(n)<=15 and 0<=integer(n)<2**32 for n in values):
                raw=b''.join(integer(n).to_bytes(4,'little') for n in values)
                if sha(raw)==extra['compared_words']:
                    hits.add('confirmed original fixture compared instruction words')
    if b'{' in data and b'}' in data:
        for group in PAIRS.finditer(data):
            tokens=TOKEN.findall(group[0])
            if any(len(token)>15 for token in tokens):
                continue
            vals=[integer(token) for token in tokens]
            if all(0<=n<256 for n in vals):
                for i in range(0,len(vals)-13,2):
                    if sha(bytes(vals[i:i+14]))==extra['opcode_lengths']:
                        hits.add('confirmed original fixture opcode and length pairs')
    if b'/*' in data and (b'"' in data or b"'" in data):
        for literal in STRING.finditer(data):
            content=literal[0][1:-1].replace(b'\\n',b'\n').replace(b'\\t',b'\t')
            canonical=b''.join(content.split())
            marker=extra['mutation_text'].get(sha(canonical))
            if marker:
                hits.add('confirmed original instruction mutation fixture '+marker)
    if any(key in data for key in (b'cmdStride',b'packetStride',b'outputStride',b'pMemStart',b'pHeapStart',b'D_800500F8')):
        for row in scan_source(name,data.decode(errors='replace')):
            hits.add(f"confirmed original renderer or boot fields {row['family']} at line {row['line']}")
    if b'func_' in data or b'0x8' in data or b'0X8' in data:
        for row in callback_rows(name,data.decode(errors='replace')):
            hits.add(f"confirmed original callback group {row['kind']}")
    if b'/*' in data:
        for comment in COMMENTS.finditer(data):
            if len(comment[0])>40:
                key=sha(b' '.join(C_WORDS.findall(comment[0])))
                if key in extra['renderer_comments']:
                    hits.add('confirmed original renderer listing in a comment')
    hits.update(check_world_source(name,data))
    return sorted(hits)

