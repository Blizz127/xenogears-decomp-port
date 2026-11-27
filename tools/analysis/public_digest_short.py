"""Detect seven confirmed short restricted spans without distributing reference data.

Scan each tracked file or unpacked member. Only complete SHA-256 matches are
findings. A structural prefix and a CRC32 avoid hashing every sliding window.
"""
import base64
import binascii
import hashlib
import re
import zlib


# CRC/SHA signatures of the seven complete spans.
_SIGNATURES = {
    16: {0x5c33cff3: "47773c1d0ec894732fa108868d6ff3db76a6bcb74ce104902a402c9fc3f2cbc8"},
    17: {
        0x99c73766: "eb5dd2bcd4785884a399e181f298d43f9f0a3679cb5614d8c70959e73a8b74d4",
        0xb935f770: "6451b22e6f34c1ba2e98f20067120ab9aae75ccfd60b84436bb5bae09f8d9076",
        0xf67461b7: "d68f17ba25a0de25a513be82f59bddea4d31ad29486651df34240f3f2298de36",
    },
    18: {
        0xadd1fd30: "c7cd5d0fbc822ccea6951f33b76c1c8a60a2f46ad38d42b1ec4b880b6c529d4e",
        0x8d233d26: "c14a6b174331ed452c9f7c064aee635e6356ea6402ab38d24d3ca579fbee8890",
        0xc262abe1: "7d0ff259c6e4006abef103b08ec67212b77716a49d680103a79f89e4bfc3aaab",
    },
}
_PREFIX = re.compile(rb"(?=[a-zA-Z]:[\\/]|[A-Z]{3,8}-[0-9])")


def _hits(data):
    if len(data) < 16 or (b":" not in data and b"-" not in data):
        return
    for candidate in _PREFIX.finditer(data):
        pos = candidate.start()
        for size, by_crc in _SIGNATURES.items():
            span = data[pos:pos + size]
            if len(span) == size:
                expected = by_crc.get(zlib.crc32(span))
                if expected and hashlib.sha256(span).hexdigest() == expected:
                    yield pos, size


_TOKEN = re.compile(rb'''//[^\n]*|/\*[\s\S]*?(?:\*/|\Z)|"(?:[^"\\\n]|\\.)*"|'(?:[^'\\\n]|\\.)*' ''', re.X)
_ESCAPE = re.compile(rb"\\(?:x[0-9a-fA-F]+|u[0-9a-fA-F]{4}|U[0-9a-fA-F]{8}|[0-7]{1,3}|.)")
_CONTINUATION = re.compile(rb"\\(?:\r\n|\n|\r)")
_SIMPLE = dict(zip(b"abfnrtv", (7, 8, 12, 10, 13, 9, 11)))
_WIDTHS = {
    1: rb"(?:char|[us]8|u_char|u?int8_t)",
    2: rb"(?:short(?:\s+int)?|[us]16|u_short|u?int16_t)",
    4: rb"(?:int|[us]32|u_int|u?int32_t)",
    8: rb"(?:long\s+long(?:\s+int)?|[us]64|u?int64_t)",
}
_ARRAY = re.compile(rb"\b(?:" + b"|".join(rb"(?P<w%d>%s)" % p for p in _WIDTHS.items()) +
                    rb")\s+(?:(?:const|volatile)\s+)*\w+\s*\[\s*(?P<count>\d*)\s*\]\s*=\s*"
                    rb"\{(?P<body>[^{}]*)\}")
_ITEM = re.compile(rb'''\s*('(?:[^'\\]|\\.)*'|[+-]?(?:0[xX][\da-fA-F]+|0[bB][01]+|\d+)[uUlL]*)\s*(,|$)''')


def _decode(data):
    def replace(m):
        text = m[0][1:]
        if text.startswith(b"x"):
            if len(text) == 1:
                return m[0]
            value = int(text[1:][-2:], 16)
        elif text[:1] in (b"u", b"U") and len(text) > 1:
            point = int(text[1:], 16)
            if point > 0x10ffff or 0xd800 <= point <= 0xdfff:
                return m[0]
            return chr(point).encode("utf-8")
        elif text[0] in b"01234567":
            value = int(text, 8)
        else:
            value = _SIMPLE.get(text[0], text[0])
        return bytes([value & 255])
    return _ESCAPE.sub(replace, data)


def _array_bytes(match):
    width = next(w for w in _WIDTHS if match["w%d" % w] is not None)
    out = bytearray()
    body = match["body"].strip()
    pos = 0
    while pos < len(body):
        m = _ITEM.match(body, pos)
        if not m:
            return None
        num = m[1]
        if num.startswith(b"'"):
            char = _decode(num[1:-1])
            if len(char) != 1:
                return None
            value = char[0]
        else:
            num = num.rstrip(b"lLuU")
            digits = num.lstrip(b"+-").lower()
            base = 16 if digits.startswith(b"0x") else 2 if digits.startswith(b"0b") else 8 if digits.startswith(b"0") else 10
            try:
                value = int(num, base)
            except ValueError:
                return None
        out.extend((value % (1 << (8 * width))).to_bytes(width, "little"))
        pos = m.end()
    if match["count"]:
        try:
            missing = width * int(match["count"]) - len(out)
        except ValueError:
            return None
        if missing < 0:
            return None
        out.extend(bytes(min(missing, max(_SIGNATURES))))
    return out


def _c_views(raw):
    # Source views report offsets after translation-phase line splicing.
    text = _CONTINUATION.sub(b"", raw)
    code = bytearray(text)
    current = bytearray()
    start = end = 0
    for m in _TOKEN.finditer(text):
        token = m[0]
        if token.startswith(b"/"):
            code[m.start():m.end()] = b" " * len(token)
            if b"\\\\" in token:
                yield m.start(), "comment escape", token.replace(b"\\\\", b"\\")
            continue
        if not token.startswith(b'"'):
            continue
        if current and code[end:m.start()].strip() not in (b"", b"u8"):
            yield start, "C string", bytes(current) + b"\0"
            current.clear()
        if not current:
            start = m.start()
        current.extend(_decode(token[1:-1]))
        end = m.end()
    if current:
        yield start, "C string", bytes(current) + b"\0"
    for m in _ARRAY.finditer(code):
        array = _array_bytes(m)
        if array:
            yield m.start(), "integer initializer", array


_HEX = re.compile(rb"(?<![a-fA-F0-9])[a-fA-F0-9]{32,}")
_BASE64 = re.compile(rb"(?<![A-Za-z0-9+/_-])[A-Za-z0-9+/_-]{22,}={0,2}")
_BYTE = rb"(?:0[xX][0-9a-fA-F]{1,2}|[0-9]{1,3})"
_BYTE_LIST = re.compile(rb"\[\s*((?:" + _BYTE + rb"\s*,\s*){15,}" + _BYTE + rb")\s*,?\s*\]")
_SPACED_HEX = re.compile(rb"(?i)(?:\b[0-9a-f]{2}[ \t\r\n]+){15,}[0-9a-f]{2}\b")


def _serialized(data):
    for m in _BYTE_LIST.finditer(data):
        raw = [x.strip() for x in m[1].split(b",")]
        values = [int(x, 16 if x.lower().startswith(b"0x") else 10) for x in raw]
        if max(values) < 256:
            yield m.start(), "byte list", bytes(values)
    for m in _HEX.finditer(data):
        value = m[0]
        if len(value) % 2 == 0:
            yield m.start(), "hex", binascii.unhexlify(value)
    for m in _SPACED_HEX.finditer(data):
        yield m.start(), "hex bytes", bytes.fromhex(m[0].decode("ascii"))
    for m in _BASE64.finditer(data):
        value = m[0]
        if len(value) % 4 == 1:
            continue
        try:
            decoded = base64.b64decode(value + b"=" * (-len(value) % 4), altchars=b"-_", validate=True)
        except binascii.Error:
            continue
        yield m.start(), "base64", decoded


# Ordinary ASCII stored as UTF-16 in Windows metadata. Limit decoding to
# bounded, structurally plausible windows instead of expanding whole files.
_WIDE = re.compile(rb"(?=(?:[a-zA-Z]\0:\0[\\/]|(?:[A-Z]\0){3,8}-\0[0-9])|"
                   rb"(?:\0[a-zA-Z]\0:\0[\\/]|(?:\0[A-Z]){3,8}\0-\0[0-9]))")


def _wide_views(data):
    for m in _WIDE.finditer(data):
        pos = m.start()
        order = "utf-16-be" if data[pos] == 0 else "utf-16-le"
        window = data[pos:pos + 2 * max(_SIGNATURES)]
        try:
            text = window[:len(window) & ~1].decode(order)
        except UnicodeDecodeError:
            # A random unpaired surrogate after the match cannot hide ASCII.
            text = window[:len(window) & ~1].decode(order, errors="replace")
        yield pos, order, text.encode("utf-8")


def check_short(path: str, data: bytes) -> list[str]:
    """Return non-sensitive evidence for every view containing a confirmed span."""
    messages = []
    def record(view, blob, base):
        for offset, size in _hits(blob):
            messages.append(f"restricted {size}-byte span in {view} at {base:#x}, decoded offset {offset:#x}")
    record("raw", data, 0)
    if b"\0" in data:
        for base, view, blob in _wide_views(data):
            record(view, blob, base)
    if b'"' in data or (b"[" in data and b"{" in data) or b"\\\\" in data:
        for base, view, blob in _c_views(data):
            record(view, blob, base)
    for base, view, blob in _serialized(data):
        record(view, blob, base)
    return messages
