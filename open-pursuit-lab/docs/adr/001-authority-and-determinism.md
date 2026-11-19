# ADR-001: authority, determinism and version boundary

Date: 19 November 2026. Status: accepted for the portable prototype; provisional
for engine integration pending UE-01 and NET-01.

## Context

Colliding high-speed vehicles, scalable traffic and police decisions have
different costs and replication needs. A claim of deterministic lockstep across
Chaos, UE versions and CPU architectures would exceed this prototype's evidence.
The current environment can compile portable C++, but cannot validate UE code.

## Decision

- Keep a small engine-independent policy layer, exact integer ticks and canonical
  seeded regression fixtures. Reuse the pursuit implementation through an adapter.
- The dedicated server owns competitive outcomes. Clients submit controls, never
  authoritative transforms, heat or race results.
- Use MassEntity + StateTree for traffic decision data and behavior. Promote only
  a capped nearby subset to Chaos Actors, with one writer per pose per tick.
- Validate Chaos with repeatable scenarios and numeric tolerances. Do not require
  bit-identical physics replay or deterministic lockstep between machines.
- Target UE 5.6 provisionally because this snapshot needs one explicit baseline,
  not a moving latest-version promise. The selected release and exact engine
  build/commit must be recorded by UE-01. Compatibility is currently unproven.
- Select owner prediction/resimulation only after the dedicated collision spike;
  no UE network mode is represented as already implemented.

## Alternatives considered

| Option | Reason not chosen as the initial architecture |
| --- | --- |
| All traffic as Chaos Actors | Couples far traffic density to rigid-body/server cost |
| Client-authoritative movement | Makes race and pursuit outcomes untrustworthy |
| Deterministic full-world lockstep | Cross-platform Chaos/contact guarantee not established |
| Full custom physics/network engine | Too much infrastructure before validating the game loop |
| Architecture documents only | Does not exercise any actual policy contracts |

## Consequences

There are two verification tiers: exact policy regression and tolerance-based UE
physics/network tests. Handoff, input buffering and client correction are real
integration tasks, not solved by having this core. No performance goal becomes an
achievement until measured on the declared map, hardware and engine build.

## Revisit triggers

Engine upgrade, failure of the 150 ms collision tests, inability to stay below
the physical Actor cap, or a requirement for persistent cross-server worlds.
Revisit decisions in a new ADR with evidence; do not silently alter this record.
