#!/usr/bin/env python3
"""Check the linked camera globals against the retail save/restore region.

Retail 800A4284/800A37A8 copies 0x1C8 bytes at 800AF880. Independent
host stubs silently bypass that copy even when isolated camera tests pass.
This checks the final executable, including stub resolution, not source text.
"""
import argparse
import subprocess

CAMERA = {
    "g_CameraEye": 0x800AF880,
    "g_CameraAt": 0x800AF890,
    "g_CameraUp": 0x800AF8A0,
    "g_CameraEye2": 0x800AF8B0,
    "g_CameraAt2": 0x800AF8C0,
    "D_800AF8D0": 0x800AF8D0,
    "D_800AF8E0": 0x800AF8E0,
    "D_800AF8E4": 0x800AF8E4,
    "D_800AF8E8": 0x800AF8E8,
    "g_CamAtMovementFrom": 0x800AF8F0,
    "g_CamAtMovementTo": 0x800AF900,
    "g_CamEyeMovementFrom": 0x800AF910,
    "g_CamEyeMovementTo": 0x800AF920,
    "D_800AF930": 0x800AF930,
    "g_FieldCameraMode": 0x800AF934,
    "D_800AF936": 0x800AF936,
    "D_800AF938": 0x800AF938,
    "D_800AF93A": 0x800AF93A,
    "g_CamMovementFlags": 0x800AF93C,
    "g_CamAtMovementDuration": 0x800AF93E,
    "g_CamAtMovementCurrent": 0x800AF940,
    "g_CamAtMovementDelta": 0x800AF950,
    "g_CamEyeMovementDuration": 0x800AF960,
    "g_CamEyeMovementCurrent": 0x800AF964,
    "g_CamEyeMovementDelta": 0x800AF974,
    "g_CamInterpolation": 0x800AF984,
    "g_Scene": 0x800AF990,
}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("binary", nargs="?", default="pc_port/build_native/xeno-port")
    args = parser.parse_args()
    symbols = {}
    for line in subprocess.check_output(["nm", "-n", args.binary], text=True).splitlines():
        fields = line.split()
        if len(fields) == 3 and fields[2] in CAMERA:
            symbols[fields[2]] = int(fields[0], 16)
    base = symbols["g_CameraEye"]
    failures = []
    for name, retail in CAMERA.items():
        expected = base + retail - 0x800AF880
        actual = symbols.get(name)
        if actual != expected:
            failures.append(f"{name}: actual={actual!r}, expected={expected:#x}")
    if failures:
        raise SystemExit("camera saved-state layout FAIL\n" + "\n".join(failures))
    print(f"camera saved-state layout PASS: {len(CAMERA)} linked retail offsets")


if __name__ == "__main__":
    main()
