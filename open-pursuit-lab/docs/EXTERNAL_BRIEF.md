# Open Pursuit Lab — project brief

Open Pursuit Lab is an early engineering project for an original open-world
street racing game with reactive traffic and escalating police pursuits. The
target stack is Unreal Engine 5 C++, Chaos Vehicles, MassEntity + StateTree and
server-authoritative multiplayer.

The first real deliverable is a runnable, engine-independent C++ policy prototype:
20-car lane following and red-light rules, a five-state pursuit policy with three
heat tiers, server input validation, ordered race checkpoints, and a seeded
snapshot fault harness. It has 26 passing local contract tests in release and
UBSan builds, a repeatable 60-second simulated scenario and raw evidence files.
AddressSanitizer failed at runtime startup on this host and remains unverified.

The repository also includes an authored UE5 integration scaffold, an architecture
document, a dependency-ordered Devin backlog and explicit simulation, performance
and multiplayer test gates. The Unreal scaffold has not been built or run; Chaos
driving, Mass/StateTree adapters and real online multiplayer are still to implement.

The first engine gate is one city block, one Chaos car, eight traffic entities,
one police unit, a race route and a dedicated server with two clients. Only after
that passes will the slice expand to a 1 km² district, three handling presets,
20 traffic vehicles, up to four police and four players. A global physical vehicle
cap keeps that scope measurable.

The hard work is coordinating kinematic/physics handoff, junction reservations,
latency-sensitive vehicle contacts and authoritative gameplay within a fixed
server/client budget. The next step is a pinned UE build and a two-car networked
physics spike, with actual logs and traces deciding the approach.

No public repository URL is claimed. The supplied source archive can be reviewed,
run locally and uploaded to a new repository. Start with README.md, then
docs/ARCHITECTURE.md and docs/BACKLOG.md.
