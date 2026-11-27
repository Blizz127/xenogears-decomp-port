"""Check for recurrences of reviewed world-map translations and source listings."""
import hashlib
import re


# The short marker is a necessary token from the reviewed body or comment.
# It selects candidates only. Findings always require the complete token digest.
_BODY_RULES = (
    (b"WM_73448_MUTANT_M2", 294, "54a2b2a52e2b3dddd3c84ac862ef351167a76b81cf58e6e142fd6e2e2adce496", "wm_80073448"),
    (b"WM_75228_HALFWORD_COUNT", 94, "597f7d8fec3069299e0c4725be240c5acd1fa75fbbe4b486e629953eff4d1307", "wm_80075228"),
    (b"WM_85FE0_RECORD_BASE_OFFSET", 378, "63b2219274c89e0eb802e7d636c0c66f128c4e896e1678e1297ba474d4a09d30", "wm_80085FE0"),
    (b"WM_865A0_MUTANT_SHIFTED_HEADER", 343, "d01bcdfe2279aae5f9ae9cc55c5351269de37337c563652b6e12809e06b880f6", "wm_800865A0"),
    (b"WM_8901C_MUTANT_SWAPPED_TEXTURE_FIELDS", 324, "89341508687bf511aa5cf3016b854b1d267492bfc491a35a679a8bd8db7f04e5", "wm_8008901C"),
    (b"WM_89160_FLAG_BYTE_OFFSET", 1314, "7ed9c7e3f4ef3e45e114b0df76871c0923c576363b1a41d3d0fbc49565c03e00", "wm_80089160"),
    (b"WM_914D0_FRAME_DELTA", 1446, "9199f1a07a831fb964a8d06334c762a3206e8aa812ad0954a82b5b6801a4e608", "wm_800914D0"),
    (b"WM_981C8_MUTANT_M1", 493, "b3e4a9e49aaaf982bb781c32731cc4aaac2a135553927309fa0957c9de036a28", "wm_800981C8"),
    (b"WM_DR_TPAGE_ABS", 675, "c3a16ed184e2fbf2545cebe0457a0c760110aa4d45aeed144729abb864dfa5ad", "wm_800923A8"),
    (b"WM_IDENTITY_MATRIX_SRC", 253, "760014a6ec07f4cb48aa71a094ae3fe991c788922030162205f6897d4f91c150", "wm_80097BC0"),
    (b"WM_TERRAIN_STRIDE_ABS", 294, "25af75ca7ac4aa8f729ed639ada259f90a614528090970f127373dce065e32ae", "wm_80093660"),
)
_COMMENT_RULES = (
    (b"80075284", "a215368a53e8ef16321b7668e2b1b4549cc811e2faced05c69fec313d2eca373", "world_map_common_tail block 1481"),
    (b"800914d0", "f86f00110de21089bd70c0295db88319d489ecff38585781920cf5fec8982b15", "world_map_callback_914d0 block 1"),
    (b"80093660", "0c73519e731fb99c08ea5086351992866372355a96a314b3871921d1df060dd0", "world_map_terrain_cell block 36"),
    (b"80093688", "ccf6753807eac55a06401e18a6f07bca37450b760fbddc403d29e324b2f1a2d5", "world_map_terrain_cell block 86"),
    (b"80093690", "8ba074eedb64bbacb34ac7de4327101ba7fe763f2b45eb25e3430b915c518697", "world_map_terrain_cell block 89"),
    (b"80093734", "2d3af4eecc87f83d5fd6e976db1ef542c262b96e09183724c3b6895fb6e88672", "world_map_terrain_cell block 140"),
)
_TOKEN = re.compile(
    r"""(?:u8|[LuU])?"(?:\\.|[^"\\])*"|(?:[LuU])?'(?:\\.|[^'\\])*'"""
    r"""|/\*.*?\*/|//[^\n]*|[A-Za-z_]\w*|(?:0[xX][a-fA-F\d]+|\d+)(?:[uUlL]*)"""
    r"""|>>=|<<=|\.\.\.|==|!=|<=|>=|&&|\|\||\+\+|--|<<|>>|->|[+\-*/%&|^]=|[^\s]""",
    re.DOTALL,
)
_COMMENT_TOKEN = re.compile(r"0x[0-9a-f]+|[a-z_][a-z0-9_]*|\d+|[^\s*]")


def _digest(tokens):
    return hashlib.sha256(b"\x00".join(tokens)).hexdigest()


# Detect the complete reviewed scheduler listing even when it is moved to a
# different comment or its surrounding prose changes.
_SCHEDULER_SHAPE_LENGTH = 192
_SCHEDULER_SHAPE_SHA256 = "79ab941d71ac72e49c83935f946c232ec81d6f3fc9a009638b58c6296f8f7ba4"


def _has_scheduler_listing(data):
    if b"0x80097830" not in data.lower():
        return False
    words = _COMMENT_TOKEN.findall(data.decode("utf-8", errors="replace").lower())
    for index, word in enumerate(words):
        if word != "s1" or index + _SCHEDULER_SHAPE_LENGTH > len(words):
            continue
        sequence = words[index:index + _SCHEDULER_SHAPE_LENGTH]
        if _digest(item.encode() for item in sequence) == _SCHEDULER_SHAPE_SHA256:
            return True
    return False


def check_world_source(name, data):
    """Return descriptions for complete known source bodies or listing comments."""
    if len(data) < 24 or b"\x00" in data:
        return []
    bodies = {}
    for marker, size, digest, label in _BODY_RULES:
        if marker in data:
            bodies.setdefault(size, {})[digest] = label
    comments = {}
    if b"/*" in data or b"//" in data:
        lower = data.lower()
        for marker, digest, label in _COMMENT_RULES:
            if marker in lower:
                comments[digest] = label
    hits = set()
    if _has_scheduler_listing(data):
        hits.add("confirmed original world-map scheduler control-flow listing")
    if not bodies and not comments:
        return sorted(hits)

    tokens = []
    openings = []
    for match in _TOKEN.finditer(data.decode("utf-8", errors="replace")):
        word = match[0]
        if word.startswith(("/*", "//")):
            if comments:
                words = _COMMENT_TOKEN.findall(word.strip("/*").lower())
                label = comments.get(_digest(x.encode() for x in words))
                if label:
                    hits.add("confirmed original world-map listing " + label)
        elif bodies:
            if word == "{":
                openings.append(len(tokens))
            tokens.append(word.encode())
    # Token spans work with renamed functions and #if alternatives which
    # introduce unmatched braces when reading unpreprocessed source.
    for start in openings:
        for size, rules in bodies.items():
            end = start + size
            if end <= len(tokens) and tokens[end - 1] == b"}":
                label = rules.get(_digest(tokens[start:end]))
                if label:
                    hits.add("confirmed original world-map translation " + label)
    return sorted(hits)
