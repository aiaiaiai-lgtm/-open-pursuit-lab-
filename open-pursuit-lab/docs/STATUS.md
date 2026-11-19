# Evidence and implementation status

Delivery date: 19 November 2026 (Asia/Singapore). This is a source snapshot,
not a published repository or playable release. No public URL has been created.

## Implemented and locally verified

- Portable C++20 policy core compiled with Apple Clang 16 on macOS arm64.
- 26 contract tests, seeded 3,600-tick scenario, duplicate-run comparison,
  trace validation, malformed-argument checks; release and UBSan runs passed.
- Combined ASan/UBSan test startup failed inside the local sanitizer runtime.
  An empty-main program reproduced the assertion. ASan coverage remains unverified.
- Integer lane following with stop-line behavior and spacing invariants across
  16 seeds; a separate 100-car ring stress case.
- Five-state pursuit policy, three heat tiers, timed loss/search/cooldown,
  reacquisition, bounded event input and timer boundaries.
- One-player server input gate and race progression from bounded server travel.
- In-process snapshot loss/jitter/duplication channel and a monotonic observer.
  Recovery explicitly delivers one no-loss final snapshot.
- Canonical policy replay digest and a small traffic policy microbenchmark.

Actual command output, exit codes, environment and source hashes are in
`evidence/local-release.json`, `evidence/local-ubsan.json`,
`evidence/local-sanitized.json` and `evidence/asan-runtime-probe.json`. These are local
records, not third-party certification. The golden hash was captured from the
first implementation and is a change detector, not a correctness proof.

## Authored but unverified

- UE5 project descriptor, Game/Editor/Server targets and build configuration.
- A pursuit component with an authority check and replicated state properties;
  a translation unit that compiles the same core source into the UE module.
- CMake build configuration and GitHub Actions workflow.
- Proposed technical architecture, task backlog, test procedures and budgets.

The UE scaffold targets **5.6 as a provisional compatibility baseline**, not as a
claim about the latest engine release. There was no UE installation available
for this task. No UnrealBuildTool, UnrealHeaderTool, Editor, cook, Gauntlet,
Insights capture, dedicated server or remote CI run was performed.

## Planned and absent

Chaos vehicle pawn/rig/tuning, a playable map and camera, Mass fragments/processors,
StateTree tasks/assets, lane graph and junction reservations, physical police
driving, server input buffering, network serialization, client prediction,
reconciliation, snapshot interpolation, interest management, four-player sessions,
physics crash tests, map streaming and UE performance measurements.

## Claims that must not be made from this snapshot

Do not describe this as a playable GTA clone, an integrated Chaos/Mass demo,
real online multiplayer, an anti-cheat system, deterministic Chaos lockstep,
a 60 FPS game, a running GitHub pipeline or a 100-vehicle UE benchmark. The
actual accomplishment is working, tested policy code plus concrete integration
source and implementation plans.
