# UE5 integration runbook — not yet executed

This is the UE-01 procedure, not a build log. The delivered project has no map or
vehicle content. All commands below are templates for a future licensed setup.

## Prerequisites and version choice

Use the provisional UE 5.6 baseline or document a deliberate change in a new ADR.
Record the exact installed/source build ID, platform compiler and SDK. The
descriptor's `EngineAssociation` may need the actual registered source-build ID
rather than `5.6`; change it explicitly in the integration PR. Verify the
`MassEntity`, `ChaosVehicles` and `StateTreeModule` dependencies against that engine.
Do not assume examples for another minor release compile unchanged.

For the dedicated server gate, provision an engine source build with server target
support; Epic's [dedicated-server setup](https://dev.epicgames.com/documentation/en-us/unreal-engine/setting-up-dedicated-servers-in-unreal-engine?application_version=5.6)
documents that workflow. The portable verifier does not install UE, obtain an
Epic account, accept a license or build an engine.

## First clean build

1. Run the portable verifier and save its report. Check out source into a path
   that preserves `include/`, `src/` and `unreal/` as siblings.
2. Associate `unreal/OpenPursuitLab.uproject` with the pinned engine and generate
   project files using that installation's normal tooling.
3. Build `OpenPursuitLabEditor` Development, then `OpenPursuitLab` Development,
   then `OpenPursuitLabServer` Development for the target platform. Preserve
   UHT/UBT stdout/stderr and exit codes for each target.
4. Fix compatibility and include/UHT issues in the scaffold before adding feature
   code. `PortablePolicy.cpp` is the only UE translation unit compiling core.cpp.
   Do not link a second copy of the same implementation.

Example Windows command prompt templates, after setting task-specific paths to
the actual engine and project (these are not real local paths):

```bat
set OPL_ENGINE=D:\YourPinnedEngine
set OPL_PROJECT=D:\YourCheckout\unreal\OpenPursuitLab.uproject
"%OPL_ENGINE%\Engine\Build\BatchFiles\Build.bat" OpenPursuitLabEditor Win64 Development -Project="%OPL_PROJECT%" -WaitMutex
"%OPL_ENGINE%\Engine\Build\BatchFiles\Build.bat" OpenPursuitLab Win64 Development -Project="%OPL_PROJECT%" -WaitMutex
"%OPL_ENGINE%\Engine\Build\BatchFiles\Build.bat" OpenPursuitLabServer Win64 Development -Project="%OPL_PROJECT%" -WaitMutex
```

Successful source compilation alone is not a packaged/playable build. Add the
test map in the next step and cook/package it explicitly with the selected
engine's Project Launcher or automation tooling; attach actual output and logs.

## Minimal map and authority proof

- Create `/Game/Maps/OPL_Blockout` with ground, a few barriers and a spawn point;
  set the actual Game/Server default maps after the asset exists. No default map
  reference is supplied to a nonexistent asset in this snapshot.
- Add a replicated Actor containing `UPursuitDirectorComponent`. Its owner must
  replicate; component replication alone does not create an Actor channel.
- Feed explicit server-only observation vectors. Visibility is latched, so the
  producer must send both acquisition and loss updates. A severity is an edge,
  not a value to resend every frame. No client RPC for observations is provided.
- Replace the temporary render-frame accumulator with a validated simulation
  clock and test hitch behavior. Keep Chaos substeps separate from policy time.
- Launch a dedicated server and two separate clients on the map. Verify client
  calls cannot mutate heat, while server state reaches both clients. Start one
  client late and verify it receives the current alert state.
- Compare event transition ticks with the portable oracle and capture a video,
  server/client logs and the generated automation report. Record every unrun check.

## Add the actual driving and traffic systems

Follow VEH-01 and the official [Chaos setup guide](https://dev.epicgames.com/documentation/en-us/unreal-engine/how-to-set-up-vehicles-in-unreal-engine?application_version=5.6).
The descriptor enables a plugin; it does not supply a mesh, rig, wheels,
physics asset, torque curve, animation blueprint, input mapping or pawn.

Implement AI-01/02 for MassEntity and StateTree. The module's dependency list
does not constitute a traffic implementation. Decide which Mass gameplay/AI
plugins and modules the selected engine requires before enabling them. Commit
real configuration and assets with their tests, rather than empty placeholder
classes or a claim that plugin activation completed the feature.

## Engine CI gate

The included GitHub workflow builds only portable code on hosted runners. A
future UE pipeline needs a licensed, appropriately secured runner with a pinned
engine cache and toolchain, then Editor/Server build, map cook, automation tests,
dedicated-session tests and trace upload. Engine binaries and secrets stay outside
this repository. Do not add a green UE badge until that pipeline actually passes.
