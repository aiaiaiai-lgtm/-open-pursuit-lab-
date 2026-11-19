#!/usr/bin/env python3
"""Check local document links and recorded source/evidence consistency."""
import ast
import hashlib
import json
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
from verify import source_manifest


def main():
    errors = []
    for path in ROOT.rglob("*.py"):
        if ".build" not in path.parts:
            ast.parse(path.read_text(), filename=str(path))
    for path in ROOT.rglob("*.md"):
        for match in re.finditer(r"\[[^\]]*\]\(([^)]+)\)", path.read_text()):
            target = match.group(1).split("#")[0]
            if not target or "://" in target or target.startswith("mailto:"):
                continue
            if not (path.parent / target).exists():
                errors.append("broken link {} -> {}".format(path.relative_to(ROOT), target))
    json.loads((ROOT / "unreal/OpenPursuitLab.uproject").read_text())
    current = source_manifest()
    for name in ["local-release.json", "local-ubsan.json", "local-sanitized.json"]:
        report = json.loads((ROOT / "evidence" / name).read_text())
        if report["source_sha256"] != current:
            errors.append(name + " source manifest differs from delivered files")
        if name != "local-sanitized.json":
            if report["status"] != "passed" or report["tests"] != {"total": 26, "passed": 26, "failed": 0}:
                errors.append(name + " missing expected passing result")
        elif report["status"] != "failed" or "sanitizer_malloc_mac.inc:189" not in report.get("error", ""):
            errors.append(name + " does not record the observed startup failure")
    release = json.loads((ROOT / "evidence/local-release.json").read_text())
    trace = ROOT / "evidence" / release["sampled_trace"]["file"]
    if hashlib.sha256(trace.read_bytes()).hexdigest() != release["sampled_trace"]["sha256"]:
        errors.append("sampled trace hash mismatch")
    probe = json.loads((ROOT / "evidence/asan-runtime-probe.json").read_text())
    if probe["commands"][0]["exit_code"] != 0 or probe["commands"][1]["exit_code"] == 0:
        errors.append("runtime probe does not reproduce startup failure")
    if errors:
        print("\n".join(errors), file=sys.stderr)
        return 1
    print("PASS: Python syntax, local Markdown links, UE descriptor JSON, source manifests and evidence consistency")
    print("Source/document files checked:", len(current))
    return 0


if __name__ == "__main__":
    sys.exit(main())
