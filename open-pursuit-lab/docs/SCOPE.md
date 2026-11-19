# Scope and acceptance gates

## Product intent

A freely drivable urban street racing sandbox in which traffic, competitive
driving and police response share one server-authoritative world. Original
graybox environments and generic vehicles are sufficient for the initial slice.
The engineering challenge is the interaction of these systems under latency,
collisions and a fixed runtime budget.

## Gates

| Gate | Scope | Acceptance evidence | Current state |
| --- | --- | --- | --- |
| G0: policy prototype | Integer ring traffic, pursuit rules, input gate, snapshot faults, race crossings | Local reproducible verifier and raw reports | Delivered; narrow headless scope |
| G1: engine walking skeleton | One block, one Chaos car, 8 traffic entities, one police unit, one race route; dedicated server + 2 clients | Editor/Server builds, recording, authority assertions, two-client late-join check | Planned |
| G2: constrained vertical slice | 1 km² district, 3 handling presets on one rig, 20 traffic, up to 4 police, 4 players, 3 heat tiers, one race | Packaged client/server, latency matrix, 10-minute performance trace and 30-minute soak | Planned |
| G3: scale research | Up to 100 policy agents, representation LOD, increased traffic density | Profiling evidence and migration invariants | Deferred; not a G2 promise |

G1 should prove all subsystem boundaries before content expansion. It is not an
estimate of calendar completion. No milestone duration is guaranteed; task sizes
in the backlog are review slices, not delivery promises.

## Hard G2 caps

- One district with a small fixed road graph; no procedural city generator.
- Three car configurations reuse the same asset and input pipeline. At most four
  player cars exist simultaneously; this is not three bespoke car art packages.
- Twenty traffic entities exist in total. At most eight are physical Chaos Actors;
  the rest use lane kinematics and visual proxies. Four police Actors are separate.
  Thus the physical vehicle cap is 4 + 8 + 4 = 16, not 28 Chaos vehicles.
- Server-authoritative race and pursuit state. Two-way collision outcome and
  prediction quality must be measured before claiming a good online experience.
- Daytime, dry roads, one lighting setup, one race, one session; restart resets state.
- No weapons, on-foot movement, interiors, pedestrians, deformable chassis,
  persistence, commerce, public matchmaking or platform certification.

## Cut order when budgets fail

First reduce active traffic physics from eight to four; then reduce police from
four to two; then shrink district detail and streaming density. Keep authoritative
race/pursuit rules, 4-player verification and performance measurements. If the
network vehicle spike fails, hold at G1 and document the unresolved behavior;
do not present snapshot replication alone as completed multiplayer driving.

## Stop conditions

Do not expand the map until G1 passes. Do not enable 100 physical vehicles to
match a headline target. Do not add production persistence or city generation
until G2 has a reproducible trace below its budgets. Engine version upgrades
require a separate compatibility task and an updated ADR.
