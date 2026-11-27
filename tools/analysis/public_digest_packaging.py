"""Match previously identified distribution markers in source, paths and built files."""

from collections import defaultdict
from hashlib import sha256
import json
import re


# Each record pins a two-byte search anchor and the entire case-folded match.
# The anchor only selects candidates. A full SHA-256 digest decides each result.
_RECORDS = (
    ("0ec1bba5852e5ac2f3f0d5f5a304ff18492acdfc7f82ee8288d49fdf26c2cfe3", 1, 8,
     "0761fd917c2de94ca9871e940ba6368a506ac45955ee6a05b844b08ffc7bed5d", 0, "deployment location"),
    ("0ec1bba5852e5ac2f3f0d5f5a304ff18492acdfc7f82ee8288d49fdf26c2cfe3", 1, 9,
     "9d41da5a5d1714d179013968919c1a4de16fed78589dd0505634c9ae5ec82768", 0, "deployment location"),
    ("0ec1bba5852e5ac2f3f0d5f5a304ff18492acdfc7f82ee8288d49fdf26c2cfe3", 1, 9,
     "be9ef2e918086b990d26db3a14449429f9c223063f9de076578462aed9bf0574", 0, "deployment location"),
    ("0ec1bba5852e5ac2f3f0d5f5a304ff18492acdfc7f82ee8288d49fdf26c2cfe3", 1, 11,
     "507407286c0b3a216778a30ddedfafff07a1b35fb52f2bc935c82d6c804122ad", 0, "deployment location"),
    ("0ec1bba5852e5ac2f3f0d5f5a304ff18492acdfc7f82ee8288d49fdf26c2cfe3", 1, 13,
     "7e49a57dcb42c6d419474b051b6ee79fe25f27fb02a9c9101e6cd0c6dc834d57", 0, "deployment location"),
    ("0ec1bba5852e5ac2f3f0d5f5a304ff18492acdfc7f82ee8288d49fdf26c2cfe3", 1, 13,
     "a0a9d4646b1b342c177f05f4ee40981446f4e0deee4cefe6492efe8af8a62b93", 0, "deployment location"),
    ("361757e6040c0546eedf814ad788411c2156b583f7ad33128d14df19ca66a50e", 11, 13,
     "babc98ed9b79892210a8e745a6506f8234b0bca681af9a47ded5a17e26b6e533", 0, "deployment location"),
    ("361757e6040c0546eedf814ad788411c2156b583f7ad33128d14df19ca66a50e", 12, 14,
     "0775edf4156c46d736c86f9b5f14b864337d0a7dae164879fecd04b21727c0a9", 0, "deployment location"),
    ("4b237bf5b4c1ddd9abaa359dfdf4cbbd86f16b8571427e935f006e7c9291c48c", 16, 21,
     "645bb27455802f4d51798c49694818fb8c3de7dd2c048974d7f9b04e65c1d713", 1, "deployment location"),
    ("4feb06dcaa912f6e448a4b63540aa6055578a7e1a39e2f2902c1c35015f471bd", 8, 17,
     "8eb4a01e7efff71d16251568ff99f19dd574cf31c45542207745e26ab7426260", 0, "content identifier"),
    ("580eb35f5771e749d774f420eb0475b17426d712ad90581944fbaaf7bd81df9e", 6, 25,
     "94f367323ab96d2a2690ba41beb54bc046154711785f47938969337f209edc14", 0, "content identifier"),
    ("5dbbba52ec389f4b87823ab5e07d4d80126d1dc09ff0f7657849e1baf21dd200", 10, 12,
     "99a4122c7dde1d06fb2eb6da3b52e54d41c00410fcf0db1c5617369105b13b65", 0, "deployment location"),
    ("5dbbba52ec389f4b87823ab5e07d4d80126d1dc09ff0f7657849e1baf21dd200", 11, 13,
     "7c12655c17594ec0c1aaa51d728f0ce375845d91b1dfe67bbaf974bdbac7e09d", 0, "deployment location"),
    ("6bc54f13bea593dc5e1c8e6b16249e669d1ce99cbeac65f2a10f36e1d86f1fd4", 1, 6,
     "9d651e88f0f69370f8e14d981bac83eed81c4ee8fb937256033e9100fa7b09d6", 1, "deployment location"),
    ("87240a13b2c5093654477177905699bd76546f2bb102756c5a4f2f6eadd2c42e", 4, 7,
     "35314a3c37d0d6da1e41a92ded9f58e1338f4a74b4f858783c874dc59fd50466", 0, "content identifier"),
    ("89c4ec9f6b3f1086b158d8ef03dfe8155e6f79d9e66434b8f9b3432fe8720e50", 3, 7,
     "c5c767165ca51f2522eed0ee579cffe84b6b16971fe6a1d545b66f57532c70ad", 0, "content identifier"),
    ("8bcd6f949d2d8315e14cb9e4a12cfb9f8b8dcfa8a1a13723acd38d84bdf20687", 10, 15,
     "3b5c202991c014fc96746eb85c071c9d8208bd922a8190ab34c12a9f94f82490", 0, "packaging metadata"),
    ("8bcd6f949d2d8315e14cb9e4a12cfb9f8b8dcfa8a1a13723acd38d84bdf20687", 13, 18,
     "559a6acebb0666297f1eb97e2d971d01e34270bdf0db0d589668c7824b81aaba", 0, "packaging metadata"),
    ("8bcd6f949d2d8315e14cb9e4a12cfb9f8b8dcfa8a1a13723acd38d84bdf20687", 13, 18,
     "b380ad71146a53a0a28e289767a4d0d812590a1bcb1b56c4a1e903cbf7dc494b", 0, "packaging metadata"),
    ("8bcd6f949d2d8315e14cb9e4a12cfb9f8b8dcfa8a1a13723acd38d84bdf20687", 15, 20,
     "351d1dbf5351d3a34bf3cdc1d796c6b1c86679ca39e0d54bf9160abf9fe1a6c7", 0, "packaging metadata"),
    ("8bcd6f949d2d8315e14cb9e4a12cfb9f8b8dcfa8a1a13723acd38d84bdf20687", 16, 21,
     "be84a6bacd29dd28b806650de6b9b528f2fdf26cebccd57be695f5bf50d81e0f", 0, "packaging metadata"),
    ("c274891790345c56cef3b53c026bdc48150948fa60c56306073d6fea7766ad6a", 16, 19,
     "f2f8a529a2a15bdb117da64007aea75b71c6880d50a96d38ade8b9b3a158f123", 0, "content identifier"),
    ("c8b60efc6045b03bbfde1b0407abc61dd702284d7dbfd4b4243ba5133c49fb82", 5, 16,
     "c294f4892c2f2debb32d389d2eb83a8835cb477d111c625dcd342de6ef45a61a", 0, "content identifier"),
    ("c8b60efc6045b03bbfde1b0407abc61dd702284d7dbfd4b4243ba5133c49fb82", 5, 18,
     "f4b0f7e7b39b7b9105bb8bde98acd7f38a941bd3d5d878726c2c75dc606a745d", 0, "content identifier"),
    ("d486745c2f915598079e05a1b76e88b6a8596149fa15fe5924ce8a55f493fe0f", 10, 13,
     "3a45b80fb43349da16697dbc85b7f47f43c43fc00752fce86b7b74afead65df0", 0, "deployment location"),
    ("ff25ca0d9a6eb4fe1ff85776f027f024ef7397a3c993a252f2e44dda1988458b", 17, 19,
     "b164c253c80a5db9304a93f0c8da89bbfcd81b9edc53329d8ce5d00b7c1c8157", 0, "packaging metadata"),
)


def _index():
    records = defaultdict(list)
    for key, offset, size, digest, boundary, label in _RECORDS:
        records[key].append((offset, size, bytes.fromhex(digest), boundary, label))
    index = {}
    for first in range(32, 127):
        for second in range(32, 127):
            pair = bytes((first, second))
            key = sha256(pair).hexdigest()
            if key in records:
                index[pair] = records[key]
    if len(index) != len(records):
        raise ValueError("unsupported distribution signature anchor")
    return index


_INDEX = _index()
_DOMAIN_CHARS = b"abcdefghijklmnopqrstuvwxyz0123456789-"
_WIDE = tuple(re.compile(pattern) for pattern in
              (rb"(?:[ -~]\x00){5,}", rb"(?:\x00[ -~]){5,}"))


def _scan(data, result):
    text = data.lower().replace(b"\\", b"/")
    for anchor, records in _INDEX.items():
        at = text.find(anchor)
        while at != -1:
            for offset, size, expected, boundary, label in records:
                start = at - offset
                end = start + size
                if start < 0 or end > len(text):
                    continue
                if boundary and end < len(text) and text[end] in _DOMAIN_CHARS:
                    continue
                if sha256(text[start:end]).digest() == expected:
                    result.add(label)
            at = text.find(anchor, at + 1)


# Complete top-level field sets of reviewed historical metadata documents.
# Comparing only the full set keeps ordinary release metadata valid.
_FIELD_SETS = {
    "3541efbb95d91df20ac0c756b46d54a04c6131f824e5a412d47c482b65e90035",
    "412038a9fae59eacee529e7bb0b986ed12bc505df60e79c2d2c3850802ae202a",
    "256f80650b9a92738c482a8f97272a891c122980fcac1c0adac31979128b334b",
    "b21c9eec32ea0463cf73366bf609482056947b0a9dc0f2660c7fce1e05f4d00d",
    "28c1f6a760fd16b6114ae4ee9bc47ae0a17d12e1a264e3ba843059ee824c4871",
}


def _known_fields(data):
    # The JSON parser detects Unicode encodings. Check its possible initial
    # octets before invoking it on arbitrary source or executable bytes.
    if not data.lstrip(b" \t\n\r\x00").startswith((b"{", b"\xef\xbb\xbf", b"\xff\xfe", b"\xfe\xff")):
        return False
    try:
        value = json.loads(data)
    except (ValueError, UnicodeError, RecursionError):
        return False
    if not isinstance(value, dict):
        return False
    fields = sorted(k.lower().encode("utf-8", "surrogatepass") for k in value)
    if any(b"\0" in key for key in fields):
        return False
    return sha256(b"\0".join(fields)).hexdigest() in _FIELD_SETS


def check_packaging(path: str, data: bytes) -> list[str]:
    """Return the categories found without printing the underlying matches."""
    result = set()
    _scan(path.encode("utf-8", "surrogateescape"), result)
    _scan(data, result)
    if b"\x00" in data:
        for align, pattern in enumerate(_WIDE):
            for run in pattern.finditer(data):
                _scan(run[0][align::2], result)
    if _known_fields(data):
        result.add("packaging metadata")
    return sorted(result)
