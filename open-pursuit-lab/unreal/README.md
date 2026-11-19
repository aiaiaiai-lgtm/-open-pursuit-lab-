# UE5 integration scaffold — unverified

This directory contains a UE 5.6-targeted project descriptor, Game/Editor/Server
targets, module rules, and a replicated pursuit component intended to reuse the
portable policy implementation. **It has not been compiled, opened in Unreal
Editor, cooked, or tested on a dedicated server.** No `.umap`, vehicle rig,
StateTree asset, Mass processor, camera, or playable game is included.

The component is a concrete starting point for integration review, not proof of
engine compatibility. A replicated owning Actor and server-side observation
producer are prerequisites. `BlueprintAuthorityOnly` metadata is supplemented
with an explicit `HasAuthority()` check; the observation method is not an RPC.
Visibility stays latched until the producer updates it. Only severity 0..3 is
accepted; multiple incident edges before a step merge by maximum severity.
No client should be allowed to assert visibility, severity, heat or race results.

The module compiles `../../../../src/core.cpp` once through `PortablePolicy.cpp`.
Preserve the repository layout and shared include path. Exceptions are enabled
for this module. Engine/plugin names and API compatibility must be checked against
the selected local engine source before calling this buildable. MassEntity is
declared as a module dependency, not enabled as a made-up plugin.

The 60 Hz accumulator executes at most four policy steps per frame, records
dropped time, and slows policy time during hitches. Replace it with a validated
server simulation clock before acceptance; it is unrelated to Chaos async
physics substeps. Clients only read replicated `AlertState` and `Heat`.

See [the integration runbook](../docs/UE_INTEGRATION.md) for the first engine gate.
