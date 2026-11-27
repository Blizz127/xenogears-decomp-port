#!/usr/bin/env python3
"""Release-gate regressions with tiny synthetic ELF files; no retail data.

Run with the host C compiler and binutils. No game build or game run occurs.
"""
from __future__ import annotations

import contextlib
import io
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest import mock

import psyq_classify
import psyq_port_guard as guard


class ReleaseGuardTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.tmp = tempfile.TemporaryDirectory(prefix="xeno-sdk-guard-")
        cls.root = Path(cls.tmp.name)
        cls.tree = cls.root / "empty-port"
        cls.tree.mkdir()
        cls.allow = cls.root / "allowlist.txt"
        cls.allow.write_text("# Empty release allowlist\n")
        cls.source = cls.root / "fixture.c"
        cls.source.write_text('const char *source_path = __FILE__;\n'
                              'int main(void) { return source_path[0] == 0; }\n')
        cls.raw = cls.root / "raw"
        cls.mapped = cls.root / "mapped"
        cls.stripped = cls.root / "stripped"
        cls.plain = cls.root / "plain"
        prefix = f"-ffile-prefix-map={cls.root}=."
        cc = guard.tool("cc", "gcc", "clang")
        for target, flags in ((cls.raw, ["-g"]),
                              (cls.mapped, ["-g", prefix]),
                              (cls.plain, [prefix])):
            subprocess.run([cc, *flags, str(cls.source), "-o", str(target)], check=True)
        subprocess.run([guard.tool("strip", "llvm-strip"), "--strip-debug",
                        "-o", str(cls.stripped), str(cls.mapped)], check=True)
        cls.classification = psyq_classify.Classification()

    @classmethod
    def tearDownClass(cls):
        cls.tmp.cleanup()

    def run_guard(self, *args):
        output = io.StringIO()
        with contextlib.redirect_stdout(output), contextlib.redirect_stderr(output):
            result = guard.run(["--tree", str(self.tree), "--allowlist", str(self.allow),
                                "--no-sources", "--strict", *map(str, args)])
        return result, output.getvalue()

    def test_missing_retail_executable_fails(self):
        code, _ = self.run_guard("--require-exe", "--exe", self.root / "missing")
        self.assertEqual(code, 2)

    def test_wrong_retail_executable_fails(self):
        code, output = self.run_guard("--require-exe", "--exe", self.source)
        self.assertEqual(code, 2)
        self.assertNotIn("PASS", output)

    def test_conflicting_disc_options_fail(self):
        with self.assertRaises(SystemExit) as error:
            self.run_guard("--require-exe", "--no-disc")
        self.assertEqual(error.exception.code, 2)

    def test_dwarf_requirement_needs_a_binary(self):
        with self.assertRaises(SystemExit) as error:
            self.run_guard("--require-dwarf", "--no-disc")
        self.assertEqual(error.exception.code, 2)

    def test_allowlist_cannot_be_rewritten_during_gate(self):
        with self.assertRaises(SystemExit) as error:
            self.run_guard("--write-allowlist", "--no-disc")
        self.assertEqual(error.exception.code, 2)

    def test_empty_allowlist_passes(self):
        code, _ = self.run_guard("--require-empty-allowlist", "--no-disc")
        self.assertEqual(code, 0)

    def test_nonempty_or_malformed_allowlist_fails(self):
        path = self.root / "nonempty.txt"
        for entry in ("table\tratan_tbl\tbin:xeno-port\n", "malformed entry\n"):
            with self.subTest(entry=entry):
                path.write_text(entry)
                code, _ = self.run_guard("--require-empty-allowlist", "--allowlist", path,
                                         "--no-disc")
                self.assertEqual(code, 1)

    def test_missing_allowlist_fails(self):
        code, _ = self.run_guard("--require-empty-allowlist", "--no-disc", "--allowlist",
                                 self.root / "missing")
        self.assertEqual(code, 2)

    def test_debug_build_passes_and_reports_skipped_tables(self):
        code, output = self.run_guard("--no-disc", "--require-dwarf", "--binary", self.mapped)
        self.assertEqual(code, 0, output)
        self.assertIn("table scan SKIPPED", output)
        self.assertNotIn("no Psy-Q code or tables", output)

    def test_no_dwarf_fails(self):
        for binary in (self.plain, self.stripped):
            with self.subTest(binary=binary):
                code, output = self.run_guard("--no-disc", "--require-dwarf", "--binary", binary)
                self.assertEqual(code, 1, output)
                self.assertIn("nodwarf", output)

    def test_packaged_debug_stripped_copy_passes(self):
        code, output = self.run_guard("--no-disc", "--binary", self.stripped)
        self.assertEqual(code, 0, output)

    def test_absolute_source_path_is_caught(self):
        findings = guard.check_paths_binary(self.raw)
        self.assertTrue(any(f[0] == "path" and str(self.source) in f[1] for f in findings))
        self.assertEqual(guard.check_paths_binary(self.mapped), [])
        self.assertEqual(guard.check_paths_binary(self.stripped), [])

    def test_distinct_long_source_paths_are_reported_in_full(self):
        directory = self.root / ("source-" * 24)
        directory.mkdir()
        first = directory / "first.c"
        second = directory / "second.c"
        first.write_text("const char first_path[] = __FILE__;\n"
                         "extern const char second_path[];\n"
                         "int main(void) { return first_path[0] != second_path[0]; }\n")
        second.write_text("const char second_path[] = __FILE__;\n")
        binary = self.root / "two-paths"
        subprocess.run([guard.tool("cc", "gcc", "clang"), str(first), str(second),
                        "-o", str(binary)], check=True)

        paths = {row[1] for row in guard.check_paths_binary(binary) if row[0] == "path"}
        code, output = self.run_guard("--no-disc", "--binary", binary)
        self.assertEqual(code, 1)
        for source in (first, second):
            self.assertIn(str(source), paths)
            self.assertIn(str(source), output)

    def test_invalid_binary_fails_inspection(self):
        with self.assertRaises(SystemExit) as error:
            self.run_guard("--no-disc", "--binary", self.source)
        self.assertEqual(error.exception.code, 2)

    def test_failed_inspector_is_not_a_clean_scan(self):
        failed = subprocess.CompletedProcess(["readelf"], 1, "", "synthetic inspector failure")
        with mock.patch.object(guard.subprocess, "run", return_value=failed):
            with self.assertRaises(SystemExit) as error:
                self.run_guard("--no-disc", "--binary", self.mapped)
        self.assertEqual(error.exception.code, 2)

    def test_psyq_translation_unit_is_caught(self):
        source = self.root / "src/slus_006.64/psyq/release_fixture.c"
        source.parent.mkdir(parents=True)
        source.write_text("int main(void) { return 0; }\n")
        binary = self.root / "sdk-tu"
        subprocess.run([guard.tool("cc", "gcc", "clang"), "-g", str(source), "-o", str(binary)],
                       check=True)
        findings = guard.check_binary(binary, self.classification, require_dwarf=True)
        self.assertTrue(any(f[0] == "tu" for f in findings), findings)

    def test_reviewed_fragment_needs_its_full_path_and_exact_contents(self):
        tree = self.root / "source-review/pc_port"
        relative = Path("src/cleanroom/sdk_cd_transfer_status.inc")
        source = tree / relative
        source.parent.mkdir(parents=True)
        original = (guard.ROOT / "pc_port" / relative).read_bytes()
        source.write_bytes(original)
        self.assertEqual(guard.check_sources(tree, self.classification), [])

        source.write_bytes(original + b"\n")
        findings = guard.check_sources(tree, self.classification)
        self.assertEqual({row[:2] for row in findings},
                         {("provider", "sha256"), ("provider", "CdDataSync")})

        source.write_bytes(original)
        legacy = tree / "src/psyq_compat.c"
        legacy.write_bytes(original + b"\nvoid ClearImage(void) {}\n")
        findings = guard.check_sources(tree, self.classification)
        self.assertEqual({row[:3] for row in findings}, {
            ("provider", "CdDataSync", "pc_port/src/psyq_compat.c"),
            ("provider", "ClearImage", "pc_port/src/psyq_compat.c"),
        })
        code, output = self.run_guard("--tree", tree, "--no-disc", "--binary", self.mapped)
        self.assertEqual(code, 1, output)
        self.assertIn("CdDataSync", output)

    def test_symlink_and_nested_matching_suffix_cannot_qualify_a_provider(self):
        tree = self.root / "aliases/pc_port"
        source = tree / "src/cleanroom/host_vsync.inc"
        source.parent.mkdir(parents=True)
        nested = tree / "src/nested/pc_port/src/cleanroom/host_vsync.inc"
        nested.parent.mkdir(parents=True)
        nested.write_bytes((guard.ROOT / "pc_port/src/cleanroom/host_vsync.inc").read_bytes())
        source.symlink_to(nested)
        findings = {row[:3] for row in guard.check_sources(tree, self.classification)}
        self.assertIn(("provider", "sha256", "pc_port/src/cleanroom/host_vsync.inc"), findings)
        self.assertIn(("provider", "Vsync",
                       "pc_port/src/nested/pc_port/src/cleanroom/host_vsync.inc"), findings)

    def test_address_requires_both_the_fragment_and_its_normal_source_unit(self):
        tree = self.root / "controller-review/pc_port"
        fragment = tree / "src/cleanroom/sdk_pad_shutdown.inc"
        fragment.parent.mkdir(parents=True)
        fragment.write_bytes((guard.ROOT / "pc_port/src/cleanroom" / fragment.name).read_bytes())
        unit = tree / "src/psyq_compat.c"
        source = ('void PadStopCom(void) {}\n#include "cleanroom/sdk_pad_shutdown.inc"\n'
                  'int main(void) { return 0; }\n')
        unit.write_text(source)
        binary = self.root / "controller-entry"
        subprocess.run([guard.tool("cc"), "-g", str(unit), "-o", str(binary)], check=True)
        options = ("--no-disc", "--require-dwarf", "--binary", binary)
        code, output = self.run_guard("--tree", tree, *options)
        self.assertEqual(code, 0, output)
        self.assertEqual(guard.COMPAT_ADDRS, {0x800408F4})

        unit.unlink()
        for subset in (tree, self.tree):
            with self.subTest(subset=subset):
                code, output = self.run_guard("--tree", subset, *options)
                self.assertEqual(code, 1, output)
                self.assertIn("bfunc\t0x800408F4", output)
                self.assertEqual(guard.COMPAT_ADDRS, set())

    def test_missing_provider_is_a_full_tree_finding(self):
        tree = self.root / "missing-provider/pc_port"
        tree.mkdir(parents=True)
        with mock.patch.object(guard, "ROOT", tree.parent):
            rows = guard.check_sources(tree, self.classification)
        missing = {row[2] for row in rows if row[:2] == ("provider", "missing")}
        self.assertEqual(missing, set(guard.REVIEWED_PROVIDERS))

    def test_strict_mode_does_not_exempt_psycross_table(self):
        synthetic = bytes(range(128))
        binary = self.root / "table-fixture"
        binary.write_bytes(synthetic)
        with mock.patch.object(guard, "psycross_symbols", return_value={"SQRT"}):
            with mock.patch.object(guard, "symbol_map", return_value=[(0, 128, "SQRT")]):
                self.assertEqual(guard.check_tables_binary(binary, [("sqrt_tbl", synthetic)]), [])
                strict = guard.check_tables_binary(binary, [("sqrt_tbl", synthetic)], strict=True)
                self.assertEqual([f[1] for f in strict], ["sqrt_tbl"])

    def test_arctangent_range_is_classified(self):
        self.assertIn(("ratan_tbl", 0x80057030, 0x80057832, "libgte"),
                      psyq_classify.load_data_ranges())


if __name__ == "__main__":
    unittest.main()
