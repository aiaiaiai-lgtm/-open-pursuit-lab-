# Open Pursuit Lab

**A working C++ policy prototype and an unverified UE5 integration scaffold for
an open-world street racing and police pursuit game.**

The intended experience is a compact, freely drivable urban district with traffic,
a timed race and escalating police response. The engineering focus is the boundary
between vehicle physics, scalable traffic decisions and server authority. This is
an original project inspired by the genre; no GTA code, map, art or branding is used.

## What exists today

| Area | Delivered | Verification boundary |
| --- | --- | --- |
| Portable C++20 core | Integer 60 Hz policy model, 20-car ring traffic, red light and spacing rules | Compiled and exercised locally; no tire, steering or collision physics |
| Pursuit policy | Patrol / Investigate / Chase / Search / Cooldown; three heat tiers | Exact timer and transition tests; no police vehicle navigation |
| Authority rules | One owner, input ranges, tick/sequence guards, bounded speed, four ordered race crossings | In-process one-player model; no authentication, sockets or real multiplayer |
| Snapshot fault harness | Seeded delay, jitter, drop, duplication and stale rejection | Server-to-observer snapshots only; no prediction, interpolation or uplink simulator |
| Regression tools | Tests, seed-sensitive replay digest, CSV trace, ASan/UBSan option, traffic microbenchmark | Actual local reports in `evidence/`; GitHub workflow supplied, not executed remotely |
| UE5 C++ scaffold | Project/targets/build rules, shared policy bridge, authority-gated replicated pursuit component | Source authored; **not built or run in UE** |
| Playable UE game | Planned | No map, car assets, Mass processors, StateTree assets or playable build yet |

This repository does **not** demonstrate a completed open-world game or measured
UE frame rate. [STATUS.md](docs/STATUS.md) is the claim ledger.

## Run the real prototype

Requires Python 3.9+ and a C++20 GCC/Clang toolchain. The standard-library-only
verifier supports macOS/Linux; CMake is optional. No pip packages or UE install
are required. Run from the project root:

```sh
python3 tools/verify.py --report evidence/my-release.json
python3 tools/verify.py --ubsan --report evidence/my-ubsan.json
python3 tools/verify.py --sanitize --report evidence/my-sanitized.json
.build/release/opl_demo --seed 2026 --ticks 3600 --trace .build/release/my-trace.csv
```

`python3 tools/check_delivery.py` audits the original delivery reports against
the shipped source hashes and checks local document links. It is an audit of this
snapshot, not a replacement for rerunning the tests after changing the code.

`CXX=clang++` or `CXX=g++` selects the compiler. The verifier compiles both
executables with warnings treated as errors, runs the contract suite, repeats the
seeded scenario, validates its trace and rejects malformed CLI inputs. Exit code
0 means these checks passed; compiler availability and sanitizer support remain
toolchain prerequisites. Local release and UBSan runs passed **26 contract tests**.
The combined ASan/UBSan runtime failed before tests started, also reproduced with
an empty program; no ASan pass is claimed. See [the evidence summary](evidence/README.md).

The 3,600-tick scenario represents 60 seconds of **simulated** time; it does not
sleep in real time. It runs one abstract server-controlled racer and a separate
20-car traffic model, injects one visible tier-3 incident, then loses sight of the
suspect. Every third tick sends a snapshot through a seeded adverse channel. The
last snapshot is explicitly delivered over a no-loss recovery tail after the
server stops. This proves the observer can accept final state after recovery,
not that an actual multiplayer vehicle reconciles correctly.

The traffic model and racer are independent fixtures. They do not collide with
each other. Traffic uses one 1 km ring lane; the future 1 km² district is a
different, larger integration goal.

Optional CMake path (provided, **not locally executed** because CMake was absent):

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

## First playable scope — planned, not delivered

The first integrated gate is deliberately small: one graybox block, one Chaos car,
eight traffic entities, one police unit and a dedicated server with two clients.
After that passes, the vertical slice expands to:

- One hand-authored 1 km² district, dry daytime, no pedestrians or interiors.
- Three handling presets sharing one vehicle rig; up to four players.
- Twenty traffic vehicles total, with at most eight traffic Chaos Actors active.
- Up to four police vehicles, three heat tiers and one ordered checkpoint race.
- Session-local state, server authority and recorded performance/network evidence.

The 100-car ring test is a policy stress fixture, **not 100 live UE cars**.
Procedural cities, persistent accounts, full damage simulation, weapons,
helicopters, weather and matchmaking are deferred. More detail and exit gates are
in [SCOPE.md](docs/SCOPE.md).

## Project map

```text
include/opl/core.hpp       Portable policy contracts
src/core.cpp              Implemented policy, authority and snapshot channel
src/main.cpp              Reproducible scenario and microbenchmark
tests/core_tests.cpp      Boundary, invariant and fault tests
tools/verify.py           No-dependency build and evidence generator
unreal/                   Unverified UE5 C++ scaffold; no content assets
docs/ARCHITECTURE.md       Ownership, scheduling, Mass/StateTree and physics design
docs/BACKLOG.md            Dependency-ordered tasks ready to assign to Devin
docs/TEST_PLAN.md          Simulation, performance and multiplayer gates
docs/UE_INTEGRATION.md     Engine bring-up and acceptance runbook
evidence/                 Actual local reports and sampled trace
.github/                  Portable CI workflow and task issue template
```

## Review and continue

Start with [the architecture](docs/ARCHITECTURE.md),
[the task backlog](docs/BACKLOG.md), and [the testing plan](docs/TEST_PLAN.md).
Task `UE-01` is the first integration job; do not mark UE work complete from a
portable test result. All new feature claims require an artifact and a reproducible
check. [CONTRIBUTING.md](CONTRIBUTING.md) defines the handoff contract.

There is no public repository URL in this delivery. To publish, create an empty
GitHub repository, initialize Git in this directory, commit the source/evidence,
add the real remote, and push. Do not upload `.build/`, engine binaries or assets
whose distribution rights have not been established. The supplied CI has no
passing badge because it has not run on GitHub.

## Design references and license

Epic documents the relevant [Chaos vehicle setup](https://dev.epicgames.com/documentation/en-us/unreal-engine/how-to-set-up-vehicles-in-unreal-engine?application_version=5.6),
[MassEntity model](https://dev.epicgames.com/documentation/en-us/unreal-engine/overview-of-mass-entity-in-unreal-engine?application_version=5.6),
[StateTree model](https://dev.epicgames.com/documentation/en-us/unreal-engine/overview-of-state-tree-in-unreal-engine?application_version=5.6),
and [networked physics modes](https://dev.epicgames.com/documentation/en-us/unreal-engine/networked-physics-overview?application_version=5.6).
These inform the intended architecture; they do not establish that this scaffold
works. Version assumptions and decisions appear in [ADR-001](docs/adr/001-authority-and-determinism.md).

Authored source and documentation use the [MIT license](LICENSE). Unreal Engine
and any future third-party content remain under their own licenses. No engine
source, Marketplace content or third-party binary is bundled.
