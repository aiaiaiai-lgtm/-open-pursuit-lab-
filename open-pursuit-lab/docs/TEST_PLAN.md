# Simulation, performance and multiplayer test plan

Status: portable checks have local evidence. Every UE test, capture and numeric
budget below is **planned and unmeasured** until its task produces an artifact.

## Verification tiers

| Tier | Method | Pass rule | Evidence state |
| --- | --- | --- | --- |
| Policy contracts | Portable test executable, no UE dependency | All named assertions pass; nonzero exit on any failure | 26 tests passed locally in release and UBSan |
| Replay regression | Seed 2026, 3,600 ticks; canonical hash and trace | Same output on repeat; captured baseline `28b84a5f94c2a8ea` | Passed locally; cross-host execution not established |
| Memory instrumentation | AddressSanitizer + UBSan | Test executable starts and completes with no findings | Combined runtime failed at startup; ASan remains unverified |
| UE integration | Automation plus dedicated process tests | Exact policy events, tolerance-based physics | Not run |
| Multiplayer quality | Real server/client processes with impairment | Authority, correction and late-join thresholds below | Not run |
| Performance | Packaged builds, Insights/network traces | Declared p95/p99 and memory/bandwidth budgets | Not run |

The portable suite covers timer boundaries/reacquisition, heat rules, invalid
events, traffic spacing/red lights, race order, owner/range/tick/sequence guards,
sequence wrap, missing input, speed cap, snapshot ordering, total loss, duplicate
suppression, recovery and replay sensitivity. See the raw `[PASS]` lines in
`evidence/local-release.json`. It does not cover UE or real transport code.

## Deterministic simulation procedure

1. Build from a clean source snapshot with a recorded compiler and flags. Run
   `tools/verify.py`, then UBSan, then combined ASan/UBSan on a supported runtime.
2. Use fixed seed 2026 and 3,600 ticks for the golden regression. Repeat and compare
   JSON output plus CSV event/state traces, not just process exit status.
3. Exercise at least seeds 1..16 for the ring spacing invariant; keep count and
   step counts in the report. The current suite runs 6,000 ticks per 20-car seed,
   plus 1,200 ticks for one 100-car fixture.
4. On a deliberate policy change, compare the first differing tick, inspect the
   rule change and update the golden baseline in a reviewed commit. Never update
   the expected digest automatically to hide a failing test.
5. In UE, record input/event vectors against the exact engine build. Compare
   policy transitions exactly; compare Chaos transforms/contact outcomes using
   tolerances. Repeat on different hardware to measure divergence. Do not require
   a cross-platform bit-identical Chaos digest.

Future engine traces must include full policy inputs, internal timer values,
checkpoint events and representation epochs so a matching compact digest cannot
hide a missing observation. The current sampled CSV is an inspection aid, not
the full input log or a save/restore mechanism.

## UE functional scenarios

| ID | Scenario / setup | Required assertion | Owner task |
| --- | --- | --- | --- |
| SIM-01 | Event vectors at different render frame rates and a 100 ms hitch | Same policy transition ticks on the validated clock; overload explicitly counted | UE-01 / TEST-01 |
| SIM-02 | Acceleration, brake, reverse/reset, low/high-speed barrier impact | No invalid transform/velocity; reset has defined race semantics | VEH-01 |
| SIM-03 | Eight cars approach four-way junction with fixed seeds | Mutual exclusion, signal compliance, bounded reservation lifetime | AI-02 |
| SIM-04 | Disabled vehicle blocks a lane for 60 s | Recovery releases reservations and avoids permanent deadlock | AI-02 |
| SIM-05 | Players oscillate across LOD thresholds 100 times | One pose writer, one body, stable ID/epoch; no active-contact demotion | AI-03 |
| SIM-06 | Pool full while four players spread apart | Traffic Actor cap remains 8 globally; explicit fallback telemetry | AI-03 / SLICE-01 |
| SIM-07 | Acquire, lose, reacquire, then escape police | Exact heat/timer policy; server-only dispatch; unit cap 1/2/4 | POL-01 |
| SIM-08 | Cross checkpoint quickly, reverse, reset, replay overlap | Swept crossing counts once; no client-awarded progress | RACE-01 |
| SIM-09 | Trigger same incident through prediction replay twice | Event ID deduplication; one heat/spawn decision | POL-01 / NET-03 |

SIM-02 must capture initial speed, vehicle mass/tune, contact normal, final state
and engine build. Tolerance selection should follow observed solver behavior;
initial thresholds are finite state, no penetration > 10 cm persisting > 0.5 s,
and no unexplained speed gain > 5 m/s in a stationary-barrier test. These are
provisional acceptance targets, not measured engine guarantees.

## Multiplayer matrix

Use one dedicated server and 2 clients for G1, then 4 clients for G2. Run separate
processes; repeat reference-hardware performance captures with clients on separate
machines so a shared machine does not hide CPU contention. Record actual observed
RTT, directional delay, jitter distribution, packet loss and impairment commands.
Configured values alone do not prove the network profile was applied.

| Profile | RTT target | Per-direction jitter | Per-direction loss | Use |
| --- | --- | --- | --- | --- |
| LAN | 0–10 ms | 0 ms | 0% | Baseline correctness |
| Typical | 80 ms | +/-10 ms | 1% | Required quality gate |
| Adverse | 150 ms | +/-25 ms | 3% | Required degraded quality gate |
| Stress | 250 ms | +/-50 ms | 5% | Observe graceful degradation; no quality promise |
| Disconnect | All packets dropped for 5 s, then recovery | N/A | 100% burst | Safe fallback, bounded queues, no stale-epoch revival |

The current portable channel uses uniform additional one-way delay of 0..12
ticks on top of 3 ticks, plus seeded 5% drops and 10% duplicate probability. This
is a different experiment from the two-way RTT matrix above.

For each required profile run 10 seeds x 120 simulated seconds with: free driving,
head-on/side/rear contact, traffic promotion during contact, pursuit escalation,
checkpoint finish ties, disconnect/reconnect and late join mid-pursuit. Record
server result, each client's final state, ACK/frame lag and correction histograms.

Provisional gates to ratify in NET-01:

- Server is the only writer of heat, finish order, collision outcome and spawns.
  Spoofed ownership, invalid fields, stale epochs and input floods cannot mutate
  those outcomes or allocate unbounded history.
- At Typical RTT, owner position correction p95 <= 0.5 m; at Adverse, p95 <= 1.0 m.
  Measure error at the same acknowledged physics frame, before visual smoothing.
- Count corrections > 2 m separately: <= 1 per minute in free driving on Typical;
  collision scenarios report a separate distribution and <= 3 per minute. Do not
  blend collision errors into an easy free-driving average.
- After input/network recovery and a 2 s settle window, reported clients agree
  with server pursuit tier, checkpoint index and finish order. Remote positional
  presentation converges within 0.25 m after the cars stop. Compare time-aligned
  physical state as well as presentation state.
- Late-join baseline becomes complete within 2 s of connection readiness on
  Typical; destroyed and prior-epoch entities do not reappear.
- Input buffers are capped at 64 frames per player; prediction history at 128
  frames per predicted vehicle initially. Overflow is counted and handled safely.

A snapshot observer receiving a guaranteed final packet does not satisfy these
gates. The portable final no-loss tail is explicitly a narrower recovery test.

## Performance budget — targets only

Reference profile to provision: Windows 11 packaged Development client on Ryzen
5 5600 / RTX 3060 / 16 GB, 1080p medium blockout settings, Lumen and Nanite off.
Dedicated Linux server on the same CPU class with 4 cores allocated; clients on
separate machines. This is a proposed benchmark profile, not the current host.
Record exact OS, CPU/GPU drivers, engine commit and project configuration.

G2 load: 4 player Chaos cars + at most 8 traffic Chaos cars + 4 police Chaos cars
= **16 physical vehicles**, alongside 20 total traffic entities. G1 uses the
smaller load in SCOPE.md but must fit the same limits before expansion.

| Metric | Initial gate | Measurement |
| --- | --- | --- |
| Client displayed frame time | p95 <= 16.67 ms, p99 <= 25 ms | 10-minute packaged capture after 60 s warmup |
| Client game thread / render thread / GPU | p95 <= 7 / 6 / 14 ms respectively | Separate timing tracks; overlapping work, not additive |
| Dedicated simulation tick at 60 Hz | p95 <= 12 ms, p99 <= 16.67 ms | Server Insights; record deadline misses |
| Player vehicle physics | Planning allocation 4 ms/tick | Exclusive server attribution |
| Traffic + police physics | Planning allocation 2 ms/tick | Capped physical Actor set |
| Traffic decisions + StateTree | Planning allocation 1.5 ms/tick | Report burst and amortized costs |
| Input / pursuit / race / replication / other | Planning allocations 0.3 / 0.2 / 0.2 / 1.0 / 0.8 ms | Sum with physics/AI = 10 ms; 2 ms buffer to p95 gate |
| Server / client peak resident memory | <= 2 GiB / <= 6 GiB | OS RSS + engine allocation trace, not just UObject count |
| Server egress per client | <= 40 KiB/s sustained, <= 80 KiB/s 1 s burst | Payload and transport overhead reported separately |
| Traffic handoff discontinuity | <= 10 cm, <= 0.5 m/s | Tick-aligned before/after pose and velocity |
| Steady-state hitch | No > 100 ms hitch after warmup | Streaming, GC and shader causes annotated |

Subsystem allocations are planning guidance, not a substitute for the total tick
gate. Use exclusive timing scopes to avoid double-counting parallel/nested work.
Client 60 FPS is a design goal; a p99 up to 25 ms explicitly allows occasional
slower frames and must not be marketed as a locked 60 FPS result.

Bandwidth sizing example: 28 relevant entities x 32-byte compact state x 20 Hz
= 17,920 bytes/s before headers, baselines and retransmission. This is arithmetic
for a proposed encoding, not measured traffic. Profile the actual protocol and
use relevancy/deltas if the overhead exceeds the gate.

## Capture and soak procedure

Warm up shaders/assets for 60 s, then capture 10 minutes on the fixed route with
four clients spread across the district. Repeat three times. Report frame/tick
sample counts and nearest-rank p50/p95/p99 per run, plus the worst run; do not
average percentiles across runs. Include 1 s bandwidth peaks and hitch causes.

Run a separate 30-minute join/leave, pursuit and race loop. After 5 minutes warmup,
compare median RSS over minutes 5–10 and 25–30: <= 5% unexplained growth; no crashes,
NaNs, stuck reservations, stale police Actors or queue growth beyond caps. Record
allocator/pool occupancy so caching is distinguished from leaks.

The portable benchmark records 30 batches of 100 policy ticks at 20 and 100
cars. Its p50/p95 values are **percentiles of batch mean microseconds per tick**,
not per-frame tail latency, UE timings, or evidence that 100 Chaos cars are viable.
No portable microbenchmark threshold is used to assert a game performance pass.

## Report schema and acceptance

Each future UE report needs: scenario ID, task ID, status (pass/fail/not-run), source
revision, exact engine build, platform/toolchain, map/assets revision, seed, load,
network profile, observed impairment, commands/exit codes, metric definitions,
sample counts, failures and raw trace/log/recording locations. Do not commit large
captures to Git; attach them as real build artifacts and record actual links only.

Passing a milestone requires all its required checks and artifacts. Missing engine,
unavailable hardware or a failed sanitizer runtime is recorded as not executed or
failed startup, never converted into a pass. GitHub CI success is only claimed
after the actual hosted workflow runs.
