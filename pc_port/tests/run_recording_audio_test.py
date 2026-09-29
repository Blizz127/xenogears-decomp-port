#!/usr/bin/env python3
"""Durable Pulse/null-sink regression for F9 recorder audio timestamps.

The test owns a temporary Pulse null sink and a generated 440 Hz WAV. It does
not change the user's default sink/source. The fixture compiles the selected
recorder .inl, captures the owned monitor through ffmpeg, and drives the
recorder at both 30 Hz and 150 Hz. The 150 Hz case checks the recorder's 60 Hz
host cap rather than requiring the video stream to contain 150 frames/sec.
"""

from __future__ import annotations

import argparse
import array
import hashlib
import json
import math
import os
from pathlib import Path
import re
import subprocess
import tempfile
import time
import uuid
import wave


CAPTURE_SECONDS = 4.0
TONE_SECONDS = 10.0
RATE = 48000
FREQUENCY = 440.0
RMS_THRESHOLD = 0.01


class AudioQualityFailure(AssertionError):
    """The owned reference worked, but the recorder quality gate failed."""


def run(command, *, env, stdout=None, stderr=None, check=True):
    return subprocess.run(command, env=env, stdout=stdout, stderr=stderr,
                          check=check, text=True)


def capture(command, *, env):
    return subprocess.check_output(command, env=env, text=True).strip()


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def tool_version(tool: str, env: dict[str, str]) -> str:
    result = subprocess.run([tool, "--version"], env=env,
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                            text=True, check=False)
    return result.stdout.splitlines()[0] if result.stdout else ""


def write_tone(path: Path) -> None:
    with wave.open(str(path), "wb") as stream:
        stream.setnchannels(2)
        stream.setsampwidth(2)
        stream.setframerate(RATE)
        frames = bytearray()
        for index in range(int(TONE_SECONDS * RATE)):
            sample = int(10000 * math.sin(2.0 * math.pi * FREQUENCY * index / RATE))
            frames.extend(sample.to_bytes(2, "little", signed=True))
            frames.extend(sample.to_bytes(2, "little", signed=True))
        stream.writeframes(frames)


def pactl(args: list[str], env: dict[str, str]) -> str:
    return capture(["pactl", *args], env=env)


def pactl_json(kind: str, env: dict[str, str]):
    return json.loads(pactl(["--format=json", "list", kind], env))


def pulse_snapshot(env: dict[str, str]) -> dict:
    return {
        "default_sink": pactl(["get-default-sink"], env),
        "default_source": pactl(["get-default-source"], env),
    }


def find_named(items, name: str):
    return next(item for item in items if item.get("name") == name)


def wait_for(predicate, timeout: float, description: str):
    deadline = time.monotonic() + timeout
    last = None
    while time.monotonic() < deadline:
        try:
            value = predicate()
            if value:
                return value
            last = value
        except (subprocess.CalledProcessError, StopIteration) as error:
            last = repr(error)
        time.sleep(0.1)
    raise AssertionError(f"timed out waiting for {description}: {last}")


def own_audio_state(env: dict[str, str], sink_name: str, monitor_name: str) -> dict:
    sinks = pactl_json("sinks", env)
    sources = pactl_json("sources", env)
    sink = find_named(sinks, sink_name)
    monitor = find_named(sources, monitor_name)
    assert sink.get("mute") is False, f"owned sink is muted: {sink}"
    assert monitor.get("mute") is False, f"owned monitor is muted: {monitor}"
    return {"sink": sink, "monitor": monitor}


def process_id(properties: dict):
    value = properties.get("application.process.id")
    try:
        return int(value)
    except (TypeError, ValueError):
        return None


def move_tone_to_owned_sink(env: dict[str, str], sink_name: str, play: subprocess.Popen):
    def locate():
        for item in pactl_json("sink-inputs", env):
            if process_id(item.get("properties", {})) == play.pid:
                return item
        return None

    sink_input = wait_for(locate, 3.0, "tone sink-input")
    sink = find_named(pactl_json("sinks", env), sink_name)
    run(["pactl", "move-sink-input", str(sink_input["index"]), str(sink["index"])],
        env=env)
    run(["pactl", "set-sink-input-mute", str(sink_input["index"]), "0"], env=env)
    run(["pactl", "set-sink-input-volume", str(sink_input["index"]), "100%"], env=env)
    return sink_input["index"], sink["index"]


def recorder_child_pid(log_path: Path) -> int | None:
    match = re.search(r"XENO_RECORDING_FFMPEG_PID=(\d+)",
                      log_path.read_text(errors="replace"))
    return int(match.group(1)) if match else None


def assert_owned_routes(env: dict[str, str], sink_name: str, monitor_name: str,
                        monitor_index: int, tone_input_index: int,
                        tone_sink_index: int, reference_pid: int,
                        recorder_pid: int) -> dict:
    sink_inputs = pactl_json("sink-inputs", env)
    tone = next(item for item in sink_inputs if item["index"] == tone_input_index)
    assert tone["sink"] == tone_sink_index, f"tone escaped owned sink: {tone}"
    source_outputs = pactl_json("source-outputs", env)
    def output_for(pid: int):
        matches = [item for item in source_outputs
                   if process_id(item.get("properties", {})) == pid]
        assert len(matches) == 1, (
            f"expected one ffmpeg source-output for PID {pid}, got {matches}; "
            f"all outputs: {source_outputs}"
        )
        item = matches[0]
        assert item.get("properties", {}).get("application.process.binary") == "ffmpeg", item
        assert item.get("source") == monitor_index, (
            f"PID {pid} is not recording owned monitor {monitor_name}: {item}"
        )
        return item

    return {"tone": tone, "reference": output_for(reference_pid),
            "recorder": output_for(recorder_pid)}


def decode_mono_8k(path: Path) -> list[float]:
    result = subprocess.run(
        ["ffmpeg", "-hide_banner", "-loglevel", "error", "-i", str(path),
         "-map", "0:a:0", "-f", "f32le", "-ac", "1", "-ar", "8000", "-"],
        stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=True,
    )
    values = array.array("f")
    values.frombytes(result.stdout)
    return list(values)


def rms_bins(samples: list[float], samples_per_bin: int = 800) -> list[float]:
    result = []
    for offset in range(0, len(samples), samples_per_bin):
        block = samples[offset:offset + samples_per_bin]
        if block:
            result.append(math.sqrt(sum(value * value for value in block) / len(block)))
    return result


def quality_assert(condition: bool, message: str) -> None:
    if not condition:
        raise AudioQualityFailure(message)


def probe(path: Path, env: dict[str, str]) -> dict:
    raw = subprocess.check_output(
        ["ffprobe", "-hide_banner", "-v", "error", "-show_streams",
         "-show_format", "-of", "json", str(path)], env=env, text=True)
    data = json.loads(raw)
    streams = data.get("streams", [])
    audio = next(stream for stream in streams if stream.get("codec_type") == "audio")
    video = next(stream for stream in streams if stream.get("codec_type") == "video")
    return {
        "audio": audio,
        "video": video,
        "format_duration": float(data["format"]["duration"]),
        "audio_duration": float(audio["duration"]),
        "video_duration": float(video["duration"]),
        "video_rate": float(audio_rate(video.get("avg_frame_rate", "0/0"))),
    }


def audio_rate(value: str) -> float:
    numerator, denominator = value.split("/")
    return float(numerator) / float(denominator) if float(denominator) else 0.0


def make_fixture(repo_root: Path, source: Path, output: Path, env: dict[str, str]) -> Path:
    fixture = repo_root / "pc_port/tests/recording_audio_fixture.cpp"
    include_path = str(source.resolve()).replace("\\", "\\\\").replace('"', '\\"')
    generated = output / "recording_audio_fixture.cpp"
    text = fixture.read_text()
    text = text.replace('#include "RECORDING_SOURCE_PLACEHOLDER"',
                        f'#include "{include_path}"')
    generated.write_text(text)
    binary = output / "recording_audio_fixture"
    compiler = os.environ.get("CXX", "g++")
    run([compiler, "-std=c++17", "-O2", "-pthread", str(generated), "-o", str(binary)],
        env=env)
    return binary


def run_case(case_dir: Path, fixture: Path, env: dict[str, str], sink_name: str,
             monitor_name: str, sink_index: int, monitor_index: int,
             tone: Path, presentation_rate: int) -> dict:
    case_dir.mkdir(parents=True, exist_ok=False)
    recordings = case_dir / "recordings"
    recordings.mkdir()
    env = env.copy()
    env.update({
        "XENO_RECORDING_DIR": str(recordings),
        "XENO_RECORDING_AUDIO_SOURCE": monitor_name,
    })
    play_log = (case_dir / "paplay.log").open("w")
    reference_log = (case_dir / "reference.log").open("w")
    fixture_log = (case_dir / "recorder.log").open("w")
    children: list[subprocess.Popen] = []
    try:
        play = subprocess.Popen(
            ["paplay", "--device=" + sink_name, "--client-name=" + sink_name,
             "--stream-name=" + sink_name, str(tone)],
            env=env, stdout=play_log, stderr=subprocess.STDOUT,
        )
        children.append(play)
        tone_input_index, owned_sink_index = move_tone_to_owned_sink(env, sink_name, play)
        assert owned_sink_index == sink_index
        time.sleep(0.25)
        reference = subprocess.Popen(
            ["ffmpeg", "-hide_banner", "-loglevel", "error", "-f", "pulse",
             "-i", monitor_name, "-t", str(CAPTURE_SECONDS), "-ac", "2",
             "-ar", str(RATE), "-c:a", "pcm_s16le", str(case_dir / "reference.wav")],
            env=env, stdout=reference_log, stderr=subprocess.STDOUT,
        )
        children.append(reference)
        recorder = subprocess.Popen([str(fixture), str(presentation_rate)], env=env,
                                     stdout=fixture_log, stderr=subprocess.STDOUT)
        children.append(recorder)
        recorder_pid = wait_for(
            lambda: recorder_child_pid(case_dir / "recorder.log"),
            3.0, "recorder ffmpeg PID",
        )
        routes = assert_owned_routes(env, sink_name, monitor_name, monitor_index,
                                     tone_input_index, sink_index, reference.pid,
                                     recorder_pid)
        recorder_exit = recorder.wait(timeout=20)
        assert recorder_exit == 0, f"fixture failed at {presentation_rate} Hz"
        assert reference.wait(timeout=20) == 0, "reference capture failed"
        output_files = sorted(recordings.glob("*.mp4"))
        assert len(output_files) == 1, f"expected one recording, got {output_files}"
        reference_path = case_dir / "reference.wav"
        reference_samples = decode_mono_8k(reference_path)
        recording_samples = decode_mono_8k(output_files[0])
        reference_bins = rms_bins(reference_samples)
        recording_bins = rms_bins(recording_samples)
        assert (
            reference_bins and math.sqrt(sum(x * x for x in reference_samples) /
                                         len(reference_samples)) > 0.05
        ), "owned reference tone is silent"
        interior_reference = reference_bins[1:-1]
        interior_recording = recording_bins[1:-1]
        reference_active = sum(value > RMS_THRESHOLD for value in interior_reference)
        recording_active = sum(value > RMS_THRESHOLD for value in interior_recording)
        assert (
            reference_active >= max(1, math.ceil(len(interior_reference) * 0.95))
        ), f"reference is not sustained: {reference_active}/{len(interior_reference)}"
        quality_assert(
            recording_active >= max(1, math.ceil(len(interior_recording) * 0.95)),
            f"recorded audio is not sustained: {recording_active}/{len(interior_recording)}",
        )
        reference_rms = math.sqrt(sum(x * x for x in reference_samples) /
                                  len(reference_samples))
        recording_rms = math.sqrt(sum(x * x for x in recording_samples) /
                                  len(recording_samples))
        ratio = recording_rms / reference_rms if reference_rms else 0.0
        quality_assert(0.85 <= ratio <= 1.15,
                       f"recorded/reference RMS ratio out of range: {ratio}")
        metadata = probe(output_files[0], env)
        quality_assert(
            abs(metadata["audio_duration"] - metadata["video_duration"]) <= 0.35,
            f"A/V duration mismatch: {metadata}",
        )
        quality_assert(metadata["video_duration"] >= 3.0,
                       f"short video: {metadata}")
        quality_assert(20.0 <= metadata["video_rate"] <= 65.0,
                       f"unexpected host-capped video rate: {metadata}")
        return {
            "presentation_rate": presentation_rate,
            "output": str(output_files[0]),
            "routes": routes,
            "reference_bins": len(interior_reference),
            "recording_bins": len(interior_recording),
            "reference_active": reference_active,
            "recording_active": recording_active,
            "reference_rms": reference_rms,
            "recording_rms": recording_rms,
            "rms_ratio": ratio,
            "probe": metadata,
        }
    finally:
        for process in reversed(children):
            if process.poll() is None:
                process.terminate()
                try:
                    process.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    process.kill()
                    process.wait(timeout=5)
        play_log.close()
        reference_log.close()
        fixture_log.close()


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--source", type=Path,
                        help="recorder .inl override; defaults to the repo source")
    parser.add_argument("--output", type=Path,
                        help="preserved output directory; defaults to a unique temp dir")
    parser.add_argument("--expect-failure", action="store_true",
                        help="expect the selected source to fail sustained-audio checks")
    args = parser.parse_args()
    repo_root = Path(__file__).resolve().parents[2]
    source = (args.source or (repo_root / "pc_port/src/psycross_video_recording.inl")).resolve()
    assert source.is_file(), source
    output = (args.output or Path(tempfile.mkdtemp(prefix="xeno-recording-audio-"))).resolve()
    output.mkdir(parents=True, exist_ok=True)
    fixture_source = repo_root / "pc_port/tests/recording_audio_fixture.cpp"
    runner_source = Path(__file__).resolve()
    env = os.environ.copy()
    if not env.get("XDG_RUNTIME_DIR") or not Path(env["XDG_RUNTIME_DIR"]).is_dir():
        env["XDG_RUNTIME_DIR"] = f"/run/user/{os.getuid()}"
    before = pulse_snapshot(env)
    hashes_before = {
        "source": sha256(source),
        "fixture": sha256(fixture_source),
        "runner": sha256(runner_source),
    }
    manifest = {
        "source": str(source),
        "hashes_before": hashes_before,
        "source_sha256": hashes_before["source"],
        "fixture_sha256": hashes_before["fixture"],
        "python": tool_version("python3", env),
        "ffmpeg": tool_version("ffmpeg", env),
        "ffprobe": tool_version("ffprobe", env),
        "pactl": tool_version("pactl", env),
        "paplay": tool_version("paplay", env),
        "before_defaults": before,
        "cases": [],
    }
    module = None
    sink_name = "xeno_recording_test_" + uuid.uuid4().hex[:12]
    monitor_name = sink_name + ".monitor"
    execution_error = None
    try:
        tone = output / "tone-440hz.wav"
        write_tone(tone)
        module = pactl(["load-module", "module-null-sink", "sink_name=" + sink_name,
                        "rate=48000", "channels=2"], env)
        run(["pactl", "set-sink-mute", sink_name, "0"], env=env)
        run(["pactl", "set-sink-volume", sink_name, "100%"], env=env)
        run(["pactl", "set-source-mute", monitor_name, "0"], env=env)
        run(["pactl", "set-source-volume", monitor_name, "100%"], env=env)
        state = own_audio_state(env, sink_name, monitor_name)
        sink_index = state["sink"]["index"]
        monitor_index = state["monitor"]["index"]
        fixture = make_fixture(repo_root, source, output, env)
        manifest["fixture"] = str(fixture)
        manifest["pulse"] = {"module": module, "sink": state["sink"],
                              "monitor": state["monitor"]}
        for rate in (30, 150):
            case = run_case(output / f"case-{rate}hz", fixture, env, sink_name,
                            monitor_name, sink_index, monitor_index, tone, rate)
            manifest["cases"].append(case)
        if args.expect_failure:
            raise AssertionError("selected source passed, but --expect-failure was requested")
    except Exception as error:
        execution_error = error
        manifest["failure"] = repr(error)
    finally:
        cleanup_errors = []
        if module is not None:
            try:
                pactl(["unload-module", str(module)], env)
                remaining_sinks = [item.get("name") for item in pactl_json("sinks", env)]
                remaining_sources = [item.get("name") for item in pactl_json("sources", env)]
                if sink_name in remaining_sinks:
                    cleanup_errors.append("owned test sink survived cleanup")
                if monitor_name in remaining_sources:
                    cleanup_errors.append("owned test monitor survived cleanup")
                manifest["cleanup"] = {
                    "module_unloaded": True,
                    "owned_sink_present": sink_name in remaining_sinks,
                    "owned_monitor_present": monitor_name in remaining_sources,
                }
            except Exception as error:
                cleanup_errors.append(f"private Pulse cleanup failed: {error!r}")
        try:
            after = pulse_snapshot(env)
            manifest["after_defaults"] = after
            if after != before:
                cleanup_errors.append(
                    f"test changed global defaults: {before!r} -> {after!r}"
                )
        except Exception as error:
            cleanup_errors.append(f"could not verify global defaults: {error!r}")
        try:
            hashes_after = {
                "source": sha256(source),
                "fixture": sha256(fixture_source),
                "runner": sha256(runner_source),
            }
            manifest["hashes_after"] = hashes_after
            if hashes_after != hashes_before:
                cleanup_errors.append(
                    f"test input changed during run: before={hashes_before!r}, "
                    f"after={hashes_after!r}"
                )
        except Exception as error:
            cleanup_errors.append(f"could not verify input hashes: {error!r}")
        if cleanup_errors:
            manifest["cleanup_errors"] = cleanup_errors
            if execution_error is None or isinstance(execution_error, AudioQualityFailure):
                execution_error = AssertionError("; ".join(cleanup_errors))
        (output / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    if execution_error is not None:
        if args.expect_failure and isinstance(execution_error, AudioQualityFailure):
            print(json.dumps({"expected_failure": True, "output": str(output),
                              "failure": manifest["failure"]}, indent=2))
            return 0
        raise execution_error
    if args.expect_failure:
        raise AssertionError("expected selected source failure was not observed")
    print(json.dumps({"status": "PASS", "output": str(output),
                      "cases": manifest["cases"]}, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
