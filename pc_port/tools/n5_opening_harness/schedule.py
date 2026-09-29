#!/usr/bin/env python3
"""Deterministic field-input schedule for the N5 opening harness.

XENO_FIELD_TEST_INPUT expects "frame:value,frame:value,..." with the first
frame 0 and strictly increasing boundaries.  The Lahan opening needs repeated
Circle confirms, so this generator emits 2-frames-on / 2-frames-off Circle
(0x20) pulses up to the parser's 4096-step cap.  The schedule frame counter
starts when FieldPollControllers first merges input, i.e. the first field
frame of whichever map the run begins on.
"""
from __future__ import annotations

import argparse
from typing import Iterator


CIRCLE = 0x20
DEFAULT_PULSES = 2048
STEP_CAP = 4096


def generate_steps(pulses: int = DEFAULT_PULSES,
                   on_frames: int = 2,
                   off_frames: int = 2,
                   step_cap: int = STEP_CAP) -> list[tuple[int, int]]:
    if pulses <= 0:
        raise ValueError("pulses must be positive")
    if on_frames <= 0 or off_frames <= 0:
        raise ValueError("on/off frame counts must be positive")

    steps: list[tuple[int, int]] = []
    frame = 0
    for pulse in range(pulses):
        if len(steps) + 2 > step_cap:
            # Drop the trailing off entry when the cap lands on an on/off
            # boundary pair; keeping the on entry preserves the full pulse.
            break
        steps.append((frame, CIRCLE))
        frame += on_frames
        steps.append((frame, 0))
        frame += off_frames
    if not steps:
        raise ValueError("step cap too small for any pulse")
    return steps


def frames_for_steps(steps: list[tuple[int, int]]) -> Iterator[int]:
    return (frame for frame, _value in steps)


def to_csv(steps: list[tuple[int, int]]) -> str:
    return ",".join(f"{frame}:0x{value:x}" for frame, value in steps)


def self_test() -> None:
    default = generate_steps()
    assert len(default) <= STEP_CAP, "exceeded parser step cap"
    assert default[0] == (0, CIRCLE), "schedule must start at frame 0 on Circle"
    frames = list(frames_for_steps(default))
    assert all(a < b for a, b in zip(frames, frames[1:])), "frames must rise"
    expected_values = [CIRCLE, 0] * (len(default) // 2)
    if len(default) % 2:
        expected_values.append(CIRCLE)
    assert [v for _f, v in default] == expected_values, "values must alternate"
    assert len(generate_steps(pulses=1)) == 2, "one pulse is on+off"
    for on in (1, 2, 4):
        steps = generate_steps(pulses=3, on_frames=on, off_frames=3)
        duty_frames = list(frames_for_steps(steps))
        assert all(
            (on if idx % 2 == 0 else 3) == (b - a)
            for idx, (a, b) in enumerate(zip(duty_frames, duty_frames[1:]))
        ), "on/off duty widths must be exact"
    print(
        "N5 SCHEDULE SELF-TEST PASS "
        f"steps={len(default)} last_frame={frames[-1]} pulses={len(default) // 2}"
    )


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--pulses", type=int, default=DEFAULT_PULSES)
    parser.add_argument("--on", type=int, default=2)
    parser.add_argument("--off", type=int, default=2)
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    if args.self_test:
        self_test()
        return 0
    steps = generate_steps(pulses=args.pulses, on_frames=args.on,
                           off_frames=args.off)
    print(to_csv(steps))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
