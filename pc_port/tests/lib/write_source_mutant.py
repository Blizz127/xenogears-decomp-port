"""Write one intentionally faulty private test copy of a provider source file."""
from pathlib import Path
import sys

source, destination, original, replacement = sys.argv[1:]
body = Path(source).read_text()
if body.count(original) != 1:
    raise SystemExit(f"Expected exactly one mutation site in {source}: {original!r}")
Path(destination).write_text(body.replace(original, replacement))
