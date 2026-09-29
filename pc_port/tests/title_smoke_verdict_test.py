"""Exercise the actual shell verdict without starting a game or requiring a disc."""
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

SCRIPT = Path(__file__).with_name("run_title_newgame_smoke.sh")
VERDICT = SCRIPT.read_text().split("# ---- verdict ", 1)[1].split("\n", 1)[1]
CHAIN = [
    "[field-diag] FieldLoad begin field=490",
    "[xeno-port][fe60] enter",
    "[xeno-port][fe60] skip Circle",
    "op=FE57",
    "[xeno-port][menu] func_800799D4 request=2 D_80059460=2 -> MenuMain",
    "[xeno-port][menu] func_801C58EC title loop enter choice=1",
    "[xeno-port][menu] title confirm choice=2",
    "[field-diag] FieldLoad begin field=4",
    "[field-diag] FieldLoad begin field=2",
]


class VerdictTest(unittest.TestCase):
    def test_extended_pulse_interval(self):
        source = SCRIPT.read_text()
        code = source[source.index('SKIP_FRAME='):source.index("printf '%s\\n'")]
        for interval in ("4", "8", "16", "32", "0", "2", "bad"):
            with self.subTest(interval=interval):
                env = dict(os.environ, CONTROL="", XENO_TITLE_SMOKE_PROLOGUE_INTERVAL=interval)
                result = subprocess.run(["bash", "-euc", code + '\nprintf "%s" "$schedule"'],
                                        env=env, text=True, capture_output=True)
                if interval not in ("4", "8", "16", "32"):
                    self.assertEqual(result.returncode, 2)
                    continue
                self.assertEqual(result.returncode, 0, result.stderr)
                pairs = [tuple(int(n, 0) for n in p.split(":"))
                         for p in result.stdout.split(",")]
                self.assertEqual(len(pairs), 4095)
                self.assertEqual(pairs[-1][1], 0)
                self.assertEqual(pairs[-2][0] - pairs[-4][0], int(interval))
                self.assertEqual(pairs[-1][0] - pairs[-2][0], 2)

    def test_optional_pre_attract_schedule(self):
        source = SCRIPT.read_text()
        schedule_code = source[source.index('SKIP_FRAME='):source.index("printf '%s\\n'")]
        for extra in ("", "1600"):
            for control in ("", "cross"):
                with self.subTest(extra=extra, control=control):
                    env = dict(os.environ, CONTROL=control,
                               XENO_TITLE_SMOKE_PRE_ATTRACT_FRAME=extra,
                               XENO_TITLE_SMOKE_ATTRACT_FRAME="2400",
                               XENO_TITLE_SMOKE_OPEN_FRAME="2900",
                               XENO_TITLE_SMOKE_UP_FRAME="3200",
                               XENO_TITLE_SMOKE_CONFIRM_FRAME="3300",
                               XENO_TITLE_SMOKE_PROLOGUE_FRAME="3800")
                    result = subprocess.run(
                        ["bash", "-euc", schedule_code + '\nprintf "%s" "$schedule"'],
                        env=env, text=True, capture_output=True, check=True)
                    pairs = [tuple(int(n, 0) for n in p.split(":"))
                             for p in result.stdout.split(",")]
                    button = 0x40 if control else 0x20
                    prefix = [(0, 0), (300, 0x40), (320, 0)]
                    if extra:
                        prefix += [(1600, button), (1602, 0)]
                    prefix += [(2400, button), (2402, 0), (2900, button),
                               (2902, 0), (3200, 0x1000), (3202, 0),
                               (3300, button), (3302, 0)]
                    self.assertEqual(pairs[:len(prefix)], prefix)
                    self.assertEqual(len(pairs), 4095)
                    self.assertTrue(all(a[0] < b[0] for a, b in zip(pairs, pairs[1:])))
                    self.assertEqual(pairs[len(prefix)], (3800, button))
                    self.assertEqual(pairs[-1][1], 0)

    def check(self, lines, expected, rc=124, control="", absent_pass=None):
        with tempfile.TemporaryDirectory(prefix="xeno-title-verdict-") as tmp:
            root = Path(tmp)
            log = root / "run.log"
            log.write_text("\n".join(lines) + "\n")
            env = dict(os.environ, LOGFILE=str(log), OUTDIR=tmp, CAPDIR=tmp,
                       CONTROL=control, REQUIRE_LAHAN="1", rc=str(rc), steps="0")
            result = subprocess.run(["bash", "-euc", VERDICT], env=env,
                                    text=True, capture_output=True)
            self.assertEqual(result.returncode, expected, result.stdout + result.stderr)
            self.assertIn("TITLE SMOKE overall=" + ("PASS" if expected == 0 else "FAIL"),
                          result.stdout)
            if absent_pass:
                self.assertNotIn("TITLE SMOKE PASS " + absent_pass, result.stdout)

    def test_complete_chain(self):
        for rc in (0, 124):
            with self.subTest(rc=rc):
                self.check(CHAIN, 0, rc)

    def test_each_missing_link(self):
        for i in range(len(CHAIN)):
            with self.subTest(link=i):
                self.check(CHAIN[:i] + CHAIN[i + 1:], 1)

    def test_exact_map_numbers(self):
        for index, wrong in ((0, 4900), (7, 490), (7, 40), (8, 20), (8, 200)):
            lines = CHAIN.copy()
            lines[index] = f"[field-diag] FieldLoad begin field={wrong}"
            with self.subTest(index=index, wrong=wrong):
                self.check(lines, 1)

    def test_missing_newgame_does_not_claim_map4(self):
        self.check(CHAIN[:6], 1, absent_pass="new.game.loads.prologue.map.4")

    def test_wrong_order(self):
        for a, b in ((0, 1), (1, 2), (2, 4), (4, 5), (5, 6), (6, 7), (7, 8)):
            lines = CHAIN.copy()
            lines[a], lines[b] = lines[b], lines[a]
            with self.subTest(a=a, b=b):
                self.check(lines, 1)

    def test_map_marker_with_trailing_fields(self):
        lines = CHAIN.copy()
        for i in (0, 7, 8):
            lines[i] += " mode=0"
        self.check(lines, 0)

    def test_runtime_errors(self):
        for rc in (1, 2, 125, 126, 127, 134, 137, 139, 143):
            with self.subTest(rc=rc):
                self.check(CHAIN, 1, rc)

    def test_fatal_or_unimplemented(self):
        for marker in ("SIGSEGV", "AddressSanitizer", "[stub] func_801D9F98"):
            with self.subTest(marker=marker):
                self.check(CHAIN + [marker], 1)

    def test_cross_control(self):
        self.check(CHAIN[:2], 0, control="cross")
        self.check(CHAIN[:2] + CHAIN[4:6], 1, control="cross")
        self.check(CHAIN, 1, control="cross")
        self.check(CHAIN[:2], 1, rc=139, control="cross")


if __name__ == "__main__":
    unittest.main()
