"""Match complete known translations by their C tokens in any textual archive member."""
import hashlib
import json
from pathlib import Path
import re

from public_world_cohort import BODIES


_CATALOG = json.loads(Path(__file__).with_suffix(".json").read_bytes())
_CATALOG.update(BODIES)
_SIZES = frozenset(row[0] for row in _CATALOG.values())

# Keep strings and character constants intact, including braces or apparent comments.
_TOKEN = re.compile(
    rb'''(?P<literal>(?:u8|[uUL])?"(?:\\.|[^"\\])*"|(?:u8|[uUL])?'(?:\\.|[^'\\])*')'''
    rb'''|(?P<comment>/\*.*?(?:\*/|\Z)|//[^\n]*)|(?P<directive>^[ \t]*\#[^\n]*)'''
    rb'''|(?P<space>[^\S\n]+|\n)|[a-zA-Z_]\w*|(?:\.\d|\d)(?:[eEpP][+-]|[\w.])*'''
    rb'''|\.\.\.|>>=|<<=|->|>>|<<|\+\+|--|&&|\|\||[=!<>+*/%&|^\-]=|\#\#|.''',
    re.DOTALL | re.MULTILINE | re.ASCII,
)


def _scan(data, matches, nested_text=True):
    starts = []
    tokens = []
    for match in _TOKEN.finditer(data):
        token = match[0]
        kind = match.lastgroup
        if kind in ("comment", "directive"):
            if nested_text and b"{" in token and b"}" in token:
                if token.startswith(b"/*"):
                    _scan(token[2:-2] if token.endswith(b"*/") else token[2:], matches, False)
                elif kind == "directive" and re.match(rb"^[ \t]*#[ \t]*define\b", token):
                    _scan(token[token.index(b"#") + 1:], matches, False)
            continue
        if kind == "space":
            continue
        if token == b"{":
            starts.append(len(tokens) + 1)
        elif token == b"}" and starts:
            start = starts.pop()
            count = len(tokens) - start
            if count in _SIZES:
                digest = hashlib.sha256(b"\0".join(tokens[start:])).hexdigest()
                row = _CATALOG.get(digest)
                if row:
                    matches.add(row[1])
            if not starts:
                tokens.clear()
                continue
        if starts:
            tokens.append(token)


def check_translation(name: str, data: bytes) -> list[str]:
    """Report verified complete translated bodies, regardless of the member's name."""
    if b"{" not in data or b"}" not in data or b"\0" in data:
        return []
    matches = set()
    _scan(re.sub(rb"\\\r?\n", b"", data), matches)
    return [f"known translated source body: {label}" for label in sorted(matches)]
