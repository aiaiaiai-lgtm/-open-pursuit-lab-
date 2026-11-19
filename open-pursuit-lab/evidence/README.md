# Actual local verification evidence

This folder records observed local execution, not planned UE tests. Report times
are UTC; the user-facing delivery date is 19 November 2026 in Asia/Singapore.

| File | Observed result | Scope |
| --- | --- | --- |
| `local-release.json` | Passed, 26/26 contract tests | Apple Clang 16, C++20, `-O2`, warnings as errors; scenario, trace, CLI checks, microbenchmark |
| `local-ubsan.json` | Passed, 26/26 contract tests | Same prototype with UBSan and fail-on-first-finding enabled; scenario and CLI checks |
| `local-sanitized.json` | Failed at sanitizer runtime startup | Combined AddressSanitizer/UBSan binary compiled, but test process aborted before tests ran |
| `asan-runtime-probe.json` | Empty main reproduces the startup assertion | No project core linked; isolates a local runtime/toolchain limitation |
| `demo-trace-sampled.csv` | 60 sampled rows, one per simulated second | Generated from a full 3,600-row local trace; inspection aid |

The startup assertion is `sanitizer_malloc_mac.inc:189` with
`!asan_init_is_running`. It also occurs in an empty `main()`. This supports the
inference that the observed startup failure is environmental; it does **not**
establish memory safety of this project. AddressSanitizer coverage is outstanding
and should be rerun on a supported toolchain. UBSan is a separate successful check.

## Seed-2026 scenario

3,600 ticks at 60 Hz represent 60 simulated seconds, without real-time sleeping.
The final server travels 1,757,924 mm and completes four ordered checkpoints.
Pursuit transitions are Chase at tick 61, Search at 480, Cooldown at 1080, Patrol
at 1380. Replay digest: `28b84a5f94c2a8ea`.

The adverse snapshot channel sends 1,200 packets, drops 53 and adds 120 duplicates.
The observer's last tick before recovery is 3591. After the server stops, a
separate no-loss final delivery and 30-tick drain produce exact final snapshot
agreement; the run rejects 509 stale/duplicate deliveries including that drain.
This tests one-way snapshot handling in one process. It is not a test of real
multiplayer, upstream input, prediction, collision reconciliation or networking APIs.

Reports contain raw command stdout/stderr, exit codes, environment, flags and a
SHA-256 manifest of source/docs at execution. `evidence/`, build products and Git
metadata are excluded from that source manifest. Report generation is local and
self-reported; another developer should rerun the verifier independently.

The release benchmark measures 20/100-car **integer policy fixtures**. Timings are
machine-specific percentiles of 30 batch means, not UE frame time or a Chaos
vehicle benchmark. Read the actual numbers in the report; no game FPS claim is
inferred. Sanitized reports omit performance numbers.

## Reproduce

```sh
python3 tools/verify.py --report evidence/reproduced-release.json
python3 tools/verify.py --ubsan --report evidence/reproduced-ubsan.json
python3 tools/verify.py --sanitize --report evidence/reproduced-asan.json
```

Do not overwrite delivery reports when reviewing a later change. The GitHub
workflow is authored but not remotely executed; no hosted CI status is asserted.
Unreal Editor/Server builds, CMake/CTest, actual client/server networking,
performance budgets and the proposed UE test plan have not been executed here.
