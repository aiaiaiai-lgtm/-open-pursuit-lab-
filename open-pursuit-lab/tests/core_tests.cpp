#include "opl/core.hpp"

#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
template<class F> void invalid(F action) {
    try { action(); } catch (const std::invalid_argument&) { return; }
    throw std::runtime_error("expected invalid_argument");
}
void advance(opl::Pursuit& p, std::uint32_t ticks, bool visible = false) {
    for (std::uint32_t i = 0; i < ticks; ++i) p.step(visible, 0);
}
}

int main() {
    using namespace opl;
    const std::vector<std::pair<std::string, std::function<void()>>> tests = {
        {"patrol ignores visibility without an incident", [] {
            Pursuit p; advance(p, 1000, true);
            require(p.state() == PursuitState::Patrol && p.heat() == 0, "false pursuit");
        }},
        {"unseen incident investigates then searches on exact boundary", [] {
            Pursuit p; p.step(false, 1); advance(p, Pursuit::kInvestigateTicks - 1);
            require(p.state() == PursuitState::Investigate, "early search");
            p.step(false, 0); require(p.state() == PursuitState::Search, "late search");
        }},
        {"visibility acquires suspect and loss timer resets", [] {
            Pursuit p; p.step(false, 2); p.step(true, 0);
            require(p.state() == PursuitState::Chase && p.heat() == 2, "acquisition failed");
            advance(p, 179); p.step(true, 0); advance(p, 179);
            require(p.state() == PursuitState::Chase, "visibility did not reset loss");
            p.step(false, 0); require(p.state() == PursuitState::Search, "loss boundary failed");
        }},
        {"search cools down and eventually clears heat", [] {
            Pursuit p; p.step(true, 3); advance(p, 180); advance(p, 599);
            require(p.state() == PursuitState::Search && p.heat() == 3, "early cooldown");
            p.step(false, 0); require(p.state() == PursuitState::Cooldown, "missing cooldown");
            advance(p, 299); require(p.heat() == 3, "early heat clear");
            p.step(false, 0); require(p.state() == PursuitState::Patrol && p.heat() == 0, "heat stuck");
        }},
        {"search and cooldown allow reacquisition", [] {
            Pursuit p; p.step(true, 2); advance(p, 180); p.step(true, 0);
            require(p.state() == PursuitState::Chase, "search reacquisition failed");
            advance(p, 780); require(p.state() == PursuitState::Cooldown, "fixture wrong");
            p.step(true, 0); require(p.state() == PursuitState::Chase, "cooldown reacquisition failed");
        }},
        {"heat escalates monotonically until cleared", [] {
            Pursuit p;
            for (std::uint8_t level = 1; level <= 3; ++level) {
                p.step(true, level); require(p.heat() == level, "tier missing");
            }
            p.step(true, 1); require(p.heat() == 3, "tier downgraded");
            invalid([&] { p.step(true, 4); });
            require(p.heat() == 3 && p.state() == PursuitState::Chase, "invalid event mutated policy");
        }},
        {"new incident interrupts cooldown", [] {
            Pursuit p; p.step(true, 1); advance(p, 780); p.step(false, 3);
            require(p.state() == PursuitState::Investigate && p.heat() == 3, "new incident ignored");
        }},
        {"traffic rejects unsupported density", [] {
            invalid([] { Traffic t(1, 0); }); invalid([] { Traffic t(1, 101); });
        }},
        {"traffic stops at red and resumes at green", [] {
            Traffic t(1, 1);
            for (unsigned i = 0; i < 3000; ++i) t.step(false);
            require(t.cars()[0].position_mm == kStopLineMm && t.cars()[0].speed_mm_tick == 0, "ran red");
            t.step(true); require(t.cars()[0].position_mm > kStopLineMm, "stuck at green");
        }},
        {"traffic spacing holds across seeds and signals", [] {
            for (std::uint32_t seed = 1; seed <= 16; ++seed) {
                Traffic t(seed, 20);
                for (unsigned tick = 0; tick < 6000; ++tick) {
                    t.step(tick % 600 < 420);
                    require(t.safe_spacing(), "traffic overlap");
                    for (const auto& car : t.cars()) {
                        require(car.position_mm >= 0 && car.position_mm < kLoopMm, "position out of bounds");
                        require(car.speed_mm_tick >= 0 && car.speed_mm_tick <= car.cruise_mm_tick, "speed out of bounds");
                    }
                }
            }
        }},
        {"100-car ring preserves spacing", [] {
            Traffic t(77, 100);
            for (unsigned tick = 0; tick < 1200; ++tick) {
                t.step(tick % 600 < 420); require(t.safe_spacing(), "dense ring overlap");
            }
        }},
        {"race requires ordered bounded crossings", [] {
            Race r; r.advance(249'500, 250'000);
            require(r.passed() == 1, "first checkpoint missed");
            r.advance(249'500, 250'000); require(r.passed() == 1, "duplicate counted");
            r.advance(749'500, 750'000); require(r.passed() == 1, "out of order counted");
            invalid([&] { r.advance(0, kLoopMm); });
            invalid([&] { r.advance(100, 99); });
            invalid([&] { r.advance(-1, 0); });
            r.advance(499'500, 500'000); r.advance(749'500, 750'000); r.advance(999'500, 1'000'000);
            require(r.finished(), "race did not finish");
        }},
        {"server rejects ownership range tick and duplicate attacks", [] {
            Server s;
            require(s.submit(8, {1, 1, 1000, 0}) == Reject::WrongOwner, "spoof accepted");
            require(s.submit(7, {1, 1, 1001, 0}) == Reject::InvalidRange, "throttle accepted");
            require(s.submit(7, {1, 1, 0, -1}) == Reject::InvalidRange, "brake accepted");
            require(s.submit(7, {1, 0, 0, 0}) == Reject::WrongTick, "past accepted");
            require(s.submit(7, {1, 2, 0, 0}) == Reject::WrongTick, "future accepted");
            require(s.submit(7, {1, 1, 1000, 0}) == Reject::None, "valid rejected");
            require(s.submit(7, {1, 1, 1000, 0}) == Reject::StaleSequence, "duplicate accepted");
            require(s.submit(7, {2, 1, 1000, 0}) == Reject::AlreadyQueued, "flood replaced command");
            s.step();
            require(s.submit(7, {1, 2, 1000, 0}) == Reject::StaleSequence, "replay accepted");
            require(s.submit(7, {2, 2, 1000, 0}) == Reject::None, "flood poisoned sequence");
        }},
        {"rejected commands leave server state unchanged", [] {
            Server s; const auto before = hash_snapshot(s.snapshot());
            for (unsigned i = 0; i < 1000; ++i) s.submit(99, {i, 1, 1000, 0});
            require(before == hash_snapshot(s.snapshot()), "rejected input moved server");
            s.step(); require(s.snapshot().travel_mm == 0, "rejected input queued");
        }},
        {"sequence wraps but half-range ambiguity is rejected", [] {
            Server s; require(s.submit(7, {0xffffffffu, 1, 1, 0}) == Reject::None, "initial sequence");
            s.step(); require(s.submit(7, {0, 2, 1, 0}) == Reject::None, "wrap rejected");
            s.step(); require(s.submit(7, {0x80000000u, 3, 1, 0}) == Reject::StaleSequence, "ambiguous accepted");
        }},
        {"missing input decays instead of retaining throttle", [] {
            Server s; s.submit(7, {1, 1, 1000, 0}); s.step();
            require(s.snapshot().speed_mm_tick == 7, "acceleration wrong");
            s.step(); require(s.snapshot().speed_mm_tick == 6, "stale throttle reused");
            for (unsigned i = 0; i < 20; ++i) s.step();
            require(s.snapshot().speed_mm_tick == 0, "neutral did not stop");
        }},
        {"server speed cap and geometric race finish", [] {
            Server s;
            for (unsigned t = 1; t <= 2200; ++t) {
                require(s.submit(7, {t, t, 1000, 0}) == Reject::None, "input rejected"); s.step();
                require(s.snapshot().speed_mm_tick <= 500, "cap exceeded");
            }
            require(s.snapshot().checkpoints == 4, "server race unfinished");
        }},
        {"server invalid incident does not partially advance tick", [] {
            Server s; invalid([&] { s.step(true, 255); });
            require(s.tick() == 0, "partial invalid step");
        }},
        {"observer rejects stale duplicate and reordered snapshots", [] {
            Observer o;
            require(o.apply({20, 10, 1, PursuitState::Chase, 3, 0}), "first snapshot rejected");
            require(!o.apply({19}) && !o.apply({20}), "stale snapshot accepted");
            require(o.state().travel_mm == 10 && o.state().heat == 3, "observer rolled back");
            require(o.apply({21}), "new snapshot rejected");
        }},
        {"snapshot link validates bounded configuration", [] {
            invalid([] { SnapshotLink l(1, {0, 0, 1001, 0}); });
            invalid([] { SnapshotLink l(1, {0, 601, 0, 0}); });
        }},
        {"total packet loss delivers nothing", [] {
            SnapshotLink link(1, {0, 0, 1000, 0});
            for (unsigned t = 1; t <= 100; ++t) link.send(t, {t});
            require(link.receive(1000).empty() && link.dropped() == 100, "loss not applied");
        }},
        {"delay boundary and duplicate suppression", [] {
            SnapshotLink link(1, {3, 0, 0, 1000}); link.send(10, {1}); Observer o;
            require(link.receive(12).empty(), "early packet");
            const auto first = link.receive(13); require(first.size() == 1 && o.apply(first[0]), "missing packet");
            const auto duplicate = link.receive(14);
            require(duplicate.size() == 1 && !o.apply(duplicate[0]), "duplicate state applied");
        }},
        {"adverse link reorders packets without observer rollback", [] {
            SnapshotLink link(2026, {3, 12, 100, 200}); Observer o;
            unsigned stale = 0, last = 0;
            for (unsigned t = 1; t <= 1000; ++t) {
                link.send(t, {t});
                for (const auto& snap : link.receive(t)) {
                    if (!o.apply(snap)) ++stale;
                    require(o.state().tick >= last, "observer rolled back"); last = o.state().tick;
                }
            }
            require(link.dropped() > 0 && link.duplicated() > 0 && stale > 0, "fault path not exercised");
        }},
        {"snapshot observer converges after explicit no-loss recovery", [] {
            SnapshotLink bad(2026, {3, 12, 100, 200}); Observer o; Server server;
            for (unsigned t = 1; t <= 600; ++t) {
                server.submit(7, {t, t, 1000, 0}); server.step();
                if (t % 3 == 0) bad.send(t, server.snapshot());
                for (const auto& snap : bad.receive(t)) o.apply(snap);
            }
            SnapshotLink recovery(1, {3, 0, 0, 0}); recovery.send(600, server.snapshot());
            for (unsigned t = 601; t <= 630; ++t) {
                for (const auto& snap : bad.receive(t)) o.apply(snap);
                for (const auto& snap : recovery.receive(t)) o.apply(snap);
            }
            require(hash_snapshot(o.state()) == hash_snapshot(server.snapshot()), "recovery mismatch");
        }},
        {"recorded replay baseline detects policy drift", [] {
            // Captured from the first local implementation, not an independent correctness oracle.
            require(replay_digest(2026, 3600) == 0x28b84a5f94c2a8eaull, "golden replay changed; review policy diff");
        }},
        {"integer replay is repeatable and seed-sensitive", [] {
            const auto a = replay_digest(2026, 3600);
            require(a == replay_digest(2026, 3600), "replay diverged");
            require(a != replay_digest(2027, 3600), "seed not used");
            require(a != replay_digest(2026, 3599), "tick not used");
        }}
    };
    unsigned failures = 0;
    for (const auto& test : tests) {
        try { test.second(); std::cout << "[PASS] " << test.first << '\n'; }
        catch (const std::exception& e) { ++failures; std::cout << "[FAIL] " << test.first << ": " << e.what() << '\n'; }
    }
    std::cout << "tests=" << tests.size() << " passed=" << tests.size() - failures << " failed=" << failures << '\n';
    return failures == 0 ? 0 : 1;
}
