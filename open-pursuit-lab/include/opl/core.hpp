#pragma once

#include <cstdint>
#include <vector>

namespace opl {

// Integer policy model, 60 ticks/s. This is NOT a vehicle physics solver.
constexpr std::uint32_t kTickHz = 60;
constexpr std::int32_t kLoopMm = 1'000'000;
constexpr std::int32_t kSafeGapMm = 6'000;
constexpr std::int32_t kStopLineMm = 495'000;

class Random {
public:
    explicit Random(std::uint32_t seed) : state_(seed ? seed : 1) {}
    std::uint32_t next();
private:
    std::uint32_t state_;
};

enum class PursuitState : std::uint8_t { Patrol, Investigate, Chase, Search, Cooldown };
const char* to_string(PursuitState state);

class Pursuit {
public:
    static constexpr std::uint32_t kInvestigateTicks = 120;
    static constexpr std::uint32_t kLostSightTicks = 180;
    static constexpr std::uint32_t kSearchTicks = 600;
    static constexpr std::uint32_t kCooldownTicks = 300;
    // Incident severity is a SERVER-derived edge event, 0 = no incident, 1..3 = tier.
    void step(bool visible, std::uint8_t incident_severity);
    PursuitState state() const { return state_; }
    std::uint8_t heat() const { return heat_; }
    std::uint32_t elapsed() const { return elapsed_; }
    std::uint32_t unseen() const { return unseen_; }
private:
    void enter(PursuitState state);
    PursuitState state_ = PursuitState::Patrol;
    std::uint8_t heat_ = 0;
    std::uint32_t elapsed_ = 0;
    std::uint32_t unseen_ = 0;
};

struct TrafficCar {
    std::uint32_t id;
    std::int32_t position_mm;
    std::int32_t speed_mm_tick;
    std::int32_t cruise_mm_tick;
};

class Traffic {
public:
    explicit Traffic(std::uint32_t seed, std::uint32_t count = 20);
    void step(bool signal_green);
    const std::vector<TrafficCar>& cars() const { return cars_; }
    bool safe_spacing() const;
private:
    std::vector<TrafficCar> cars_;
};

// Forward-only, one-lap policy fixture. Checkpoints are derived from server travel.
class Race {
public:
    void advance(std::int64_t old_mm, std::int64_t new_mm);
    std::uint8_t passed() const { return passed_; }
    bool finished() const { return passed_ == 4; }
private:
    std::uint8_t passed_ = 0;
};

struct Command {
    std::uint32_t sequence;
    std::uint32_t target_tick;
    std::int16_t throttle; // [0,1000]
    std::int16_t brake;    // [0,1000]
};
enum class Reject { None, WrongOwner, InvalidRange, WrongTick, StaleSequence, AlreadyQueued };
const char* to_string(Reject result);

struct Snapshot {
    std::uint32_t tick = 0;
    std::int64_t travel_mm = 0;
    std::int32_t speed_mm_tick = 0;
    PursuitState pursuit = PursuitState::Patrol;
    std::uint8_t heat = 0;
    std::uint8_t checkpoints = 0;
};

class Server {
public:
    explicit Server(std::uint32_t owner = 7) : owner_(owner) {}
    // Strict one-tick mailbox. No future input buffer, authentication or sockets.
    Reject submit(std::uint32_t peer, const Command& command);
    void step(bool visible = false, std::uint8_t incident_severity = 0);
    Snapshot snapshot() const;
    std::uint32_t tick() const { return tick_; }
private:
    std::uint32_t owner_;
    std::uint32_t tick_ = 0;
    std::uint32_t last_sequence_ = 0;
    bool has_sequence_ = false;
    bool queued_ = false;
    Command pending_{};
    std::int64_t travel_mm_ = 0;
    std::int32_t speed_ = 0;
    Race race_;
    Pursuit pursuit_;
};

class Observer {
public:
    bool apply(const Snapshot& snapshot);
    bool has_state() const { return has_state_; }
    const Snapshot& state() const { return state_; }
private:
    bool has_state_ = false;
    Snapshot state_{};
};

struct LinkConfig {
    std::uint32_t delay_ticks = 3;
    std::uint32_t jitter_ticks = 2;
    std::uint32_t loss_per_mille = 50;
    std::uint32_t duplicate_per_mille = 100;
};

// In-process snapshot channel. Independent of UE and any transport protocol.
class SnapshotLink {
public:
    SnapshotLink(std::uint32_t seed, LinkConfig config);
    void send(std::uint32_t now, const Snapshot& snapshot);
    std::vector<Snapshot> receive(std::uint32_t now);
    std::uint32_t dropped() const { return dropped_; }
    std::uint32_t duplicated() const { return duplicated_; }
    std::uint32_t sent() const { return sent_; }
private:
    struct Packet { std::uint64_t due; Snapshot snapshot; };
    Random random_;
    LinkConfig config_;
    std::vector<Packet> packets_;
    std::uint32_t dropped_ = 0;
    std::uint32_t duplicated_ = 0;
    std::uint32_t sent_ = 0;
};

// Canonical byte-order hash: regression signal, not a cryptographic checksum.
std::uint64_t hash_snapshot(const Snapshot& state);
std::uint64_t hash_traffic(const Traffic& traffic);
std::uint64_t replay_digest(std::uint32_t seed, std::uint32_t ticks);

} // namespace opl
