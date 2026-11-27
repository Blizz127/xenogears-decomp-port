"""Check the source selection used by the release regression scan."""

from io import BytesIO
from pathlib import Path
import os
import subprocess
import tempfile
import tarfile
import unittest
import warnings
import zipfile

from public_guard_archives import archive_inputs
from public_source_guard import index_entries, object_inputs, tree_entries, worktree_inputs


class SourceSelectionTest(unittest.TestCase):
    def setUp(self):
        directory = tempfile.TemporaryDirectory()
        self.addCleanup(directory.cleanup)
        self.root = Path(directory.name).resolve()
        self.git("init", "-q")
        self.git("config", "user.name", "Guard Test")
        self.git("config", "user.email", "test@example.invalid")
        self.name = "sample bytes"
        (self.root / self.name).write_bytes(b"initial")
        self.git("add", self.name)
        self.git("commit", "-qm", "initial")
        (self.root / self.name).write_bytes(b"indexed")
        self.git("add", self.name)
        (self.root / self.name).write_bytes(b"working")

    def git(self, *args):
        return subprocess.check_output(["git", "-C", self.root, *args])

    def test_reads_selected_version(self):
        self.assertEqual(dict(worktree_inputs(self.root, index_entries(self.root))),
                         {self.name: b"working"})
        self.assertEqual(dict(object_inputs(self.root, index_entries(self.root))),
                         {self.name: b"indexed"})
        self.assertEqual(dict(object_inputs(self.root, tree_entries(self.root, "HEAD"))),
                         {self.name: b"initial"})

    @unittest.skipUnless(hasattr(os, "symlink"), "requires symlinks")
    def test_link_target_is_scanned_as_text(self):
        link = self.root / "reference"
        try:
            link.symlink_to(self.name)
        except OSError as error:
            self.skipTest(str(error))
        self.git("add", "reference")
        self.assertEqual(dict(worktree_inputs(self.root, index_entries(self.root)))["reference"],
                         os.fsencode(self.name))
        self.assertEqual(dict(object_inputs(self.root, index_entries(self.root)))["reference"],
                         os.fsencode(self.name))


class ArchiveInputTest(unittest.TestCase):
    def test_every_physical_member_is_read(self):
        outer = BytesIO()
        inner = BytesIO()
        with tarfile.open(fileobj=inner, mode="w:gz") as archive:
            item = tarfile.TarInfo("source.c")
            item.size = 4
            archive.addfile(item, BytesIO(b"body"))
        with warnings.catch_warnings(), zipfile.ZipFile(outer, "w", zipfile.ZIP_DEFLATED) as archive:
            warnings.simplefilter("ignore", UserWarning)
            archive.writestr("same", b"first")
            archive.writestr("same", b"last")
            archive.writestr("source.tar.gz", inner.getvalue())
        rows = list(archive_inputs("release.zip", outer.getvalue()))
        self.assertEqual([v for k, v in rows if k == "release.zip!same"],
                         [b"first", b"last"])
        self.assertIn(("release.zip!source.tar.gz!source.c", b"body"), rows)

    def test_invalid_archive_fails(self):
        with self.assertRaises(zipfile.BadZipFile):
            list(archive_inputs("broken.zip", b"incomplete"))


if __name__ == "__main__":
    unittest.main()
