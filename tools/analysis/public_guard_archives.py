"""Read members of common distribution archives in memory, including repeated names."""

from io import BytesIO
import tarfile
import zipfile


def archive_inputs(name, data):
    remaining_bytes = 256 * 1024 * 1024
    remaining_members = 100_000

    def take(path, size):
        nonlocal remaining_bytes, remaining_members
        remaining_members -= 1
        remaining_bytes -= size
        if size < 0 or remaining_bytes < 0 or remaining_members < 0:
            raise ValueError(f"archive exceeds scan limits at {path!r}")

    def visit(path, blob, depth):
        yield path, blob
        lower = path.lower()
        is_zip = blob.startswith((b"PK\x03\x04", b"PK\x05\x06")) or lower.endswith(".zip")
        is_tar = blob[257:262] == b"ustar" or lower.endswith(
            (".tar", ".tgz", ".tar.gz", ".tbz2", ".tar.bz2", ".txz", ".tar.xz"))
        if not (is_zip or is_tar):
            return
        if depth >= 8:
            raise ValueError(f"archive nesting exceeds scan limit at {path!r}")
        if is_zip:
            with zipfile.ZipFile(BytesIO(blob)) as archive:
                for member in archive.infolist():
                    child = path + "!" + member.filename
                    take(child, member.file_size)
                    if member.is_dir():
                        yield child, b""
                    else:
                        yield from visit(child, archive.read(member), depth + 1)
        else:
            with tarfile.open(fileobj=BytesIO(blob), mode="r|*") as archive:
                for member in archive:
                    child = path + "!" + member.name
                    take(child, member.size)
                    if member.isfile():
                        with archive.extractfile(member) as source:
                            content = source.read()
                        if len(content) != member.size:
                            raise ValueError(f"incomplete archive member {child!r}")
                        yield from visit(child, content, depth + 1)
                    elif member.issym() or member.islnk():
                        yield child, member.linkname.encode("utf-8", "surrogateescape")
                    else:
                        yield child, b""

    yield from visit(name, data, 0)
