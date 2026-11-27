"""Match withdrawn World sources and verify references to these units in release manifests."""
import hashlib
import json
from pathlib import Path, PurePosixPath
import re

from public_digest_world_origin import _TOKEN

CATALOG = json.loads(Path(__file__).with_suffix(".json").read_bytes())
FILES = CATALOG["files"]
BODIES = CATALOG["bodies"]
WITHDRAWN = {digest: path for path, versions in FILES.items() for _, digest in versions}
UNIT_TOKENS = CATALOG["unit_tokens"]
SIZES = {size for versions in FILES.values() for size, _ in versions}
OBJECTS = {("src_world_map_main.c.o" if path == "src/world_map/main.c"
            else PurePosixPath(path).name + ".o") for path in FILES if path.endswith(".c")}

SHELL = "pc_port/build_port.sh"
LINUX = "pc_port/tools/public_linux/linux-build.json"
WINDOWS = "pc_port/tools/win64/compile-profile.json"
TRACKED = re.compile(r"^(?P<scope>.*[/!])?(?P<path>src/world_map/main\.c|pc_port/(?:src/[^/!]+|"
                     r"build_port\.sh|tools/public_linux/linux-build\.json|tools/win64/compile-profile\.json))$")
PORT_ARRAY = re.compile(rb"(?m)^PORT_SOURCES=\(\s*\n(.*?)^\)", re.S)


def file_reason(name, data):
    """The exact withdrawn snapshot is identifiable after movement or renaming."""
    if len(data) in SIZES:
        path = WITHDRAWN.get(hashlib.sha256(data).hexdigest())
        if path:
            return ["withdrawn World release source snapshot: " + path]
    return token_reason(data)


def token_reason(data):
    """Ignore commentary and spacing on substantial, formerly shipped C units."""
    if b"\0" in data or len(data) < 120 or b"{" not in data or b";" not in data:
        return []
    folded = data.lower()
    if not any(marker in folded for marker in (b"wm_", b"world_map", b"worldmap")):
        return []
    tokens = (m[0].encode() for m in _TOKEN.finditer(data.decode(errors="replace"))
              if not m[0].startswith(("/*", "//")))
    path = UNIT_TOKENS.get(hashlib.sha256(b"\0".join(tokens)).hexdigest())
    return ["withdrawn World release translation unit: " + path] if path else []


class ReleaseRosters:
    """Keep a source tree or each archive prefix independent of other inputs."""
    def __init__(self):
        self.units = {}
        self.expected = {}
        self.errors = []

    def visit(self, name, data):
        match = TRACKED.fullmatch(name)
        if not match:
            return
        path = match["path"]
        scope = match["scope"] or ""
        if path in FILES:
            self.units.setdefault(scope, set()).add(path)
            return
        if path not in (SHELL, LINUX, WINDOWS):
            return
        if path == SHELL:
            match = PORT_ARRAY.search(data)
            if not match:
                self.errors.append((name, "cannot read PORT_SOURCES release array"))
                return
            # The release builder uses one literal path per item in this array.
            units = {word.decode("utf-8") for line in match[1].splitlines()
                     for word in line.partition(b"#")[0].strip().split()}
        else:
            try:
                profile = json.loads(data)
                if path == LINUX:
                    units = {unit["source"] for unit in profile["units"].values()}
                    links = {value.removeprefix("@obj@/") for value in profile["link"]}
                    stale = (links & OBJECTS) - set(profile["units"])
                    self.errors.extend((name, "unbound World link object: " + obj) for obj in sorted(stale))
                else:
                    units = {source for _, source, _ in profile["units"]}
            except (ValueError, TypeError, AttributeError, KeyError) as error:
                self.errors.append((name, "cannot read World-source release roster: " + str(error)))
                return
        self.expected[(scope, name)] = units.intersection(FILES)

    def findings(self):
        result = list(self.errors)
        for (scope, name), required in self.expected.items():
            missing = required - self.units.get(scope, set())
            result.extend((name, "unresolved World source in release roster: " + path)
                          for path in sorted(missing))
        return sorted(result)
