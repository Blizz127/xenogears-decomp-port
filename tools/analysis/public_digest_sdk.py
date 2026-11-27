"""Recognize known SDK headers and reviewed partial copies by content digest.

Whole-file checks ignore filenames. Partial checks cover selected token
sequences; rewritten implementations and fragments outside that set need
separate provenance review.
"""
from __future__ import annotations

from collections import deque
from functools import lru_cache
import hashlib
import json
from pathlib import Path
import re


# Lexical units are length-independent and keep quoted content intact. Comments
# and formatting outside quoted strings do not affect a declaration fingerprint.
_TOKEN = re.compile(
    rb"""//[^\r\n]*|/\*[\s\S]*?(?:\*/|\Z)|"(?:\\[\s\S]|[^"\\])*"|'(?:\\[\s\S]|[^'\\])*'|[A-Za-z_][A-Za-z_0-9]*|[0-9][A-Za-z_0-9]*|[^\s]"""
)
_SPACE = b" \t\r\n\v\f"


def _tokens(data: bytes):
    for match in _TOKEN.finditer(data):
        value = match[0]
        if not value.startswith((b"//", b"/*")):
            yield value


def _hash(value: bytes) -> str:
    return hashlib.sha256(value).hexdigest()


@lru_cache(maxsize=8192)
def _pivot(value: bytes) -> str:
    return _hash(value)


def _load_signatures():
    with Path(__file__).with_suffix(".json").open(encoding="ascii") as source:
        table = json.load(source)
    exact = set(table["complete"])
    compact = set(table["compact"])
    pivots = {}
    for size, marker, offset, extent, whole in table["embedded"]:
        pivots.setdefault(size, {}).setdefault(marker, []).append((offset, extent, whole))
    endings = {}
    lengths = set()
    max_window = 0
    for pivot_size, pivot_hash, width, digest, group in table["windows"]:
        endings.setdefault(pivot_hash, []).append((width, digest, group))
        lengths.add(pivot_size)
        max_window = max(max_window, width)
    return exact, compact, pivots, endings, lengths, max_window


_EXACT, _COMPACT, _LINE_PIVOTS, _ENDINGS, _LENGTHS, _MAX_WINDOW = _load_signatures()


def _contains_header(data: bytes, compact: bytes) -> bool:
    # Locate a complete header by a line inside it. Confirm all its normalized
    # bytes, even if unrelated content surrounds it in the same input.
    position = 0
    for line in data.splitlines():
        trimmed = line.translate(None, _SPACE)
        by_hash = _LINE_PIVOTS.get(len(trimmed))
        if by_hash:
            for offset, extent, wanted in by_hash.get(_hash(trimmed), ()):
                start = position - offset
                if start >= 0 and start + extent <= len(compact):
                    if _hash(compact[start:start + extent]) == wanted:
                        return True
        position += len(trimmed)
    return False


def _has_fragment(data: bytes) -> bool:
    recent = deque(maxlen=_MAX_WINDOW)
    first_match = {}
    for value in _tokens(data):
        recent.append(value)
        if len(value) not in _LENGTHS or not (value[:1].isalpha() or value.startswith(b"_")):
            continue
        patterns = _ENDINGS.get(_pivot(value))
        if not patterns:
            continue
        tokens = tuple(recent)
        for width, wanted, group in patterns:
            if len(tokens) < width or _hash(b"\x00".join(tokens[-width:])) != wanted:
                continue
            # A reviewed partial-copy signature is conclusive by itself.
            # Other headers require two distinct, disjoint reference windows.
            if group == 0 or group in first_match and first_match[group] != wanted:
                return True
            first_match[group] = wanted
    return False


def check_sdk(path: str, data: bytes) -> list[str]:
    """Check a source or decoded member. Its filename does not affect the result."""
    if _hash(data) in _EXACT:
        return ["sdk-complete-header"]
    compact = data.translate(None, _SPACE)
    if _hash(compact) in _COMPACT:
        return ["sdk-whitespace-normalized-header"]
    if _contains_header(data, compact):
        return ["sdk-embedded-header"]
    if _has_fragment(data):
        return ["sdk-declaration-fragment"]
    return []
