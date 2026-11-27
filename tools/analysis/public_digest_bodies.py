"""Detect complete known C body variants while allowing independent implementations."""

import hashlib
import re


# Literals are single tokens so braces and comment delimiters in strings are inert.
_TOKEN = re.compile(
    rb'//[^\n]*|/\*.*?(?:\*/|\Z)|"(?:\\.|[^"\\])*"|'
    rb"'(?:\\.|[^'\\])*'|[a-zA-Z_]\w*|0[xX][\da-fA-F]+[uUlL]*|\d+[uUlL]*|"
    rb'>>=|<<=|>>|<<|==|!=|<=|>=|\+\+|--|\+=|-=|->|&&|\|\||[^\s]',
    re.DOTALL | re.ASCII,
)

# Each key gives the token count and SHA-256 of a complete normalized body.
_BODY_DIGESTS = {
    12: {
        'f9de3553bb6218f4d6b557c49534d80c1d3b2118a87b36a873575d1c17389dfc': 'StUnSetRing',
    },
    14: {
        '22f1295ad65e7bf3324c3d4a7a0054d987623bc1a9f855515175f137c889ff4e': 'StSetRing',
        'd260eef1798002a3e16d38e9b3811551422c51af5063cb2a4938db440e52f4d9': 'StSetMask',
    },
    33: {
        'f581f50ae8a6a923b9baee4acb83cc49ff53d5c9833071019dbf5115143d9543': 'DecDCTvlcSize',
    },
    34: {
        '38f4e9e407da618a0ee2874be73d72d9ac08243502684b47b72cbf52f6f8a36a': 'StClearSlots',
    },
    35: {
        'ed7fc0fcfd383f6b00e95b44fcaec4998178b5fba8bd691627a347910776aa05': 'StClearSlots',
    },
    37: {
        '2badf042462243c8228844549185a151099d506ac4046a60728a5fb34dd1bc8d': 'StClearRing',
    },
    40: {
        'dbb46f3d31e22e1e8399fb064f0a02144e6dec1d4c99503dcfec6bde91c64d2c': 'StGetBackloc',
    },
    45: {
        '59198dae1568ddbe95f9bdb0bf9dc50080bb308e14e8d8c62a3193fda9af9b2b': 'StSetStream',
        'ddebba63509247915ca2f28819664bf9f0d4c907d2e75a41002bb476414e0ae6': 'StSetStream',
    },
    60: {
        '896240364c077f8c297e275f0ebadfc79fcc5247532e18198f1d0ad80a8382a3': 'StStartRead',
    },
    68: {
        '9bc4d17132d6c3e596f6a1223fe5261c91d12770f3d76a73aa9405b7c1af9245': 'StFrameReady',
    },
    69: {
        'e21b1d02539f34a5b56e13d4fd463331da9200631bddfc0d6ac1d9948c0ed80a': 'StFrameReady',
    },
    84: {
        'cf4f7b49759c4a8a32665380167ac3362b36c56f4ac5dbca93db18fe00935d3a': 'StGetNext',
    },
    88: {
        '4421e8eb4b9d180ff4743488c356d2c5d61a1281cc4df4e70b764d80fef1875f': 'StGetNext',
    },
    102: {
        '9b8b1d23c8541b9461f446d8c55599359b181154edd10618f114dce2a0a6dbfd': 'StFreeRing',
    },
    106: {
        '3fbc237122784d337f79435aa76d34c32ea0cbcbed6b215e47cfa19912dc090e': 'StFreeRing',
    },
    146: {
        'f66add26d17fc09dbaa365065a6cd2e0ea0869618f23c14170de8d118bef9364': 'StStartRead',
    },
    197: {
        'b6fa0f6ac9ac658de8549e2242a9634022600b95428ecb35b3e0e2f9d48190f9': 'StStartRead',
    },
    254: {
        'cd7389e8d37c22704d670c72541dbae9a42c4972a8a1da532ef9667d145ee22e': 'func_800AAC08',
    },
    260: {
        'af76d9ace8de337a4edcbeeae0e8e9dadf3db773736bf2df995fe465755fafbf': 'func_800AB378',
    },
    264: {
        '1e01bced7913892cbc6a7b0913e81e353cfb0685b17eb6b3df892c8cd0755d4f': 'func_800AB378',
    },
    550: {
        'bd8a5cec02d32e05b7b60dbb9163a82c64e477c4c876518119559b4208b9ea4f': 'func_800AAF80',
    },
    576: {
        '1168014853b9a88b3927df2b31f24702580d6a27f78d6aa71a7f9450f5d35d9f': 'StCdInterrupt',
    },
    600: {
        'b8dc136e681d8c001796cc5f2147ecab4b5b924ab7999ae3dea3e61f97b82bf3': 'StCdInterrupt',
    },
    619: {
        '8007c775f0b0ba23166e5014f49ea7f13eb9bc527ae7350587882e44a364993f': 'StCdInterrupt',
    },
    663: {
        '2494e1e60f8f6ea2608af587f76e874a122d2c7aa74d923014f88324e9957703': 'StCdInterrupt',
    },
    703: {
        '2cf5b6b42870cdf73c3616e4ce115d0c25c67dbb69b73eb0b92206b8bd1795ec': 'StCdInterrupt',
    },
    779: {
        '16fe1a9dcaef473f6dfd1493314773f4bcadf34d8598e64ce7a1b20a3b5ddb58': 'StCdInterrupt',
    },
    1000: {
        'f04cfc6ca38073e4e8f9a7273642155701b72d0f76bc7a45e6635fe06d4ccfd3': 'DecDCTvlc',
    },
    1049: {
        'b766c7829b2202af8ae1fbaabe015be00f5c4abaaa72cebf72c12c2a9de8b7fc': 'StCdInterrupt',
    },
}
_MACRO_DIGESTS = {
    42: {
        '947b6e4929540ba82e8f11c79aaf62f1ecb04f6fc1cb5b6452a4a36c7daa8227': 'VLC_REFILL',
    },
}


def _fingerprint(tokens):
    return hashlib.sha256(b'\x1f'.join(tokens)).hexdigest()


def _check_tokens(tokens, matches):
    openings = []
    for end, token in enumerate(tokens, 1):
        if token == b'{':
            openings.append(end - 1)
        elif token == b'}' and openings:
            start = openings.pop()
            variants = _BODY_DIGESTS.get(end - start)
            if variants:
                label = variants.get(_fingerprint(tokens[start:end]))
                if label:
                    matches.add(label)
        elif token == b'do':
            for count, variants in _MACRO_DIGESTS.items():
                start = end - 1
                stop = start + count
                if stop <= len(tokens) and tokens[stop - 1] == b')':
                    label = variants.get(_fingerprint(tokens[start:stop]))
                    if label:
                        matches.add(label)


def _check_text(data, matches, include_comments=True):
    tokens = []
    for item in _TOKEN.finditer(data):
        value = item.group()
        if value.startswith((b'/*', b'//')):
            if include_comments and value.startswith(b'/*') and b'{' in value:
                _check_text(value[2:-2] if value.endswith(b'*/') else value[2:],
                            matches, include_comments=False)
            continue
        tokens.append(value)
    _check_tokens(tokens, matches)


def check_bodies(path: str, data: bytes) -> list[str]:
    """Report known field, VLC and stream bodies anywhere in a textual member."""
    if b'{' not in data or b'}' not in data or b'\x00' in data:
        return []
    logical = re.sub(rb'\\\r?\n', b'', data)
    matches = set()
    _check_text(logical, matches)
    return [f'known source body: {name}' for name in sorted(matches)]
