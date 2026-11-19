#!/usr/bin/env python3
"""Build and verify the portable prototype with Python stdlib + GCC/Clang.

Does not discover, build, run, or validate Unreal Engine.
"""
import argparse
import datetime
import hashlib
import json
import os
from pathlib import Path
import platform
import re
import shlex
import shutil
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[1]


def source_manifest():
    manifest = {}
    for path in sorted(ROOT.rglob("*")):
        rel = path.relative_to(ROOT)
        if any(part in {".build", "build", ".git", "evidence", "__pycache__", "Binaries", "Intermediate", "Saved"}
               for part in rel.parts):
            continue
        if path.is_file():
            manifest[rel.as_posix()] = hashlib.sha256(path.read_bytes()).hexdigest()
    return manifest


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    sanitizers = parser.add_mutually_exclusive_group()
    sanitizers.add_argument("--sanitize", action="store_true", help="AddressSanitizer + UBSan")
    sanitizers.add_argument("--ubsan", action="store_true", help="UBSan only, when ASan runtime is unavailable")
    parser.add_argument("--report", type=Path, help="JSON evidence output, relative to project root")
    args = parser.parse_args()
    report = {
        "schema_version": 1,
        "status": "running",
        "scope": "portable C++ integer policy model only; Unreal integration NOT built or run",
        "started_at_utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
        "platform": platform.platform(),
        "machine": platform.machine(),
        "python": platform.python_version(),
        "sanitizers": ["address", "undefined"] if args.sanitize else (["undefined"] if args.ubsan else []),
        "commands": [],
        "source_sha256": source_manifest(),
    }
    instrumented = args.sanitize or args.ubsan
    build = ROOT / ".build" / ("sanitized" if args.sanitize else ("ubsan" if args.ubsan else "release"))
    build.mkdir(parents=True, exist_ok=True)

    def run(command, expected=0):
        start = time.perf_counter()
        result = subprocess.run(command, cwd=ROOT, text=True, capture_output=True, timeout=120)
        report["commands"].append({
            "argv": [str(x).replace(str(ROOT), "<project>") for x in command],
            "exit_code": result.returncode,
            "elapsed_seconds": round(time.perf_counter() - start, 6),
            "stdout": result.stdout,
            "stderr": result.stderr,
        })
        if result.returncode != expected:
            raise RuntimeError("command returned {}, expected {}: {}\n{}\n{}".format(
                result.returncode, expected, command[0], result.stdout, result.stderr))
        return result.stdout

    try:
        compiler = shlex.split(os.environ.get("CXX", ""))
        if not compiler:
            compiler = [shutil.which("clang++") or shutil.which("g++") or "c++"]
        report["compiler"] = run(compiler + ["--version"]).splitlines()[0]
        flags = ["-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-I", str(ROOT / "include")]
        flags += (["-O1", "-g", "-fsanitize=" + ("address,undefined" if args.sanitize else "undefined"),
                   "-fno-sanitize-recover=all", "-fno-omit-frame-pointer"] if instrumented else ["-O2"])
        for target, source in [("opl_tests", "tests/core_tests.cpp"), ("opl_demo", "src/main.cpp")]:
            run(compiler + flags + [str(ROOT / "src/core.cpp"), str(ROOT / source), "-o", str(build / target)])
        test_output = run([str(build / "opl_tests")])
        match = re.search(r"tests=(\d+) passed=(\d+) failed=(\d+)", test_output)
        if not match or match.group(1) != match.group(2) or match.group(3) != "0":
            raise RuntimeError("test summary invalid")
        report["tests"] = {"total": int(match.group(1)), "passed": int(match.group(2)), "failed": 0}
        print(test_output, end="")
        trace = build / "demo-trace.csv"
        demo_argv = [str(build / "opl_demo"), "--seed", "2026", "--ticks", "3600"]
        demo = json.loads(run(demo_argv + ["--trace", str(trace)]))
        repeat = json.loads(run(demo_argv))
        if demo != repeat:
            raise RuntimeError("seeded demo is not repeatable")
        expected_states = ["Chase", "Search", "Cooldown", "Patrol"]
        if not (demo["race_checkpoints"] == 4 and demo["traffic_spacing_safe"] and
                demo["converged_after_explicit_no_loss_tail"] and demo["snapshot_packets_dropped"] > 0 and
                [x["state"] for x in demo["pursuit_transitions"]] == expected_states):
            raise RuntimeError("demo contracts failed")
        trace_lines = trace.read_text().splitlines()
        if len(trace_lines) != 3601:
            raise RuntimeError("trace row count mismatch")
        report["demo"] = demo
        report["benchmark"] = None if instrumented else json.loads(run([str(build / "opl_demo"), "--benchmark"]))
        for invalid_args in [["--ticks", "0"], ["--ticks", "abc"], ["--seed", "-1"], ["--unknown"], ["--seed"]]:
            run([str(build / "opl_demo")] + invalid_args, expected=2)
        if args.report and not instrumented:
            destination = args.report if args.report.is_absolute() else ROOT / args.report
            destination.parent.mkdir(parents=True, exist_ok=True)
            sampled = destination.parent / "demo-trace-sampled.csv"
            sampled.write_text("\n".join([trace_lines[0]] + trace_lines[60::60]) + "\n")
            report["sampled_trace"] = {"file": sampled.name, "rows": 60,
                                       "sha256": hashlib.sha256(sampled.read_bytes()).hexdigest()}
        report["status"] = "passed"
        print("PASS: demo replay, trace, recovery and CLI rejection checks")
        print("Replay digest:", demo["replay_digest"])
    except (OSError, RuntimeError, ValueError, subprocess.TimeoutExpired) as error:
        report["status"] = "failed"
        report["error"] = str(error).replace(str(ROOT), "<project>")
        print("FAIL:", error, file=sys.stderr)
    finally:
        report["finished_at_utc"] = datetime.datetime.now(datetime.timezone.utc).isoformat()
        if args.report:
            destination = args.report if args.report.is_absolute() else ROOT / args.report
            destination.parent.mkdir(parents=True, exist_ok=True)
            destination.write_text(json.dumps(report, indent=2) + "\n")
            print("Evidence:", destination.relative_to(ROOT) if destination.is_relative_to(ROOT) else destination)
    return 0 if report["status"] == "passed" else 1


if __name__ == "__main__":
    sys.exit(main())
