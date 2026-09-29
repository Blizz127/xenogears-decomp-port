"""Source inventory checks; not VM execution or gameplay acceptance."""
import importlib.util
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
SPEC = importlib.util.spec_from_file_location(
    "clip_inventory", ROOT / "tools/scripts/audit_field_clip_vm.py"
)
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


class InventoryTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.payload = MODULE.read_overlay(ROOT / "disc/disc1.bin")

    def test_all_opcode_slots_and_shared_handlers(self):
        report = MODULE.inventory(self.payload)
        entries = report["dispatch"]
        self.assertEqual([e["opcode"] for e in entries], list(range(113)))
        self.assertEqual(entries[0]["entry"], "0x801e3d84")
        self.assertEqual(entries[1]["entry"], "0x801e3d8c")
        self.assertEqual(entries[0x21]["entry"], "0x801e46d4")
        self.assertEqual(entries[0x70]["entry"], "0x801e5940")
        self.assertEqual(entries[0x28]["entry"], entries[0x29]["entry"])
        self.assertEqual(entries[4]["entry"], "0x801e5974")
        self.assertEqual(report["instruction_bytes"], 8164)
        self.assertEqual(report["unique_dispatch_entries"], 81)
        self.assertEqual(len(report["continuation_opcodes"]), 29)
        self.assertIn({"site": "0x801e5998", "target": "0x800796f4"}, report["direct_calls"])

    def test_changed_dispatch_is_rejected(self):
        altered = bytearray(self.payload)
        altered[0x40] ^= 4
        with self.assertRaisesRegex(ValueError, "overlay SHA"):
            MODULE.inventory(altered)

    def test_sound_bank_route_is_not_a_scale_operation(self):
        route = MODULE.inventory(self.payload)["sound_bank_route"]
        self.assertEqual(route["opcode"], 0x3C)
        self.assertEqual(route["handler"], "0x801e4f00")
        self.assertEqual(route["bank_call"], "0x801e4f18")
        self.assertEqual(route["consumer"], "0x8003a3b8")
        self.assertEqual(route["selector_zero_slot"], "0x8005919c")
        self.assertEqual(route["selector_one_object_offset"], 0xB0)
        self.assertEqual(route["selector_two_object_offset"], 0xB4)
        self.assertEqual(route["other_selector_result"], 2)
        self.assertEqual(route["native_global_owner"], "UNRESOLVED")

    def test_changed_body_is_rejected(self):
        altered = bytearray(self.payload)
        altered[0x79F0] ^= 1
        with self.assertRaisesRegex(ValueError, "overlay SHA"):
            MODULE.inventory(altered)

    def test_truncated_payload_is_rejected(self):
        with self.assertRaisesRegex(ValueError, "overlay size"):
            MODULE.inventory(self.payload[:-1])


if __name__ == "__main__":
    unittest.main()
