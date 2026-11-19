#include "opl/core.hpp"

#include <algorithm>
#include <charconv>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
std::uint32_t number(const char* arg) {
    const std::string value(arg); std::uint32_t result = 0;
    const auto parsed = std::from_chars(value.data(), value.data() + value.size(), result);
    if (parsed.ec != std::errc{} || parsed.ptr != value.data() + value.size()) throw std::invalid_argument("invalid integer");
    return result;
}
void benchmark() {
    using Clock = std::chrono::steady_clock;
    std::cout << "{\"scope\":\"integer traffic policy only; not UE frame time\",\"samples\":[";
    for (const auto count : {20u, 100u}) {
        opl::Traffic traffic(2026, count);
        for (unsigned i = 0; i < 100; ++i) traffic.step(true);
        std::vector<double> batches;
        for (unsigned batch = 0; batch < 30; ++batch) {
            const auto start = Clock::now();
            for (unsigned i = 0; i < 100; ++i) traffic.step(i < 70);
            const auto end = Clock::now();
            batches.push_back(std::chrono::duration<double, std::micro>(end - start).count() / 100.0);
        }
        std::sort(batches.begin(), batches.end());
        std::cout << (count == 20 ? "" : ",") << "{\"cars\":" << count
                  << ",\"batches\":30,\"ticks_per_batch\":100,\"batch_mean_us_p50\":" << batches[14]
                  << ",\"batch_mean_us_p95\":" << batches[28] << ",\"digest\":" << opl::hash_traffic(traffic) << '}';
    }
    std::cout << "]}\n";
}
}

int main(int argc, char** argv) {
    try {
        std::uint32_t seed = 2026, ticks = 3600;
        std::string trace_path;
        for (int i = 1; i < argc; ++i) {
            const std::string arg(argv[i]);
            if (arg == "--benchmark" && argc == 2) { benchmark(); return 0; }
            if ((arg == "--seed" || arg == "--ticks" || arg == "--trace") && i + 1 < argc) {
                const char* value = argv[++i];
                if (arg == "--seed") seed = number(value);
                else if (arg == "--ticks") ticks = number(value);
                else trace_path = value;
            } else throw std::invalid_argument("usage: opl_demo [--seed N] [--ticks 1..100000] [--trace file.csv] | --benchmark");
        }
        if (ticks == 0 || ticks > 100'000) throw std::invalid_argument("ticks must be 1..100000");
        std::ofstream trace;
        if (!trace_path.empty()) {
            trace.open(trace_path); if (!trace) throw std::runtime_error("cannot open trace output");
            trace << "tick,travel_mm,speed_mm_tick,pursuit,heat,checkpoints,observer_tick\n";
        }
        opl::Server server; opl::Traffic traffic(seed); opl::Observer observer;
        opl::SnapshotLink link(seed, {3, 12, 50, 100});
        std::vector<std::pair<std::uint32_t, opl::PursuitState>> transitions;
        auto previous = opl::PursuitState::Patrol;
        std::uint32_t stale = 0;
        for (std::uint32_t t = 0; t < ticks; ++t) {
            const opl::Command command{t + 1, t + 1, static_cast<std::int16_t>(t % 300 < 240 ? 1000 : 0), 0};
            if (server.submit(7, command) != opl::Reject::None) throw std::logic_error("demo command rejected");
            server.step(t >= 60 && t < 300, t == 60 ? 3 : 0);
            traffic.step(t % 600 < 420);
            const auto snap = server.snapshot();
            if (snap.pursuit != previous) { transitions.emplace_back(snap.tick, snap.pursuit); previous = snap.pursuit; }
            if (snap.tick % 3 == 0) link.send(snap.tick, snap);
            for (const auto& packet : link.receive(snap.tick)) if (!observer.apply(packet)) ++stale;
            if (trace.is_open()) trace << snap.tick << ',' << snap.travel_mm << ',' << snap.speed_mm_tick << ','
                << opl::to_string(snap.pursuit) << ',' << unsigned(snap.heat) << ',' << unsigned(snap.checkpoints)
                << ',' << (observer.has_state() ? observer.state().tick : 0) << '\n';
        }
        const auto before_recovery = observer.has_state() ? observer.state().tick : 0;
        // Explicit no-loss final delivery; not a claim of convergence under infinite loss.
        opl::SnapshotLink recovery(seed, {3, 0, 0, 0}); recovery.send(ticks, server.snapshot());
        for (std::uint32_t t = ticks + 1; t <= ticks + 30; ++t) {
            for (const auto& packet : link.receive(t)) if (!observer.apply(packet)) ++stale;
            for (const auto& packet : recovery.receive(t)) observer.apply(packet);
        }
        const auto final = server.snapshot();
        const bool converged = opl::hash_snapshot(final) == opl::hash_snapshot(observer.state());
        std::cout << "{\n  \"scope\": \"headless integer policy prototype; in-process snapshot channel\",\n"
                  << "  \"seed\": " << seed << ", \"ticks\": " << ticks << ", \"tick_hz\": 60,\n"
                  << "  \"traffic_cars\": 20, \"traffic_spacing_safe\": " << (traffic.safe_spacing() ? "true" : "false") << ",\n"
                  << "  \"race_checkpoints\": " << unsigned(final.checkpoints) << ", \"travel_mm\": " << final.travel_mm << ",\n"
                  << "  \"pursuit_final\": \"" << opl::to_string(final.pursuit) << "\", \"pursuit_transitions\": [";
        for (std::size_t i = 0; i < transitions.size(); ++i) std::cout << (i ? "," : "")
            << "{\"tick\":" << transitions[i].first << ",\"state\":\"" << opl::to_string(transitions[i].second) << "\"}";
        std::cout << "],\n  \"snapshot_packets_sent\": " << link.sent() << ", \"snapshot_packets_dropped\": " << link.dropped()
                  << ", \"snapshot_packets_duplicated\": " << link.duplicated() << ", \"stale_snapshots_rejected\": " << stale
                  << ",\n  \"observer_tick_before_recovery\": " << before_recovery
                  << ", \"converged_after_explicit_no_loss_tail\": " << (converged ? "true" : "false")
                  << ",\n  \"replay_digest\": \"" << std::hex << opl::replay_digest(seed, ticks) << "\"\n}\n";
        return converged && traffic.safe_spacing() ? 0 : 1;
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 2; }
}
