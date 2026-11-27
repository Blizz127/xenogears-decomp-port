"""Find semantic literal projections of the resident renderer and boot tables."""
import bisect
import hashlib
import re
import struct
from collections import defaultdict


GLOBALS = ("D_800500F8", "D_800500FC", "D_80050100", "D_80050104")
STRIDES = ("cmdStride", "packetStride", "outputStride")
POINTERS = ("pMemStart", "pHeapStart")
STATE = POINTERS + ("hasOverlay",)
# The digest dictionaries follow this module. They identify rows without the data.
TOKEN = re.compile(r"""(?P<skip>/\*[\s\S]*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"|'(?:\\.|[^'\\])*'|\s+)|(?P<word>[A-Za-z_]\w*)|(?P<num>0[xX][0-9a-fA-F]+[uUlL]*|0[bB][01]+[uUlL]*|[0-9]+[uUlL]*)|(?P<punct>.)""")
TYPES = frozenset(("unsigned", "signed", "char", "int", "short", "long",
                   "s8", "u8", "s16", "u16", "s32", "u32", "s64", "u64",
                   "int8_t", "uint8_t", "int16_t", "uint16_t", "int32_t",
                   "uint32_t", "int64_t", "uint64_t", "uintptr_t", "intptr_t",
                   "size_t", "void", "*", "const"))
WRAPPERS = frozenset(("PSX_ADDR", "UINT32_C", "INT32_C", "UINT16_C", "INT16_C",
                      "UINT64_C", "INT64_C", "UINT8_C", "INT8_C"))


def integer(word):
    if not word or not word[0].isdigit():
        return None
    number = re.sub(r"[uUlL]+$", "", word).lower()
    try:
        if number.startswith(("0x", "0b")):
            return int(number, 0)
        return int(number, 8 if len(number) > 1 and number[0] == "0" else 10)
    except ValueError:
        return None


def literal(words):
    """Evaluate a single C integer, with ordinary casts, parentheses or PSX_ADDR."""
    if not words:
        return None
    if len(words) == 1:
        return integer(words[0])
    if words[0] in ("+", "-"):
        value = literal(words[1:])
        if value is not None:
            return value if words[0] == "+" else -value
    if words[0] in WRAPPERS and words[1] == "(" and words[-1] == ")":
        return literal(words[2:-1])
    if words[0] == "(":
        depth = 0
        for i, word in enumerate(words):
            depth += (word == "(") - (word == ")")
            if not depth:
                if i == len(words) - 1:
                    return literal(words[1:i])
                if i > 1 and all(t in TYPES for t in words[1:i]):
                    return literal(words[i+1:])
                break
    return None


def guest_offset(value):
    if 0 <= value < 0x200000:
        return value
    if value >> 29 in (4, 5) and value & 0x1fffffff < 0x200000:
        return value & 0x1fffffff
    return None


def scan_source(path, text):
    """Return matching complete field groups, with source line numbers and digests."""
    if not any(name in text for name in GLOBALS + STRIDES + POINTERS):
        return []
    tokens = [t for t in TOKEN.finditer(text) if t.lastgroup != "skip"]
    words = [t.group() for t in tokens]
    newlines = [m.start() for m in re.finditer("\n", text)]
    line = lambda i: 1 + bisect.bisect_left(newlines, tokens[i].start())
    global_values = {}
    by_table = defaultdict(lambda: defaultdict(dict))
    scopes = []
    for i, w in enumerate(words):
        if w == "{":
            table, row = scopes[-1] if scopes else (None, None)
            if i >= 4 and words[i-1] == "=" and words[i-2] == "]":
                start = i - 3
                while start >= 0 and words[start] not in ("[", "{", ";", "]"):
                    start -= 1
                if start > 0 and words[start] == "[":
                    if words[start-1].isidentifier():
                        table, row = (words[start-1], i), None
                    elif table is not None:
                        row = literal(words[start+1:i-2])
            scopes.append((table, row))
            continue
        if w == "}":
            if scopes:
                scopes.pop()
            continue
        if w not in GLOBALS + STRIDES + STATE or i + 2 >= len(words) or words[i+1] != "=":
            continue
        end = i + 2
        while end < len(words) and words[end] not in (";", ",", "}") and end - i < 36:
            end += 1
        value = literal(words[i+2:end])
        if value is None:
            continue
        if w in GLOBALS:
            global_values[w] = (value, line(i))
            continue
        if i == 0 or words[i-1] != ".":
            continue
        if i >= 5 and words[i-2] == "]":
            start = i - 3
            while start >= 0 and words[start] not in ("[", "{", ";", "]"):
                start -= 1
            if start <= 0 or words[start] != "[" or not words[start-1].isidentifier():
                continue
            table, row = (words[start-1], None), literal(words[start+1:i-2])
        elif scopes:
            table, row = scopes[-1]
        else:
            continue
        if table is None or row is None:
            continue
        if w in POINTERS:
            value = guest_offset(value)
            if value is None:
                continue
        by_table[table][row][w] = (value, line(i))

    hits = []

    def compare(family, fields, cells, expected, rows=None):
        if len(cells) != len(fields) * (len(rows) if rows else 1):
            return
        payload = b"".join(struct.pack("<I", value & 0xffffffff) for value, _ in cells)
        digest = hashlib.sha256(payload).hexdigest()
        if digest == expected:
            hit = {"path": str(path), "line": min(n for _, n in cells),
                   "family": family, "fields": list(fields), "bytes": len(payload),
                   "sha256": digest}
            if rows is not None:
                hit["rows"] = list(rows)
            hits.append(hit)

    compare("renderer_globals", GLOBALS,
            [global_values[k] for k in GLOBALS if k in global_values], GLOBAL_DIGEST)
    for rows in by_table.values():
        for family, fields, fingerprints, aggregate in (
                ("renderer_strides", STRIDES, STRIDE_HASHES, STRIDE_DIGEST),
                ("boot_pointer_pairs", POINTERS, PAIR_HASHES, PAIR_DIGEST),
                ("boot_state_scalars", STATE, STATE_HASHES, STATE_DIGEST)):
            ordered = []
            for row, digest in fingerprints.items():
                values = rows.get(row, {})
                if all(k in values for k in fields):
                    cells = [values[k] for k in fields]
                    ordered.extend(cells)
                    compare(family, fields, cells, digest, (row,))
            compare(family, fields, ordered, aggregate, tuple(fingerprints))
    return hits


GLOBAL_DIGEST = 'becabbaf173b6dd2fef25c649fb8022d97c21d2168febf4f7c8e11cac64c060a'
STRIDE_DIGEST = '03a8ab521d077b6eb22d66af7d8e52c8518798d797de5cdaa756ee591328ff38'
STRIDE_HASHES = {
    0: "5b5338af887cfb494210993ceb177bc8978a1fb9d60a589e116832022f6c4cf4",
    1: "31253fc453cad947ed72be02ff5bdd5bbc6e7c8584d91304c45bf5ca10b2a9d1",
    2: "0eacd67d966fe8d152b076dcb0557cf8f92bcf9f67c5bdcefec0a0ce06e658cb",
    3: "aaf1c96717d309346bafbea3afd83aa328dc216cf36e799900079e64464449d0",
    4: "5b5338af887cfb494210993ceb177bc8978a1fb9d60a589e116832022f6c4cf4",
    5: "31253fc453cad947ed72be02ff5bdd5bbc6e7c8584d91304c45bf5ca10b2a9d1",
    6: "0eacd67d966fe8d152b076dcb0557cf8f92bcf9f67c5bdcefec0a0ce06e658cb",
    7: "aaf1c96717d309346bafbea3afd83aa328dc216cf36e799900079e64464449d0",
    8: "8ea8a787acbbc8c6f42e72e8ada3dd2ec68557359e8a72390dcc73dd50464bc0",
    9: "4270976d03d6affa743f6e0f53636bd22ba0800a1c387a8afea82bfd02a8c00f",
    10: "01a7598daf1ca1767a1159992b46f20bd5851fe4a2a711d84d08669b0b50b251",
    11: "119337894a4eb999544f22fd11e9d0b16fb0fcadcb1e872c5ecf49e16ad6fece",
    12: "8ea8a787acbbc8c6f42e72e8ada3dd2ec68557359e8a72390dcc73dd50464bc0",
    13: "4270976d03d6affa743f6e0f53636bd22ba0800a1c387a8afea82bfd02a8c00f",
    14: "01a7598daf1ca1767a1159992b46f20bd5851fe4a2a711d84d08669b0b50b251",
    15: "119337894a4eb999544f22fd11e9d0b16fb0fcadcb1e872c5ecf49e16ad6fece",
    16: "c6fa3f901e02b87b389014a870a459d52b1f78c1d2946f481d8d84a48c6cbe2f",
}
PAIR_DIGEST = '55380881a9a14594d85d00e0c7560e4a144b7ea29fdfc7494a711493bee7e477'
PAIR_HASHES = {
    0: "8d049a8d346cd7f05254dc6b4c68953c835d5de392aebe44bf639e1ca491d0f1",
    1: "a5d02ada6e7180d2975f5d6e023f24c7abfe9d9ebd12b1d92426b18c462d4b2a",
    2: "140d8baf8c8f4c7a18c058328b139898dda6a19d9bc741619047b0b3843fd570",
    3: "15616b17f91eacd4d32bcfeaa8cff449b4a5e01d23756f41258d016a731cde12",
    5: "8d049a8d346cd7f05254dc6b4c68953c835d5de392aebe44bf639e1ca491d0f1",
    6: "df4a25ef5e2ef65eacf2c7686e9187070ed8bc4d6bb20bdc3d35afa784748ae3",
}
STATE_DIGEST = 'ccf10a3f1d1749e40a35835a170e084ce0317ad0bb9d8945789ec5764dca34a3'
STATE_HASHES = {
    0: "07e367780b86cc0e1e84d93f8b53ecf34550961787e1185502ea0d299dda94d9",
    1: "04b3d190521d25c670eb38e577ab787062b7dca241eac7d5d7c85b1212d2d7ac",
    2: "18ae784b95e127e33b3f7533a2b1dc2e50ade4d877108bf5b3f4c29cb4759007",
    3: "20faf096a251351d8114e97dc6ea0145df9ee2e357cd44390654b34ff5ab977e",
    5: "07e367780b86cc0e1e84d93f8b53ecf34550961787e1185502ea0d299dda94d9",
    6: "683d8875549b19c9d392e06616112668f5d68fba4f8e862ec426e01cc8a00d2e",
}
