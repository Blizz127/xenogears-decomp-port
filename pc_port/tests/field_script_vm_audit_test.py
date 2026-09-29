#!/usr/bin/env python3
"""Focused positive and negative controls for audit_field_script_vm.py."""

from __future__ import annotations

import argparse
import importlib.util
import os
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
TOOL = ROOT / "pc_port/tools/audit_field_script_vm.py"


def load_tool():
    spec = importlib.util.spec_from_file_location("field_script_vm_audit", TOOL)
    assert spec is not None and spec.loader is not None
    module = importlib.util.module_from_spec(spec)
    # dataclasses resolves annotations through sys.modules while decorating.
    import sys

    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


def main() -> int:
    audit = load_tool()
    matching = os.environ.get("FIELD_MATCHING_ROOT")
    args = argparse.Namespace(
        root=ROOT,
        matching_root=Path(matching) if matching else None,
        json=None,
        csv=None,
        strict_runtime=False,
    )
    report = audit.build_audit(args)

    assert report["schema"] == 5
    dispatch = report["dispatch"]
    assert dispatch["primary_slots"] == 0x100
    assert dispatch["secondary_slots"] == 0xE3
    assert dispatch["total_slots"] == 483
    assert dispatch["unique_handlers"] == 481
    assert dispatch["table_mismatches"] == []
    assert report["retail_field"]["sha256"] == audit.FIELD_RETAIL_SHA256
    assert all(row["source_kind"] != "missing" for row in report["handlers"])
    native = report["native"]
    native_object_root = ROOT / "pc_port/build_native/obj"
    expected_native_artifacts = (
        (ROOT / "pc_port/build_native/stubs.c").is_file()
        and native_object_root.is_dir()
        and (ROOT / "pc_port/build_native/xeno-port").is_file()
        and any(native_object_root.glob("*.o"))
    )
    assert native["artifacts_present"] == expected_native_artifacts
    assert bool(native["missing_artifacts"]) == (not native["artifacts_present"])
    if native["artifacts_present"]:
        assert native["missing_artifacts"] == []
    assert native["authored_stub_functions"] == audit.scan_authored_stub_functions(
        ROOT
    )
    assert native["field_direct_authored_stub_caller_count"] == len(
        native["field_direct_authored_stub_edges"]
    )
    assert native["field_direct_authored_stub_edge_count"] == sum(
        len(row["authored_stub_targets"])
        for row in native["field_direct_authored_stub_edges"]
    )
    assert native["field_direct_stub_caller_count"] == len(
        native["field_direct_stub_edges"]
    )
    assert native["field_direct_stub_edge_count"] == sum(
        len(row["generated_stub_targets"])
        for row in native["field_direct_stub_edges"]
    )
    assert native["field_generated_data_caller_count"] == len(
        native["field_generated_data_edges"]
    )
    assert native["field_generated_data_edge_count"] == sum(
        len(row["generated_stub_targets"])
        for row in native["field_generated_data_edges"]
    )
    assert native["field_generated_data_target_count"] == len(
        native["field_generated_data_targets"]
    )
    assert native["field_generated_data_default_32_count"] == sum(
        row["allocated_size"] == 32
        for row in native["field_generated_data_targets"]
    )
    assert all(
        {"function", "path", "line", "conditionals", "native_may_compile"}
        <= set(row)
        for row in native["active_assert_zero"]
    )
    assert native["native_active_assert_zero"] == [
        row for row in native["active_assert_zero"] if row["native_may_compile"]
    ]

    # Strict mode is a whole-field fail-closed gate, not merely a dispatch
    # table check. Exercise each independent blocker so later refactors cannot
    # accidentally narrow its scope.
    strict_fixture = {
        "handlers": [{"audit_class": "RETAIL_MIPS_MATCH"}],
        "native": {
            "artifacts_present": True,
            "field_direct_stub_edges": [],
            "field_direct_authored_stub_edges": [],
            "field_generated_data_edges": [],
            "native_active_assert_zero": [],
        },
    }
    assert not audit.has_strict_runtime_blockers(strict_fixture)
    strict_fixture["native"]["artifacts_present"] = False
    assert audit.has_strict_runtime_blockers(strict_fixture)
    strict_fixture["native"]["artifacts_present"] = True
    strict_fixture["handlers"][0]["audit_class"] = "INCOMPLETE_SOURCE_MARKER"
    assert audit.has_strict_runtime_blockers(strict_fixture)
    strict_fixture["handlers"][0]["audit_class"] = "RETAIL_MIPS_MATCH"
    strict_fixture["native"]["field_direct_stub_edges"] = [{"caller": "gap"}]
    assert audit.has_strict_runtime_blockers(strict_fixture)
    strict_fixture["native"]["field_direct_stub_edges"] = []
    strict_fixture["native"]["field_direct_authored_stub_edges"] = [
        {"caller": "authored_gap"}
    ]
    assert audit.has_strict_runtime_blockers(strict_fixture)
    strict_fixture["native"]["field_direct_authored_stub_edges"] = []
    strict_fixture["native"]["field_generated_data_edges"] = [
        {"caller": "data_gap"}
    ]
    assert audit.has_strict_runtime_blockers(strict_fixture)
    strict_fixture["native"]["field_generated_data_edges"] = []
    strict_fixture["native"]["native_active_assert_zero"] = [
        {"function": "gap"}
    ]
    assert audit.has_strict_runtime_blockers(strict_fixture)

    # Retail parity is deliberately stronger than runtime completeness. A
    # nonmatching or unproven native implementation remains red even after all
    # direct placeholder/data/assert blockers have been retired.
    retail_fixture = {
        "dispatch": {"table_mismatches": []},
        "handlers": [{"audit_class": "RETAIL_MIPS_MATCH"}],
        "native": {
            "artifacts_present": True,
            "field_direct_stub_edges": [],
            "field_direct_authored_stub_edges": [],
            "field_generated_data_edges": [],
            "native_active_assert_zero": [],
        },
    }
    assert not audit.has_strict_retail_blockers(retail_fixture)
    retail_fixture["handlers"][0]["audit_class"] = "C_NONMATCHING_UNPROVEN"
    assert audit.has_strict_retail_blockers(retail_fixture)
    retail_fixture["handlers"][0]["audit_class"] = "RETAIL_MIPS_MATCH"
    retail_fixture["dispatch"]["table_mismatches"] = [{"slot": "00"}]
    assert audit.has_strict_retail_blockers(retail_fixture)

    # The C scanner must ignore braces in comments/strings and stop at the
    # actual function boundary rather than swallowing the next definition.
    scanner_fixture = (
        'void target(void) {\n'
        '    const char *text = "{not code}"; /* } neither { */\n'
        '    if (text) { return; } // } ignored\n'
        '}\n'
        'void next(void) { }\n'
    )
    cleaned = audit.strip_c_for_braces(scanner_fixture)
    opening = cleaned.index("{", cleaned.index("target"))
    ending = audit.find_matching_brace(cleaned, opening)
    assert cleaned.count("{") == 3
    assert cleaned.count("}") == 3
    assert "void next" not in scanner_fixture[opening:ending]

    with tempfile.TemporaryDirectory(prefix="field-vm-assert-scan.") as tmp:
        fixture_root = Path(tmp)
        fixture_path = fixture_root / "src/field/fixture.c"
        fixture_path.parent.mkdir(parents=True)
        fixture_path.write_text(
            "/* assert(0) in a comment is not executable. */\n"
            "void proven(void) { const char *s = \"assert(0)\"; }\n"
            "void incomplete(void) { assert(0 && \"missing\"); }\n"
            "#ifdef XENO_PC_PORT\n"
            "void native_gap(void) { assert(0); }\n"
            "#else\n"
            "void retail_only_gap(void) { assert(0); }\n"
            "#endif\n",
            encoding="utf-8",
        )
        assert_sites = audit.scan_active_assert_zero(fixture_root)
        assert len(assert_sites) == 3
        assert assert_sites[0]["function"] == "incomplete"
        assert assert_sites[1]["function"] == "native_gap"
        assert assert_sites[1]["native_may_compile"]
        assert assert_sites[2]["function"] == "retail_only_gap"
        assert not assert_sites[2]["native_may_compile"]

    if matching:
        comparisons = report["matching_build"]["comparison_counts"]
        assert sum(comparisons.values()) == 481
        assert set(comparisons) <= {
            "match",
            "size_mismatch",
            "code_mismatch",
            "relocation_target_mismatch",
        }
        assert comparisons.get("match", 0) > 0
        assert report["matching_build"]["relocation_target_checks"] > 0

    # Negative control 1: a port-table substitution must be detectable even
    # though the table retains the expected number of entries.
    source_path = ROOT / "pc_port/src/data_field.c"
    source = source_path.read_text(encoding="utf-8")
    mutant = source.replace(
        "FIELD_VM_HANDLER(func_800A1B70)",
        "FIELD_VM_HANDLER(FieldScriptVMHandlerJmp)",
        1,
    )
    assert mutant != source
    with tempfile.TemporaryDirectory(prefix="field-vm-audit-test.") as tmp:
        mutant_path = Path(tmp) / "data_field.c"
        mutant_path.write_text(mutant, encoding="utf-8")
        mutant_tables = audit.parse_port_tables(mutant_path)
    symbols = audit.parse_symbol_addresses(ROOT / "config/symbol_addrs.field.txt")
    field_vram = audit.parse_field_vram(ROOT / "config/field.yaml")
    retail_tables = audit.read_retail_tables(
        ROOT / "disc/field.bin", field_vram, symbols
    )
    mutant_handler = mutant_tables["g_FieldScriptVMHandlers"][0]
    mutant_address = audit.resolve_handler_address(mutant_handler, symbols)
    assert mutant_address != retail_tables["g_FieldScriptVMHandlers"][0]

    # Negative control 2: relocation operand masking must never erase opcode
    # bits.  A non-relocation instruction mutation remains a hard mismatch.
    assert (0x27BDFFE8 & audit.MIPS_RELOC_MASKS["R_MIPS_26"]) != (
        0x23BDFFE8 & audit.MIPS_RELOC_MASKS["R_MIPS_26"]
    )

    # Negative control 3: relocation comparison is symmetric. A target present
    # only on either side, or the same target with a different kind, must fail.
    relocation = audit.Relocation("R_MIPS_26", "retail_target")
    assert audit.external_relocation_signature([relocation]) != (
        audit.external_relocation_signature([])
    )
    assert audit.external_relocation_signature([relocation]) != (
        audit.external_relocation_signature(
            [audit.Relocation("R_MIPS_HI16", "retail_target")]
        )
    )
    assert audit.external_relocation_signature([relocation], 0x08000000) != (
        audit.external_relocation_signature([relocation], 0x08000001)
    )

    # Negative control 4: section-local jump operands are not thrown away.
    # Moving a whole function is normalized, but changing its internal target
    # must be detected.
    current_owner = audit.Symbol(Path("current.o"), 0x100, 0x40)
    retail_owner = audit.Symbol(Path("retail.o"), 0x200, 0x40)
    text_jump = [audit.Relocation("R_MIPS_26", ".text")]
    current_jump = audit.section_relocation_signature(
        text_jump, 0x08000048, current_owner
    )
    retail_jump = audit.section_relocation_signature(
        text_jump, 0x08000088, retail_owner
    )
    mutated_jump = audit.section_relocation_signature(
        text_jump, 0x0800008C, retail_owner
    )
    assert current_jump == retail_jump
    assert current_jump != mutated_jump

    with tempfile.TemporaryDirectory(prefix="field-vm-stub-parse.") as tmp:
        stub_fixture = Path(tmp) / "stubs.c"
        stub_fixture.write_text(
            "unsigned char retail_state[64] __attribute__((aligned(8)));\n"
            "long missing_fn(void) { xeno_port_stub(\"missing_fn\"); return 0; }\n",
            encoding="utf-8",
        )
        assert audit.parse_generated_data(stub_fixture) == {"retail_state": 64}
        assert audit.parse_generated_stubs(stub_fixture) == {"missing_fn"}

    with tempfile.TemporaryDirectory(prefix="field-vm-authored-stub-scan.") as tmp:
        fixture_root = Path(tmp)
        fixture_path = fixture_root / "pc_port/src/fixture.c"
        fixture_path.parent.mkdir(parents=True)
        fixture_path.write_text(
            "/* xeno_port_stub(\"comment\") must not count. */\n"
            "void real_gap(int value) {\n"
            "    const char *text = \"xeno_port_stub(ignored)\";\n"
            "    (void)value; (void)text; xeno_port_stub(\"real_gap\");\n"
            "}\n"
            "void complete(void) { }\n",
            encoding="utf-8",
        )
        authored = audit.scan_authored_stub_functions(fixture_root)
        assert authored == [
            {
                "function": "real_gap",
                "path": "pc_port/src/fixture.c",
                "line": 2,
                "stub_call_lines": [4],
            }
        ]

    print(
        "FIELD_SCRIPT_VM_AUDIT_TEST PASS "
        "slots=483 unique=481 table_substitution_mutant=detected "
        "instruction_mutant=detected relocation_mutant=detected "
        "source_scanner_fixture=passed assert_scanner_fixture=passed "
        "authored_stub_scanner_fixture=passed"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
